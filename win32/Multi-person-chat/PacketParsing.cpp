#include "PacketParsing.h"



bool PacketParsing::threadlock = true;


int PacketParsing::Unpack(unsigned char * data)
{
	//因为id保存在前面，所以直接强制转换就可以得到id
	return *(int*)data;
}

DWORD __stdcall PacketParsing::Fun(LPVOID lpParamter)
{
	PacketParsing *p = (PacketParsing*)lpParamter;
	vector<Chater>::iterator iv;
	while (1)
	{
		threadlock = true;
		//遍历vector如果Offline_time_ms超过3000ms则视为离线
		for (iv = p->chaters.begin(); iv != p->chaters.end(); ++iv)
		{
			(*iv).Offline_time_ms += 1000;
			if ((*iv).Offline_time_ms > 3000)
				(*iv).isOffline = true;
		}
		threadlock = false;
		Sleep(1000);
		//MSleep(p->packeg_time*1);
	}
}

//void PacketParsing::MSleep(long lTime)
//{
//	LARGE_INTEGER litmp;
//	LONGLONG QPart1, QPart2;
//	double dfMinus, dfFreq, dfTim, dfSpec;
//	QueryPerformanceFrequency(&litmp);
//	dfFreq = (double)litmp.QuadPart;
//	QueryPerformanceCounter(&litmp);
//	QPart1 = litmp.QuadPart;
//	dfSpec = 0.000001*lTime;
//
//	do
//	{
//		QueryPerformanceCounter(&litmp);
//		QPart2 = litmp.QuadPart;
//		dfMinus = (double)(QPart2 - QPart1);
//		dfTim = dfMinus / dfFreq;
//	} while (dfTim < dfSpec);
//}

void PacketParsing::Pack(unsigned char * data, int datalen)
{
	//将所有数据往后移动一个sizeof(int)
	memcpy(data + sizeof(int),data, datalen);
	//保存id
	*(int*)data = self_id;
}

void PacketParsing::Analysis(unsigned char * data, int len)
{
	vector<Chater>::iterator iv;
	
	bool find = false;
	int ID = Unpack(data);
	//如果该数据包的id是自己发出来的，则直接退出
	if (ID == self_id)
		return;
	//Toggle = false;
	//等待锁
	while (threadlock);
	for (iv = chaters.begin(); iv != chaters.end(); ++iv)
	{
		//从所有的对象中查找id
		if ((*iv).ID == ID)
		{
			//找到的话更新信息，并将标志位find置为true
			find = true;
			(*iv).isOffline = false;
			(*iv).Offline_time_ms = 0;
			(*iv).Recent_call_time = time(NULL);
			//解码数据
			int len2 = dec->OPUS_Decode((unsigned char*)(data + sizeof(int)), (*iv).out_buffer, buffer_size, len - sizeof(int));
			(*iv).realtime_am = 0;
			//计算真实音量、刷新峰峰值
			for (int i = 0; i < len2; i++)
			{
				(*iv).out_buffer[i] = (*iv).out_buffer[i] * ((*iv).volume / 100.0);
				(*iv).realtime_am += abs((*iv).out_buffer[i]);
			}
			(*iv).realtime_am = (*iv).realtime_am / len2;
			//播放
			(*iv).spk->set_speak_data((unsigned char *)(*iv).out_buffer, len2 * 2);
			//struct tm *local;
			//local=localtime(&(*iv).Recent_call_time);
			return;
		}
	}
	//如果没有找到对象则创建一个
	if (find == false)
	{
		Chater c;
		c.ID = ID;
		c.isOffline = false;
		//c->name
		c.Offline_time_ms = 0;
		c.realtime_am = 0;
		c.Recent_call_time = time(NULL);
		c.volume = 100;
		
		chaters.push_back(c);
		chaters.back().spk = new WindowsSpeaker;
		chaters.back().spk->initial_device(wave_fs);
		chaters.back().out_buffer = new opus_int16[buffer_size];
	}

	//解码播放
	dec->OPUS_Decode((unsigned char*)(data + sizeof(int)), chaters.back().out_buffer, buffer_size, len - sizeof(int));
	chaters.back().spk->set_speak_data((unsigned char *)chaters.back().out_buffer, buffer_size * 2);
	//can_read_data = false;
	//if (select_buffer == 1) {
	/*if(Toggle){
		for (int i = 0; i < buffer_size; i++)
			buffer2[i] = out_buffer[i];
	}
	else {
		for (int i = 0; i < buffer_size; i++)
			buffer1[i] = out_buffer[i];
	}

	if (Toggle_last == true && Toggle == false)
		memset(buffer2, 0, buffer_size * 2);
	if (Toggle_last == false && Toggle == true)
		memset(buffer1, 0, buffer_size * 2);

	Toggle_last = Toggle;*/
	//can_read_data = true;
}

void PacketParsing::SetChaterVolume(int id, int vol)
{
	vector<Chater>::iterator iv;
	while (threadlock);
	for (iv = chaters.begin(); iv != chaters.end(); ++iv)
	{
		//寻找与id相同的对象，并更新音量
		if ((*iv).ID == id)
		{
			(*iv).volume = vol;
			return;
		}
	}
}
//获取所有的Chater
vector<Chater> PacketParsing::get_all_chater()
{
	while (threadlock);
	return vector<Chater>(chaters);
}

//opus_int16 * PacketParsing::get_fix_data()
//{
//	//if (Toggle_last == true && Toggle == true)
//	//	return buffer2;
//	//if (Toggle_last == false && Toggle == false)
//	//	return buffer1;
//	//if (can_read_data)
//	/*{
//		can_read_data = false;
//		if (select_buffer == 1)
//			return buffer1;
//		else
//			return buffer2;
//	}*/
//	return nullptr;
//}
//构造函数初始化所有信息
PacketParsing::PacketParsing(int blocksize,int fs,int id)
{
	wave_fs = fs;
	self_id = id;
	//packeg_time = (1.0 / fs) * blocksize * 1000;
	buffer_size = blocksize;
	//buffer1 = new opus_int16[blocksize];
	//buffer2 = new opus_int16[blocksize];
	
	dec = new OPUS_API(fs);
	hThread = CreateThread(NULL, 0, Fun, this, 0, NULL);
}
//析构函数释放所有资源
PacketParsing::~PacketParsing()
{
	TerminateThread(hThread, 0);
	vector<Chater>::iterator iv;
	for (iv = chaters.begin(); iv != chaters.end(); ++iv)
	{
		delete (*iv).out_buffer;
		(*iv).spk->deinitial_device();
		delete (*iv).spk;
	}
	delete dec;
	chaters.clear();
}
