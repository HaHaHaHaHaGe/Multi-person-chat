#include "WindowsMicrophone.h"
#include "WindowsSpeaker.h"
#include "WindowsSocketUDP.h"
#include "OPUS_API.h"
#include "PacketParsing.h"
#include <iostream>
#include "rtpsession.h"
#include "rtppacket.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtpmemorymanager.h"

using namespace jrtplib;
using namespace std;
unsigned char *mic_data = new unsigned char[1024 * 10];			//存放麦克风原始数据的缓存
unsigned char *mic_encode_data = new unsigned char[1024 * 10];	//存放麦克风压缩编码后数据的缓存
//unsigned char *data3 = new unsigned char[1024 * 10];
WindowsMicrophone mic;
//WindowsSocketUDP u1("192.168.3.255", 8888, 8888);
//WindowsSocketUDP u2("192.168.3.255", 8889, 8888);
//OPUS_API dec(424000);
OPUS_API *enc;
PacketParsing *pak;
RTPSession *sess;
WSADATA dat;
uint32_t destip;
std::string ipstr;
int status, i, num;
RTPUDPv4TransmissionParams transparams;
RTPSessionParams sessparams;
RTPIPv4Address *addr;

bool shutdown_thread = false;
DWORD __stdcall Fun2(LPVOID lpParamter)
{
	while (!shutdown_thread)
	{
		int recv = mic.get_mic_data(mic_data);
		if (recv != 0)
		{
			int enclen = enc->OPUS_Encode((opus_int16*)mic_data, mic_encode_data, 2880, 102880);
			pak->Pack(mic_encode_data, enclen);
			sess->SendPacket((void *)mic_encode_data, enclen, 0, false, 10);
			
			//u1.SocketUDP_Send(data2, enclen);
		}
		//vector<Chater>::iterator iv;
		//vector<Chater> c = pak->get_all_chater();
		//system("cls");
		//for (iv = c.begin(); iv != c.end(); ++iv)
		//{
		//	cout << (*iv).ID << endl;
		//}
	}
	return 0;
}



DWORD __stdcall Fun(LPVOID lpParamter)
{
	while (!shutdown_thread)
	{
		sess->BeginDataAccess();

		// check incoming packets
		if (sess->GotoFirstSourceWithData())
		{
			do
			{
				RTPPacket *pack;

				while ((pack = sess->GetNextPacket()) != NULL)
				{
					// You can examine the data here
					//char * s = (char*)pack->GetPayloadData();
					pak->Analysis(pack->GetPayloadData(), pack->GetPayloadLength());
					//printf("Got packet !%s\n", s);

					// we don't longer need the packet, so
					// we'll delete it
					sess->DeletePacket(pack);
				}
			} while (sess->GotoNextSourceWithData());
		}

		sess->EndDataAccess();

#ifndef RTP_SUPPORT_THREAD
		sess->Poll();
		//checkerror(status);
#endif // RTP_SUPPORT_THREAD
		//if (recv1 > 0)
			//pak->Analysis(data3, recv1);
	}
	return 0;
}
HANDLE hThread;
HANDLE hThread2;
//释放所有资源
extern "C" __declspec(dllexport) void deinitial_muti_chat()
{
	shutdown_thread = true;
	//TerminateThread(hThread2,0);
	//TerminateThread(hThread, 0);
	mic.deinitial_device();
	sess->BYEDestroy(RTPTime(10, 0), 0, 0);
	delete pak;
	delete addr;
	delete enc;
	delete sess;
}

//初始化
extern "C" __declspec(dllexport) void initial_muti_chat(char* ip, int port, int id)
{
	shutdown_thread = false;//线程主循环条件
	enc = new OPUS_API(24000, OPUS_APPLICATION_VOIP);	//初始化编码器对象

	//////////////////////////////////////////////////
	//RTP协议栈初始化
	WSAStartup(MAKEWORD(2, 2), &dat);	
	destip = inet_addr(ip);
	destip = ntohl(destip);
	addr = new RTPIPv4Address(destip, port);
	sess = new RTPSession;
	sessparams.SetOwnTimestampUnit(1.0 / 10.0);
	sessparams.SetAcceptOwnPackets(true);
	transparams.SetPortbase(port);
	status = sess->Create(sessparams, &transparams);
	status = sess->AddDestination(*addr);
	//////////////////////////////////////////////////

	pak = new  PacketParsing(2880, 24000, id); //用于数据包封装、解封装、解码、播放一体的对象
	mic.initial_device(24000, 2880);		//初始化麦克风
	hThread = CreateThread(NULL, 0, Fun, NULL, 0, NULL);	//创建一个用于接收udp广播数据的线程
	hThread2 = CreateThread(NULL, 0, Fun2, NULL, 0, NULL);  //创建一个用于发送本机麦克风压缩数据的线程
}
//改变某个id对象的音量
extern "C" __declspec(dllexport) void set_chater_vol(int id, int vol)
{
	pak->SetChaterVolume(id,vol);
}

//获取已存在（离线与在线）的所有对象信息
//参数：
//id:对象的唯一id
//isOffline:该对象是否离线
//Offline_time_ms：离线时间单位ms
//realtime_am：语音信息峰峰值
//volume：音量
extern "C" __declspec(dllexport) int get_chaters_info(int *id,bool *isOffline,int *Offline_time_ms,int *realtime_am,int *volume)
{
	vector<Chater>::iterator iv;
	vector<Chater> chaters =  pak->get_all_chater();
	int i = 0;
	for (iv = chaters.begin(); iv != chaters.end(); ++iv)
	{
		id[i] = (*iv).ID;
		isOffline[i] = (*iv).isOffline;
		//(*iv).name;
		Offline_time_ms[i] = (*iv).Offline_time_ms;
		realtime_am[i] = (*iv).realtime_am;
		volume[i] = (*iv).volume;
		i++;
	}
	return i;
}
//int main()
//{
//	int test1[100];
//	int test2[100];
//	int test3[100];
//	int test4[100];
//	bool a[100];
//
//	while (1)
//	{
//		initial_muti_chat((char*)"192.168.3.255", 8888, 1234);
//		Sleep(200);
//		deinitial_muti_chat();
//	}
//	//{
//	//	int aa = get_chaters_info(test1, a, test2, test3, test4);
//	//	system("cls");
//	//	for (int i = 0; i < aa; i++)
//	//	{
//	//		cout << test1[i] << "    " << a[i] << "    " << test2[i] << "    " << test3[i] << "    " << test4[i] << endl;
//	//	}
//	//	Sleep(50);
//	//}
//}
