#pragma comment(lib, "ws2_32.lib") 

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define SOCKET_ERROR -1
#define MAX_MESAGES 5

#include "Windows.h"
#include "WinSock2.h"
#include "Ws2tcpip.h"

#include <string>

using namespace std; 

struct Client
{
	SOCKET s; 
	struct sockaddr_in remoteAddr;

	int msg_count_success = 0; 
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
	Client client; 

	InitSockets();
	client.s = CreateUDPSocket();
	//Destination IP address
	client.remoteAddr.sin_family = AF_INET;
	client.remoteAddr.sin_port = htons(8000);
	const char *remoteAddrStr = "127.0.0.1";
	inet_pton(AF_INET, remoteAddrStr, &client.remoteAddr.sin_addr);

	int ret; 
	while(client.msg_count_success < MAX_MESAGES)
	{
		const char* message = "Ping"; 
		ret = sendto(client.s, message, sizeof(message), 0, (struct sockaddr*)&client.remoteAddr, sizeof(client.remoteAddr));
	
		if (ret == SOCKET_ERROR)
			printWSErrorAndExit(error);
		else
		{
			const char* msg_received = new char[10];
			msg_received = "";

			char* buf = new char[10];
			int size = sizeof(sockaddr);
			ret = recvfrom(client.s, buf, 10, 0, (struct sockaddr*)&client.remoteAddr, &size);

			if (ret == SOCKET_ERROR)
				printWSErrorAndExit(error);
			else
			{
				client.msg_count_success++;
				printf("Message %d received from server: %s \n", client.msg_count_success, buf);
			}
		}
	}

	CloseSocket(client.s);
	CleanUpSockets();
	system("pause");
}

