#include <stdio.h>
#include <string.h>
#include "opus.h"
#include "opus_types.h"
#include "opus_private.h"
#include "opus_multistream.h"
#include <Ws2tcpip.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#define BUFFSIZE 1024 * 1024
struct cycle_buffer {
	char *buf;
	unsigned int size;
	unsigned int in;
	unsigned int out;
};
static struct cycle_buffer *fifo = NULL;//定义全局FIFO
//初始化环形缓冲区
static int init_cycle_buffer(void)
{
	int size = BUFFSIZE, ret;

	ret = size & (size - 1);
	if (ret)
		return ret;
	fifo = (struct cycle_buffer *) malloc(sizeof(struct cycle_buffer));
	if (!fifo)
		return -1;

	memset(fifo, 0, sizeof(struct cycle_buffer));
	fifo->size = size;
	fifo->in = fifo->out = 0;
	fifo->buf = (char *)malloc(size);
	if (!fifo->buf)
		free(fifo);
	else
		memset(fifo->buf, 0, size);
	return 0;
}
unsigned int fifo_getdatalen()  //从环形缓冲区中取数据
{
	return fifo->in - fifo->out;
}

unsigned int fifo_get(char *buf, unsigned int len)  //从环形缓冲区中取数据
{
	unsigned int l;
	len = min(len, fifo->in - fifo->out);
	l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
	memcpy(buf, fifo->buf + (fifo->out & (fifo->size - 1)), l);
	memcpy(buf + l, fifo->buf, len - l);
	fifo->out += len;
	return len;
}

