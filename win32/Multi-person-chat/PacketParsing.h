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

//һ�������������Ϣ
typedef struct Chater
{
	int ID; //Ψһid
	char *name;  //���ƣ���δ�õ���
	int volume;  //����
	int realtime_am;//���ֵ
	time_t Recent_call_time;//�����ͨ��ʱ��
	float Offline_time_ms;//����ʱ��
	bool isOffline;//�Ƿ�����

	WindowsSpeaker *spk; //windows��������
	opus_int16 *out_buffer;//�洢���µ���������
}Chater;

class PacketParsing
{
	vector<Chater> chaters;  //Chater�ļ���

	int buffer_size; //�洢ÿ��Chater��buffer��С
	//opus_int16 *buffer1;
	//opus_int16 *buffer2;
	//int select_buffer;
	//bool can_read_data;
	int wave_fs;	//�洢�趨ÿ��Chater spk�Ĳ�����
	//int packeg_time;
	OPUS_API *dec;	//������
	

	int self_id;	//�洢�Լ���id����ֹ�Լ��յ��Լ�����������
	//bool Toggle = false;
	//bool Toggle_last = false;
	int Unpack(unsigned char *data);	//�������ȡ����������id��ѹ������
	static DWORD WINAPI Fun(LPVOID lpParamter); //��ʱ�����̣߳�ÿ��Chater�ĳ�ʱʱ��
	static bool threadlock;	//�߳�������ֹ�����߳��������߳�ͬʱ����vector
	HANDLE hThread;		//�����̵߳ľ��
public:
	void Pack(unsigned char *data,int datalen); //���������������id��Ϣ�������Զ������ƶ�һ��sizeof��int����


	//�������ݰ��������RTP������ԭʼ���ݣ������н�������롢���ŵȲ���
	void Analysis(unsigned char *data,int len);	
	//�趨һ��Chater������
	void SetChaterVolume(int id, int vol);
	//��ȡ���е�Chater�����ƣ�
	vector<Chater> get_all_chater();

	//opus_int16 *get_fix_data();
	PacketParsing(int blocksize,int fs, int id);
	~PacketParsing();
};

