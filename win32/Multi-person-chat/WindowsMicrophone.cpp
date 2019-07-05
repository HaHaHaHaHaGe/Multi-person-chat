#include "WindowsMicrophone.h"

char WindowsMicrophone::buffer1[10240] = { 0 };
char WindowsMicrophone::buffer2[10240] = { 0 };
WAVEHDR WindowsMicrophone::pwh1;
WAVEHDR WindowsMicrophone::pwh2;
int WindowsMicrophone::BUFFSIZE = 1024 * 1024;
cycle_buffer *WindowsMicrophone::fifo = NULL;
CRITICAL_SECTION WindowsMicrophone::cs;
int WindowsMicrophone::BLOCK_SIZE;
int WindowsMicrophone::FS;
HWAVEIN WindowsMicrophone::phwi;
bool WindowsMicrophone::shutdown_mic;
int WindowsMicrophone::init_cycle_buffer()
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

void WindowsMicrophone::RecordWave()
{
	
	WAVEINCAPS waveIncaps;
	int count = 0;
	MMRESULT mmResult;
	count = waveInGetNumDevs();//获取系统有多少个声卡

	mmResult = waveInGetDevCaps(0, &waveIncaps, sizeof(WAVEINCAPS));//查看系统声卡设备参数，不用太在意这两个函数。

	//printf("\ncount = %d\n", count);
	//printf("\nwaveIncaps.szPname=%s\n", waveIncaps.szPname);

	if (MMSYSERR_NOERROR == mmResult)
	{
		WAVEFORMATEX pwfx;
		WaveInitFormat(&pwfx, 1, FS, 16);
		//printf("\n请求打开音频输入设备");
		//printf("\n采样参数：单声道 48kHz 16bit\n");
		mmResult = waveInOpen(&phwi, WAVE_MAPPER, &pwfx, (DWORD_PTR)(WindowsMicrophone::MicCallback), NULL, CALLBACK_FUNCTION);//打开音频设备，设置回调函数

		if (MMSYSERR_NOERROR == mmResult)
		{


			pwh1.lpData = buffer1;
			pwh1.dwBufferLength = BLOCK_SIZE * 2;
			pwh1.dwUser = 1;
			pwh1.dwFlags = 0;
			mmResult = waveInPrepareHeader(phwi, &pwh1, sizeof(WAVEHDR));//准备缓冲区
			//printf("\n准备缓冲区1");

			pwh2.lpData = buffer2;
			pwh2.dwBufferLength = BLOCK_SIZE * 2;
			pwh2.dwUser = 2;
			pwh2.dwFlags = 0;
			mmResult = waveInPrepareHeader(phwi, &pwh2, sizeof(WAVEHDR));//
			//printf("\n准备缓冲区2\n");

			if (MMSYSERR_NOERROR == mmResult)
			{
				mmResult = waveInAddBuffer(phwi, &pwh1, sizeof(WAVEHDR));//添加缓冲区
				mmResult = waveInAddBuffer(phwi, &pwh2, sizeof(WAVEHDR));

				//printf("\n将缓冲区2加入音频输入设备\n");
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

unsigned int WindowsMicrophone::fifo_getdatalen()
{
	return fifo->in - fifo->out;
}

unsigned int WindowsMicrophone::fifo_get(char * buf, unsigned int len)
{
	unsigned int l;
	len = min(len, fifo->in - fifo->out);
	l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
	memcpy(buf, fifo->buf + (fifo->out & (fifo->size - 1)), l);
	memcpy(buf + l, fifo->buf, len - l);
	fifo->out += len;
	return len;
}

unsigned int WindowsMicrophone::fifo_put(char * buf, unsigned int len)
{
	unsigned int l;
	len = min(len, fifo->size - fifo->in + fifo->out);
	l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));
	memcpy(fifo->buf + (fifo->in & (fifo->size - 1)), buf, l);
	memcpy(fifo->buf, buf + l, len - l);
	fifo->in += len;
	return len;
}

void WindowsMicrophone::WaveInitFormat(LPWAVEFORMATEX m_WaveFormat, WORD nCh, DWORD nSampleRate, WORD BitsPerSample)
{
	m_WaveFormat->wFormatTag = WAVE_FORMAT_PCM;
	m_WaveFormat->nChannels = nCh;
	m_WaveFormat->nSamplesPerSec = nSampleRate;
	m_WaveFormat->nAvgBytesPerSec = nSampleRate * nCh * BitsPerSample / 8;
	m_WaveFormat->nBlockAlign = m_WaveFormat->nChannels * BitsPerSample / 8;
	m_WaveFormat->wBitsPerSample = BitsPerSample;
	m_WaveFormat->cbSize = 0;
}

DWORD WindowsMicrophone::MicCallback(HWAVEIN hwavein, UINT uMsg, DWORD dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	int len = 0;
	if (shutdown_mic)
		return 0;
	switch (uMsg)
	{
	case WIM_OPEN://打开设备时这个分支会执行。
		//printf("\n设备已经打开...\n");
		break;
	case WIM_DATA://当缓冲区满的时候这个分支会执行，不要再这个分支中出现阻塞语句，会丢数据	，waveform audio本身没有缓冲机制。		
		//printf("\n缓冲区%d存满...\n", ((LPWAVEHDR)dwParam1)->dwUser);
		waveInAddBuffer(hwavein, (LPWAVEHDR)dwParam1, sizeof(WAVEHDR));

		EnterCriticalSection(&cs); //进入临界区
		len = fifo_put((char*)(((LPWAVEHDR)dwParam1)->lpData), BLOCK_SIZE * 2); //将缓冲区的数据写入环形fifo
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

int WindowsMicrophone::get_mic_data(unsigned char *out)
{
	if (fifo_getdatalen() > BLOCK_SIZE * 2)
	{
		EnterCriticalSection(&cs); //进入临界区			
		int len = fifo_get((char*)out, BLOCK_SIZE * 2);//从fifo中获取数据			
		LeaveCriticalSection(&cs);//离开临界区
		return len;
	}
	return 0;
}

void WindowsMicrophone::initial_device(int fs, int blocksize)
{
	shutdown_mic = false;
	FS = fs;
	BLOCK_SIZE = blocksize;
	InitializeCriticalSection(&cs);//初始化临界区
	init_cycle_buffer();
	RecordWave();
}

void WindowsMicrophone::deinitial_device()
{
	shutdown_mic = true;
	Sleep(500);
	waveInReset(phwi);
	waveInClose(phwi);
	free(fifo);
	waveInUnprepareHeader(phwi,&pwh1, sizeof(WAVEHDR));
	waveInUnprepareHeader(phwi, &pwh2, sizeof(WAVEHDR));
}

WindowsMicrophone::WindowsMicrophone()
{
	
}


WindowsMicrophone::~WindowsMicrophone()
{
}
