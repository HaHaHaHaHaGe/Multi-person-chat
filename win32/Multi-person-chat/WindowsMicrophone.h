#pragma once
#include <stdio.h>
#include <string.h>
#include <Ws2tcpip.h>
#include <winsock2.h>
#pragma comment(lib, "winmm.lib")
typedef struct cycle_buffer {
	char *buf;
	unsigned int size;
	unsigned int in;
	unsigned int out;
}cycle_buffer;
class WindowsMicrophone
{
	//////////////////////////////////////////////
	//windows ¼������Ҫ�ı���
	static CRITICAL_SECTION cs;
	static WAVEHDR pwh1;
	static char buffer1[];
	static WAVEHDR pwh2;
	static char buffer2[];
	static HWAVEIN phwi;
	static int init_cycle_buffer();
	static void RecordWave();
	///////////////////////////////////////////
	//���λ��������ڴ洢mic����
	static int BUFFSIZE;//��������С
	static cycle_buffer *fifo;//����ȫ��FIFO
	static unsigned int fifo_getdatalen();
	static unsigned int fifo_get(char *buf, unsigned int len);
	static unsigned int fifo_put(char *buf, unsigned int len);


	//Ƶ������С
	static int FS;
	static int BLOCK_SIZE;
	//�̹߳ر���
	static bool shutdown_mic;

	//��Ƶ��ʽ��ʼ��
	static void WaveInitFormat(LPWAVEFORMATEX m_WaveFormat, WORD nCh, DWORD nSampleRate, WORD BitsPerSample);
	//ϵͳ¼���ص�����
	static DWORD CALLBACK MicCallback(HWAVEIN hwavein, UINT uMsg, DWORD dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

public:
	//��ȡmic���ݣ�����ֵΪ��С
	static int get_mic_data(unsigned char *out);
	//��ʼ���豸��fs�����ʣ�blocksize���С
	static void initial_device(int fs,int blocksize);
	//�ͷ�������Դ
	static void deinitial_device();
	WindowsMicrophone();
	~WindowsMicrophone();
};

