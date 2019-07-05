#include "WindowsSocketUDP.h"





int WindowsSocketUDP::SocketUDP_Send(unsigned char * data, int len)
{

	return sendto(serSocket, (const char *)data, len, 0, (sockaddr *)&remoteAddr, nAddrLen);
}

int WindowsSocketUDP::SocketUDP_Recv(unsigned char * buffer, int len)
{
	return recvfrom(serSocket, (char*)buffer, len, 0, (sockaddr *)&recvAddr, &recvAddrLen);
}

WindowsSocketUDP::WindowsSocketUDP(const char * send_ip, int bind_port,int remote_port)
{
	sockVersion = MAKEWORD(2, 2);
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return;
	}

	serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serSocket == INVALID_SOCKET)
	{
		return;
	}

	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(bind_port);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(serSocket, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		closesocket(serSocket);
		return;
	}


	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons(remote_port);
	struct in_addr s;
	inet_pton(AF_INET, send_ip, &s);
	remoteAddr.sin_addr = s;
	nAddrLen = sizeof(remoteAddr);
	recvAddrLen = sizeof(recvAddr);
}

WindowsSocketUDP::~WindowsSocketUDP()
{
}
