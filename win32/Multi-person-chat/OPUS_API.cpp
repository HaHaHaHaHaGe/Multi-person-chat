#include "OPUS_API.h"



OPUS_API::OPUS_API(opus_int32 fs, int application)
{
	opusenc = opus_encoder_create(fs, 1, application, &error);
	opusdec = NULL;
}


int OPUS_API::OPUS_Encode(opus_int16 * data, unsigned char * out, int block_size, int out_size)
{
	return opus_encode(opusenc, data, block_size, out, out_size);
}

int OPUS_API::OPUS_Decode(unsigned char * data, opus_int16 * out, int block_size, int data_size)
{
	return opus_decode(opusdec, data, data_size, out, block_size, 0);
}

OPUS_API::OPUS_API(opus_int32 fs)
{
	opusdec = opus_decoder_create(fs, 1, &error);
	opusenc = NULL;
}

OPUS_API::~OPUS_API()
{
	if(opusenc != NULL)
		opus_encoder_destroy(opusenc);
	else
		opus_decoder_destroy(opusdec);
}
