#pragma once
#include <stdio.h>
#include <string.h>
#include <Ws2tcpip.h>
#include <winsock2.h>
#pragma comment(lib, "winmm.lib")
class WindowsSpeaker
{
#define BLOCK_SIZE 8192
#define BLOCK_COUNT 20
	bool shutdown_thread;
	unsigned char recvdata[2048];
	HWAVEOUT hWaveOut; /* device handle */
	WAVEFORMATEX wfx; /* look this up in your documentation */
	//MMRESULT result;/* for waveOut return values */
	unsigned char pcmdata[1024 * 20];

	CRITICAL_SECTION waveCriticalSection;
	volatile int waveFreeBlockCount;
	WAVEHDR* waveBlocks;
	int waveCurrentBlock;
	static void CALLBACK waveOutProc(
		HWAVEOUT hWaveOut,
		UINT uMsg,
		DWORD dwInstance,
		DWORD dwParam1,
		DWORD dwParam2
	);
	WAVEHDR* allocateBlocks(int size, int count);
	void freeBlocks(WAVEHDR* blockArray);
	void writeAudio(HWAVEOUT hWaveOut, LPSTR data, int size);
public:
	//����������in����ԭʼ���ݣ�len���ݳ���
	void set_speak_data(unsigned char *in,int len);
	//��ʼ���豸��fs������
	void initial_device(int fs);
	//�ͷ�������Դ
	void deinitial_device();
	WindowsSpeaker();
	~WindowsSpeaker();
};

