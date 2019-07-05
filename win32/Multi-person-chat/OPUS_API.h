#pragma once
#include <stdio.h>
#include <string.h>
#include "opus.h"
#include "opus_types.h"
#include "opus_private.h"
#include "opus_multistream.h"
class OPUS_API
{
	OpusDecoder *opusdec;
	OpusEncoder *opusenc;
	int error;
public:
	//������
	//������
	//data����Ҫ���������
	//out��������������Ŀռ�
	//block_size:��Ϣ���С������鿴opus�ֲ�
	//out_size������ռ�Ĵ�С����ֹ�����
	//����ֵ�������Ĵ�С
	int OPUS_Encode(opus_int16 *data, unsigned char *out, int block_size, int out_size);



	//������
	//������
	//data����Ҫ���������
	//out��������������Ŀռ�
	//block_size:��Ϣ���С������鿴opus�ֲ�
	//out_size������ռ�Ĵ�С����ֹ�����
	//����ֵ�������Ĵ�С
	int OPUS_Decode(unsigned char *data, opus_int16 *out, int block_size, int data_size);
	//ʹ�ú��������������Ǳ��������ǽ�����
	OPUS_API(opus_int32 fs);//������
	OPUS_API(opus_int32 fs,int application);//������
	~OPUS_API();
};