unsigned int fifo_put(char *buf, unsigned int len) //将数据放入环形缓冲区
{
	unsigned int l;
	len = min(len, fifo->size - fifo->in + fifo->out);
	l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));
	memcpy(fifo->buf + (fifo->in & (fifo->size - 1)), buf, l);
	memcpy(fifo->buf, buf + l, len - l);
	fifo->in += len;
	return len;
}
CRITICAL_SECTION cs;
DWORD CALLBACK MicCallback(HWAVEIN hwavein, UINT uMsg, DWORD dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)//回调函数当数据缓冲区慢的时候就会触发，回调函数，执行下面的RecordWave函数之后相当于创建了一个线程
{
	int len = 0;
	switch (uMsg)
	{
	case WIM_OPEN://打开设备时这个分支会执行。
		//printf("\n设备已经打开...\n");
		break;
	case WIM_DATA://当缓冲区满的时候这个分支会执行，不要再这个分支中出现阻塞语句，会丢数据	，waveform audio本身没有缓冲机制。		
		//printf("\n缓冲区%d存满...\n", ((LPWAVEHDR)dwParam1)->dwUser);
		waveInAddBuffer(hwavein, (LPWAVEHDR)dwParam1, sizeof(WAVEHDR));

		EnterCriticalSection(&cs); //进入临界区
		len = fifo_put((char*)(((LPWAVEHDR)dwParam1)->lpData), 2880*2); //将缓冲区的数据写入环形fifo
		LeaveCriticalSection(&cs);//退出临界区
			//fwrite(((LPWAVEHDR)dwParam1)->lpData,10240, 1, fp);
			//printf("lens=%d", len);
		break;
	case WIM_CLOSE:
		//printf("\n设备已经关闭...\n");
		break;
	default:
		break;
	}
	return 0;
}
void WaveInitFormat(LPWAVEFORMATEX m_WaveFormat, WORD nCh, DWORD nSampleRate, WORD BitsPerSample)//初始化音频格式
{
	m_WaveFormat->wFormatTag = WAVE_FORMAT_PCM;
	m_WaveFormat->nChannels = nCh;
	m_WaveFormat->nSamplesPerSec = nSampleRate;
	m_WaveFormat->nAvgBytesPerSec = nSampleRate * nCh * BitsPerSample / 8;
	m_WaveFormat->nBlockAlign = m_WaveFormat->nChannels * BitsPerSample / 8;
	m_WaveFormat->wBitsPerSample = BitsPerSample;
	m_WaveFormat->cbSize = 0;
}
WAVEHDR pwh1;
char buffer1[10240];
WAVEHDR pwh2;
char buffer2[10240];
void RecordWave()
{

	HWAVEIN phwi;
	WAVEINCAPS waveIncaps;
	int count = 0;
	MMRESULT mmResult;
	count = waveInGetNumDevs();//获取系统有多少个声卡

	mmResult = waveInGetDevCaps(0, &waveIncaps, sizeof(WAVEINCAPS));//查看系统声卡设备参数，不用太在意这两个函数。

	printf("\ncount = %d\n", count);
	printf("\nwaveIncaps.szPname=%s\n", waveIncaps.szPname);

	if (MMSYSERR_NOERROR == mmResult)
	{
		WAVEFORMATEX pwfx;
		WaveInitFormat(&pwfx, 1, 48000, 16);
		printf("\n请求打开音频输入设备");
		printf("\n采样参数：单声道 48kHz 16bit\n");
		mmResult = waveInOpen(&phwi, WAVE_MAPPER, &pwfx, (DWORD_PTR)(MicCallback), NULL, CALLBACK_FUNCTION);//打开音频设备，设置回调函数

		if (MMSYSERR_NOERROR == mmResult)
		{
			

			pwh1.lpData = buffer1;
			pwh1.dwBufferLength = 2880*2;
			pwh1.dwUser = 1;
			pwh1.dwFlags = 0;
			mmResult = waveInPrepareHeader(phwi, &pwh1, sizeof(WAVEHDR));//准备缓冲区
			printf("\n准备缓冲区1");

			pwh2.lpData = buffer2;
			pwh2.dwBufferLength = 2880*2;
			pwh2.dwUser = 2;
			pwh2.dwFlags = 0;
			mmResult = waveInPrepareHeader(phwi, &pwh2, sizeof(WAVEHDR));//
			printf("\n准备缓冲区2\n");

			if (MMSYSERR_NOERROR == mmResult)
			{
				mmResult = waveInAddBuffer(phwi, &pwh1, sizeof(WAVEHDR));//添加缓冲区
				mmResult = waveInAddBuffer(phwi, &pwh2, sizeof(WAVEHDR));

				printf("\n将缓冲区2加入音频输入设备\n");
				if (MMSYSERR_NOERROR == mmResult)
				{
					mmResult = waveInStart(phwi);
					//printf("\n请求开始录音\n");
					/*
					Sleep(10000);
					waveInStop(phwi);//停止录音
					//waveInReset(phwi);
					waveInClose(phwi);//关闭音频设备
					waveInUnprepareHeader(phwi,&pwh1, sizeof(WAVEHDR));//释放buffer
					waveInUnprepareHeader(phwi,&pwh2, sizeof(WAVEHDR));
					printf("stop capture!\n");
					fflush(stdout);
					*/
				}
			}
		}
	}
}
unsigned char out[1024 * 1024];
OpusEncoder * opus;
int main()
{
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return 0;
	}
	
	SOCKET serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serSocket == INVALID_SOCKET)
	{
		printf("socket error !");
		return 0;
	}
	
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(8888);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(serSocket, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
		{
		 printf("bind error !");
		closesocket(serSocket);
		return 0;
	}

	sockaddr_in remoteAddr;
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons(8888);
	struct in_addr s;
	inet_pton(AF_INET, "192.168.3.255", &s);
	remoteAddr.sin_addr = s;
	int nAddrLen = sizeof(remoteAddr);




	InitializeCriticalSection(&cs);//初始化临界区
	init_cycle_buffer();
	RecordWave();




	int error;
	opus = opus_encoder_create(48000, 1, OPUS_APPLICATION_AUDIO, &error);


	opus_int16 buff[10240] = { 0 };
	int count = 200;
	int recv2 = 0;
	while (count)
	{
		if (fifo_getdatalen() > 2880*2)
		{
			EnterCriticalSection(&cs); //进入临界区			
			int len = fifo_get((char*)buff, 2880*2);//从fifo中获取数据			
			LeaveCriticalSection(&cs);//离开临界区
			int recv = opus_encode(opus, buff, 2880, out, sizeof(out));
			if (recv > 0)
			{
				recv2 = sendto(serSocket, (const char *)out, recv, 0, (sockaddr *)&remoteAddr, nAddrLen);
				if (recv2 < 0)
				{
					
					printf("error ! %d\n", GetLastError());
				}
			}
			//count--;
		}
	}

	opus_encoder_destroy(opus);
}