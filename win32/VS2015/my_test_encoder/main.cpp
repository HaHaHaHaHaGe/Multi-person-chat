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
static struct cycle_buffer *fifo = NULL;//����ȫ��FIFO
//��ʼ�����λ�����
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
unsigned int fifo_getdatalen()  //�ӻ��λ�������ȡ����
{
	return fifo->in - fifo->out;
}

unsigned int fifo_get(char *buf, unsigned int len)  //�ӻ��λ�������ȡ����
{
	unsigned int l;
	len = min(len, fifo->in - fifo->out);
	l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
	memcpy(buf, fifo->buf + (fifo->out & (fifo->size - 1)), l);
	memcpy(buf + l, fifo->buf, len - l);
	fifo->out += len;
	return len;
}

unsigned int fifo_put(char *buf, unsigned int len) //�����ݷ��뻷�λ�����
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
DWORD CALLBACK MicCallback(HWAVEIN hwavein, UINT uMsg, DWORD dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)//�ص����������ݻ���������ʱ��ͻᴥ�����ص�������ִ�������RecordWave����֮���൱�ڴ�����һ���߳�
{
	int len = 0;
	switch (uMsg)
	{
	case WIM_OPEN://���豸ʱ�����֧��ִ�С�
		//printf("\n�豸�Ѿ���...\n");
		break;
	case WIM_DATA://������������ʱ�������֧��ִ�У���Ҫ�������֧�г���������䣬�ᶪ����	��waveform audio����û�л�����ơ�		
		//printf("\n������%d����...\n", ((LPWAVEHDR)dwParam1)->dwUser);
		waveInAddBuffer(hwavein, (LPWAVEHDR)dwParam1, sizeof(WAVEHDR));

		EnterCriticalSection(&cs); //�����ٽ���
		len = fifo_put((char*)(((LPWAVEHDR)dwParam1)->lpData), 2880*2); //��������������д�뻷��fifo
		LeaveCriticalSection(&cs);//�˳��ٽ���
			//fwrite(((LPWAVEHDR)dwParam1)->lpData,10240, 1, fp);
			//printf("lens=%d", len);
		break;
	case WIM_CLOSE:
		//printf("\n�豸�Ѿ��ر�...\n");
		break;
	default:
		break;
	}
	return 0;
}
void WaveInitFormat(LPWAVEFORMATEX m_WaveFormat, WORD nCh, DWORD nSampleRate, WORD BitsPerSample)//��ʼ����Ƶ��ʽ
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
	count = waveInGetNumDevs();//��ȡϵͳ�ж��ٸ�����

	mmResult = waveInGetDevCaps(0, &waveIncaps, sizeof(WAVEINCAPS));//�鿴ϵͳ�����豸����������̫����������������

	printf("\ncount = %d\n", count);
	printf("\nwaveIncaps.szPname=%s\n", waveIncaps.szPname);

	if (MMSYSERR_NOERROR == mmResult)
	{
		WAVEFORMATEX pwfx;
		WaveInitFormat(&pwfx, 1, 48000, 16);
		printf("\n�������Ƶ�����豸");
		printf("\n���������������� 48kHz 16bit\n");
		mmResult = waveInOpen(&phwi, WAVE_MAPPER, &pwfx, (DWORD_PTR)(MicCallback), NULL, CALLBACK_FUNCTION);//����Ƶ�豸�����ûص�����

		if (MMSYSERR_NOERROR == mmResult)
		{
			

			pwh1.lpData = buffer1;
			pwh1.dwBufferLength = 2880*2;
			pwh1.dwUser = 1;
			pwh1.dwFlags = 0;
			mmResult = waveInPrepareHeader(phwi, &pwh1, sizeof(WAVEHDR));//׼��������
			printf("\n׼��������1");

			pwh2.lpData = buffer2;
			pwh2.dwBufferLength = 2880*2;
			pwh2.dwUser = 2;
			pwh2.dwFlags = 0;
			mmResult = waveInPrepareHeader(phwi, &pwh2, sizeof(WAVEHDR));//
			printf("\n׼��������2\n");

			if (MMSYSERR_NOERROR == mmResult)
			{
				mmResult = waveInAddBuffer(phwi, &pwh1, sizeof(WAVEHDR));//��ӻ�����
				mmResult = waveInAddBuffer(phwi, &pwh2, sizeof(WAVEHDR));

				printf("\n��������2������Ƶ�����豸\n");
				if (MMSYSERR_NOERROR == mmResult)
				{
					mmResult = waveInStart(phwi);
					//printf("\n����ʼ¼��\n");
					/*
					Sleep(10000);
					waveInStop(phwi);//ֹͣ¼��
					//waveInReset(phwi);
					waveInClose(phwi);//�ر���Ƶ�豸
					waveInUnprepareHeader(phwi,&pwh1, sizeof(WAVEHDR));//�ͷ�buffer
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




	InitializeCriticalSection(&cs);//��ʼ���ٽ���
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
			EnterCriticalSection(&cs); //�����ٽ���			
			int len = fifo_get((char*)buff, 2880*2);//��fifo�л�ȡ����			
			LeaveCriticalSection(&cs);//�뿪�ٽ���
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