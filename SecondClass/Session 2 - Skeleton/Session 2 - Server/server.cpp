#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"
#include "WinSock2.h"
#include "Ws2tcpip.h"

#include <list>
#include <cstdlib>
#include <iostream>

// Globals
SOCKET serverSocket; // Listen socket
std::list<SOCKET> sockets; // All connected sockets

void logSocketErrorAndExit(const char *msg)
{
	wchar_t *s = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&s, 0, NULL);
	fprintf(stderr, "%s: %S\n", msg, s);
	LocalFree(s);
	system("pause");
	exit(-1);
}

void handleIncomingData(sockaddr_in adress)
{
	// Input buffer
	const int inputBufferLen = 1300;
	char inputBuffer[inputBufferLen];

	// Create a new socket set
	fd_set readfds;
	FD_ZERO(&readfds);
	// Fill the set
	for (auto s : sockets) {
		FD_SET(s, &readfds);
	}

	// Call select
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int res = select(0, &readfds, nullptr, nullptr, &timeout);
	if (res == SOCKET_ERROR) {
		logSocketErrorAndExit("select 4 read");
	}
	else
		readfds.fd_count++;
	// List to mark disconnected sockets
	std::list<SOCKET> disconnectedSockets;

	struct sockaddr_in serverAddress;

	// Read selected sockets
	for (auto s = sockets.begin(); s != sockets.end(); s++)
	{
		// Check if s is ready for read
		if (FD_ISSET((*s), &readfds)) {

			// Is the server socket 
			if ((*s) == serverSocket) {
				// Accept a new connection
				int size = sizeof(sockaddr);
				SOCKET new_socket = accept((*s), (sockaddr*)&adress, &size);

				if (new_socket == SOCKET_ERROR)
				{
					int lastError = WSAGetLastError();


					if (lastError != WSAEWOULDBLOCK)
					{
						logSocketErrorAndExit("accept");
					}
				}

				// TODO
				// Add the new socket to our list sockets
				else
					sockets.push_back(new_socket);
				// TODO
			}

			// Is a client socket
			else
			{

				// Call recv
				int bytesRecv = recv((*s), inputBuffer, inputBufferLen, 0);
				if (bytesRecv == SOCKET_ERROR)
				{
					int lastError = WSAGetLastError();

					if (lastError != WSAEWOULDBLOCK) {
						// Other error handling
					}

					if (lastError == WSAECONNRESET) {
						disconnectedSockets.push_back((*s));
					}


				}
				else if (bytesRecv == 0) // Success
				{
					disconnectedSockets.push_back((*s));
				}
				else
				{
					printf("%s", inputBuffer);
				}

				// TODO

				// Handle errors
				// - WSAEWOULDBLOCK (do nothing)
				// - WSAECONNRESET (client disconnected forcibly, so insert s into disconnectedSockets)

				// Handle graceful client disconnection (recv returned 0, so insert s into disconnectedSockets)

				// On success, log the received message
			}
		}
	}

	// Remove disconnected sockets
	for (auto s : disconnectedSockets) {
		sockets.remove(s);
	}
}

void handleOutgoingData()
{
	// Output buffer
	const char *outputBuffer = "Update state packet";
	const int outputBufferLen = strlen(outputBuffer) + 1;

	// Create a new socket set
	fd_set sendfds;
	FD_ZERO(&sendfds);
	// Fill the set
	for (auto s : sockets) {
		FD_SET(s, &sendfds);
	}
	// Timeout
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	// Call select
	int res = select(0, nullptr, &sendfds, nullptr, &timeout);
	if (res == SOCKET_ERROR) {
		logSocketErrorAndExit("select 4 send");
	}
	// List to mark disconnected sockets
	std::list<SOCKET> disconnectedSockets;

	// Send data to selected sockets
	for (auto s : sockets)
	{
		if (FD_ISSET(s, &sendfds)) {

			// Send data
			const char *outputBuffer = "Update state packet";
			const int outputBufferLen = strlen(outputBuffer) + 1;
			int bytesSent = send(s, outputBuffer, outputBufferLen, 0);
			if (bytesSent == SOCKET_ERROR) {
				if (WSAGetLastError() == WSAECONNRESET) {
					std::cout << "Client disconnected forcibly" << std::endl;
					disconnectedSockets.push_back(s);
				}
				else {
					logSocketErrorAndExit("send");
				}
			}
		}
	}

	// Remove disconnected sockets
	for (auto s : disconnectedSockets) {
		sockets.remove(s);
	}
}

void server(int port)
{
	// Startup
	WSAData wsaData;
	int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res == SOCKET_ERROR) {
		logSocketErrorAndExit("WSAStartup");
	}

	// Create
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		logSocketErrorAndExit("socket");
	}

	// Add the socket to the Socket list
	sockets.push_back(serverSocket);

	// Reuse address
	int enable = 1;
	res = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&enable, sizeof(int));
	if (res == SOCKET_ERROR) {
		logSocketErrorAndExit("setsockopt(SO_REUSEADDR)");
	}

	// Bind
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;
	res = bind(serverSocket, (const sockaddr*)&serverAddress, sizeof(serverAddress));
	if (res == SOCKET_ERROR) {
		logSocketErrorAndExit("bind");
	}

	// Listen
	res = listen(serverSocket, 32);
	if (res == SOCKET_ERROR) {
		logSocketErrorAndExit("listen");
	}

	// Loop
	while (true)
	{
		handleIncomingData(serverAddress);

		// handleInput();
		// simulatePhysics();
		// simulateAI();
		// ...

		handleOutgoingData();

		// Print iteration
		static int iteration = 0;
		std::cout << "Server iteration " << iteration++ << std::endl;

		// Wait a second
		Sleep(2000);
	}

	// Delete
	closesocket(serverSocket);

	// Cleanup
	res = WSACleanup();
	if (res == SOCKET_ERROR) {
		logSocketErrorAndExit("WSACleanup");
	}
}

int main(int argc, char **argv)
{
	const int port = 8000;
	server(port);
	system("pause");
	return 0;
}
