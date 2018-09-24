#pragma comment(lib, "ws2_32.lib") 

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define SOCKET_ERROR -1

#include "Windows.h"
#include "WinSock2.h"
#include "Ws2tcpip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace std; 

struct Server
{
	SOCKET s;
	struct sockaddr_in bindAddr;

	string curr_message; 
	int num_connections; 
};

bool InitSockets()
{
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		// Log and handle error
		if (WSAGetLastError() == SOCKET_ERROR)
			return false;
	}

	return true;
}

bool CleanUpSockets()
{
	int iResult = WSACleanup();
	return true;
}

SOCKET CreateUDPSocket()
{
	return socket(AF_INET, SOCK_DGRAM, 0);
}

SOCKET CreateTCPSocket()
{
	return socket(AF_INET, SOCK_STREAM, 0);
}

bool CloseSocket(SOCKET s)
{
	auto result = closesocket(s);

	if (result == SOCKET_ERROR)
		return false;

	return true;
}
//Descriptive error message
void printWSErrorAndExit(const char *msg)
{
	wchar_t *s = NULL;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
		| FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&s,
		0,
		NULL);
	fprintf(stderr, "%s: %S\n", msg, s);
	LocalFree(s);
	system("pause");
	exit(-1);
}

int main(int argc, char **argv)
{
	const char* error = "";
	Server server; 

	InitSockets(); 
	server.s = CreateUDPSocket();
	//Binding
	server.bindAddr.sin_family = AF_INET;
	server.bindAddr.sin_port = htons(8000);
	server.bindAddr.sin_addr.S_un.S_addr = INADDR_ANY;

	server.num_connections = 0;
	int enable = 1;
	int ret = setsockopt(server.s, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));

	if (ret == SOCKET_ERROR)
		printWSErrorAndExit(error);

	ret = bind(server.s, (const struct sockaddr*)&server.bindAddr, sizeof(server.bindAddr));
	
	if (ret == SOCKET_ERROR)
		printWSErrorAndExit(error);

	while (server.num_connections < 5)
	{
		const char* message = "Pong";
		const char *remoteAddrStr = "127.0.0.1";

		char* buf = new char[10];
		int size = sizeof(sockaddr);
		//Recieve Ping
		ret = recvfrom(server.s, buf, 10, 0, (struct sockaddr*)&server.bindAddr, &size);
		server.curr_message = buf;

		//Send Pong
		inet_pton(AF_INET, remoteAddrStr, &server.bindAddr.sin_addr);
		ret = sendto(server.s, message, sizeof(message), 0, (struct sockaddr*)&server.bindAddr,sizeof(server.bindAddr));
		if (ret != false)
		{
			server.num_connections++;
			printf("Message %d sended succesfully: %s \n", server.num_connections, buf);
		}
	}
		
	
	CloseSocket(server.s); 
	CleanUpSockets();
	system("pause");
}


