#pragma once
#include <stdio.h>
#include <string.h>
#include <Ws2tcpip.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "winmm.lib")
//遗弃，采用新的RTP方式
class WindowsSocketUDP
{
	WSADATA wsaData;
	WORD sockVersion;
	SOCKET serSocket;
	sockaddr_in serAddr;
	sockaddr_in remoteAddr;
	sockaddr_in recvAddr;
	int recvAddrLen = sizeof(recvAddr);
	int nAddrLen = sizeof(remoteAddr);
public:
	int SocketUDP_Send(unsigned char *data, int len);
	int SocketUDP_Recv(unsigned char *buffer,int len);
	WindowsSocketUDP(const char* send_ip,int bind_port, int remote_port);
	~WindowsSocketUDP();
};

