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

unsigned char recvdata[2048];
HWAVEOUT hWaveOut; /* device handle */
WAVEFORMATEX wfx; /* look this up in your documentation */
//MMRESULT result;/* for waveOut return values */
unsigned char pcmdata[1024 * 20];
unsigned char *pcmdata_ptr = pcmdata;

static CRITICAL_SECTION waveCriticalSection;
static volatile int waveFreeBlockCount;
static WAVEHDR* waveBlocks;
static int waveCurrentBlock;
#define BLOCK_SIZE 8192
#define BLOCK_COUNT 20
static void CALLBACK waveOutProc(
	HWAVEOUT hWaveOut,
	UINT uMsg,
	DWORD dwInstance,
	DWORD dwParam1,
	DWORD dwParam2
)
{
	/*
	* pointer to free block counter
	*/
	int* freeBlockCounter = (int*)dwInstance;
	/*
	* ignore calls that occur due to openining and closing the
	* device.
	*/
	if (uMsg != WOM_DONE)
		return;
	EnterCriticalSection(&waveCriticalSection);
	(*freeBlockCounter)++;
	LeaveCriticalSection(&waveCriticalSection);
}
WAVEHDR* allocateBlocks(int size, int count)
{
	unsigned char* buffer;
	int i;
	WAVEHDR* blocks;
	DWORD totalBufferSize = (size + sizeof(WAVEHDR)) * count;
	/*
	* allocate memory for the entire set in one go
	*/
	if ((buffer = (unsigned char*)HeapAlloc(
		GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		totalBufferSize
	)) == NULL)
	{
		fprintf(stderr, "Memory allocation error\n");
		ExitProcess(1);
	}
	/*
	* and set up the pointers to each bit
	*/
	blocks = (WAVEHDR*)buffer;
	buffer += sizeof(WAVEHDR) * count;
	for (i = 0; i < count; i++) {
		blocks[i].dwBufferLength = size;
		blocks[i].lpData = (LPSTR)buffer;
		buffer += size;
	}
	return blocks;
}
void freeBlocks(WAVEHDR* blockArray)
{
	/*
	* and this is why allocateBlocks works the way it does
	*/
	HeapFree(GetProcessHeap(), 0, blockArray);
}
void writeAudio(HWAVEOUT hWaveOut, LPSTR data, int size)
{
	WAVEHDR* current;
	int remain;
	current = &waveBlocks[waveCurrentBlock];
	while (size > 0) {
		/*
		* first make sure the header we're going to use is unprepared
		*/
		if (current->dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));
		if (size < (int)(BLOCK_SIZE - current->dwUser)) {
			memcpy(current->lpData + current->dwUser, data, size);
			current->dwUser += size;
			break;
		}
		remain = BLOCK_SIZE - current->dwUser;
		memcpy(current->lpData + current->dwUser, data, remain);
		size -= remain;
		data += remain;
		current->dwBufferLength = BLOCK_SIZE;
		waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));
		waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));
		EnterCriticalSection(&waveCriticalSection);
		waveFreeBlockCount--;
		LeaveCriticalSection(&waveCriticalSection);
		/*
		* wait for a block to become free
		*/
		//while (!waveFreeBlockCount)
		//	Sleep(10);
		/*
		* point to the next block
		*/
		waveCurrentBlock++;
		waveCurrentBlock %= BLOCK_COUNT;
		current = &waveBlocks[waveCurrentBlock];
		current->dwUser = 0;
	}
}
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


	sockaddr_in recvAddr;
	int recvAddrLen = sizeof(recvAddr);

	
	
	waveBlocks = allocateBlocks(BLOCK_SIZE, BLOCK_COUNT);
	waveFreeBlockCount = BLOCK_COUNT;
	waveCurrentBlock = 0;
	InitializeCriticalSection(&waveCriticalSection);
	wfx.nSamplesPerSec = 48000; /* sample rate */
	wfx.wBitsPerSample = 16; /* sample size */
	wfx.nChannels = 1; /* channels*/
	wfx.cbSize = 0; /* size of _extra_ info */
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;


	if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutProc,
		(DWORD_PTR)&waveFreeBlockCount, CALLBACK_NULL) != MMSYSERR_NOERROR) {
		fprintf(stderr, "unable to open WAVE_MAPPER device\n");
	}

	
	OpusDecoder *opus;
	int error;
	opus = opus_decoder_create(48000, 1, &error);

	while(1)
	{
		int ret = recvfrom(serSocket, (char*)recvdata, sizeof(recvdata), 0, (sockaddr *)&recvAddr, &recvAddrLen);
		int recv = opus_decode(opus, recvdata, ret, (opus_int16*)pcmdata_ptr, 2880, 0);
		writeAudio(hWaveOut, (LPSTR)pcmdata_ptr, recv*2);
	}
}