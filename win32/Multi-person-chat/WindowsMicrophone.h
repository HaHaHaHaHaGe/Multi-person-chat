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
	//windows 录音所需要的变量
	static CRITICAL_SECTION cs;
	static WAVEHDR pwh1;
	static char buffer1[];
	static WAVEHDR pwh2;
	static char buffer2[];
	static HWAVEIN phwi;
	static int init_cycle_buffer();
	static void RecordWave();
	///////////////////////////////////////////
	//环形缓冲器用于存储mic数据
	static int BUFFSIZE;//缓冲器大小
	static cycle_buffer *fifo;//定义全局FIFO
	static unsigned int fifo_getdatalen();
	static unsigned int fifo_get(char *buf, unsigned int len);
	static unsigned int fifo_put(char *buf, unsigned int len);


	//频率与块大小
	static int FS;
	static int BLOCK_SIZE;
	//线程关闭用
	static bool shutdown_mic;

	//音频格式初始化
	static void WaveInitFormat(LPWAVEFORMATEX m_WaveFormat, WORD nCh, DWORD nSampleRate, WORD BitsPerSample);
	//系统录音回调函数
	static DWORD CALLBACK MicCallback(HWAVEIN hwavein, UINT uMsg, DWORD dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

public:
	//获取mic数据，返回值为大小
	static int get_mic_data(unsigned char *out);
	//初始化设备，fs采样率，blocksize块大小
	static void initial_device(int fs,int blocksize);
	//释放所有资源
	static void deinitial_device();
	WindowsMicrophone();
	~WindowsMicrophone();
};

