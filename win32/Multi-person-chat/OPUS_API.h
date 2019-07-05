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
	//编码器
	//参数：
	//data：需要编码的数据
	//out：用于输出编码后的空间
	//block_size:信息块大小，具体查看opus手册
	//out_size：输出空间的大小（防止溢出）
	//返回值：编码后的大小
	int OPUS_Encode(opus_int16 *data, unsigned char *out, int block_size, int out_size);



	//解码器
	//参数：
	//data：需要解码的数据
	//out：用于输出解码后的空间
	//block_size:信息块大小，具体查看opus手册
	//out_size：输出空间的大小（防止溢出）
	//返回值：解码后的大小
	int OPUS_Decode(unsigned char *data, opus_int16 *out, int block_size, int data_size);
	//使用函数重载来决定是编码器还是解码器
	OPUS_API(opus_int32 fs);//解码器
	OPUS_API(opus_int32 fs,int application);//编码器
	~OPUS_API();
};

