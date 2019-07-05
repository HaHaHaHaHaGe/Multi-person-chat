#pragma once
#include <list>
#include <vector>
#include <iostream> 
#include "OPUS_API.h"
#include "WindowsSpeaker.h"
#include <windows.h> 
#include <algorithm> 
#include <stdexcept> 
#include <time.h>
using namespace std;

//一个对象的所有信息
typedef struct Chater
{
	int ID; //唯一id
	char *name;  //名称（暂未用到）
	int volume;  //音量
	int realtime_am;//峰峰值
	time_t Recent_call_time;//最近的通话时间
	float Offline_time_ms;//离线时间
	bool isOffline;//是否离线

	WindowsSpeaker *spk; //windows放声对象
	opus_int16 *out_buffer;//存储最新的语音数据
}Chater;

class PacketParsing
{
	vector<Chater> chaters;  //Chater的集合

	int buffer_size; //存储每个Chater的buffer大小
	//opus_int16 *buffer1;
	//opus_int16 *buffer2;
	//int select_buffer;
	//bool can_read_data;
	int wave_fs;	//存储设定每个Chater spk的采样率
	//int packeg_time;
	OPUS_API *dec;	//解码器
	

	int self_id;	//存储自己的id，防止自己收到自己的语音数据
	//bool Toggle = false;
	//bool Toggle_last = false;
	int Unpack(unsigned char *data);	//解包，提取出语音包的id与压缩数据
	static DWORD WINAPI Fun(LPVOID lpParamter); //定时清理线程，每个Chater的超时时间
	static bool threadlock;	//线程锁，防止清理线程与其他线程同时访问vector
	HANDLE hThread;		//清理线程的句柄
public:
	void Pack(unsigned char *data,int datalen); //封包，将数据增加id信息（数据自动往后移动一个sizeof（int））


	//解析数据包，输入从RTP传来的原始数据，并进行解包、解码、播放等操作
	void Analysis(unsigned char *data,int len);	
	//设定一个Chater的音量
	void SetChaterVolume(int id, int vol);
	//获取所有的Chater（复制）
	vector<Chater> get_all_chater();

	//opus_int16 *get_fix_data();
	PacketParsing(int blocksize,int fs, int id);
	~PacketParsing();
};

