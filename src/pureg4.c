/*
BSD 2-Clause License

Copyright (c) 2023, Mikhail Morozov

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "pureg4.h"

#include <string.h>

typedef struct {
	uint8_t *buf, *curptr;
	size_t buf_size;
	size_t byte_offset;
	size_t bit_offset;
} array_t;

static void ArrayInit(uint8_t *buf, size_t buf_size, array_t *array);
static size_t ArrayFullOffset(array_t *array);
static uint32_t ArrayReadToUInt32(array_t *array, size_t bits);

static const size_t pureg4_mmrheader_size = 8;
static const uint32_t pureg4_mmrheader_sign = 0x4d4d52;

size_t pureg4GetDecodedImageSize(uint16_t width, uint16_t height)
{
	if(SIZE_MAX / width < height)
		return 0;
	
	return (size_t)width*(size_t)height;
}

size_t pureg4GetEncodedImageSize(uint16_t width, uint16_t height)
{
	size_t body_size;
	
	if(SIZE_MAX / width < height)
		return 0;

	body_size = (size_t)width*(size_t)height;
	
	if(SIZE_MAX - pureg4_mmrheader_size < body_size) return 0;
	
	return body_size+pureg4_mmrheader_size;
}

size_t pureg4DecodeMMRHeader(uint8_t *buf, size_t buf_size, pureg4_mmrheader_t *mmrheader)
{
	array_t array;
	uint32_t sign;
	
	if(!buf || !mmrheader) return 0;
	if(buf_size < pureg4_mmrheader_size) return 0;
	
	ArrayInit(buf, buf_size, &array);
	
	sign = ArrayReadToUInt32(&array, 24);
	if(sign != pureg4_mmrheader_sign) return 0;
	
	mmrheader->flags = ArrayReadToUInt32(&array, 8);
	mmrheader->width = ArrayReadToUInt32(&array, 16);
	mmrheader->height = ArrayReadToUInt32(&array, 16);
	
	if(ArrayFullOffset(&array) != pureg4_mmrheader_size) return 0;
	
	return pureg4_mmrheader_size;
}

size_t pureg4DecodeImage(uint8_t *buf, size_t buf_size, uint16_t width, uint16_t height, uint8_t flags, uint8_t *imgbuf)
{
	uint8_t *p;
	
	if(SIZE_MAX / width < height) return 0;
	
	if(flags & (~PUREG4_MMR_FLAG_MIN_IS_BLACK)) return 0;
	
	memset(imgbuf, 0, (size_t)width*(size_t)height);
	
	if(flags & PUREG4_MMR_FLAG_MIN_IS_BLACK) {
		size_t i;
		
		p = imgbuf;
		
		for(i = 0; i < (size_t)width*(size_t)height; i++)
			*p = 255-(*p);
	}
	
	return buf_size;
}

static void ArrayInit(uint8_t *buf, size_t buf_size, array_t *array)
{
	memset(array, 0, sizeof(array_t));
	
	array->buf = buf;
	array->curptr = buf;
	array->buf_size = buf_size;
}

static size_t ArrayFullOffset(array_t *array)
{
	size_t offset;
	
	offset = array->byte_offset;
	if(array->bit_offset) offset++;
	
	return offset;
}

static uint32_t ArrayReadToUInt32(array_t *array, size_t bits)
{
	uint32_t ret = 0;
	
	if(!bits || bits > 32) return 0;
	
	while(bits > 0) {
		size_t bits_to_read;
		
		if(array->byte_offset >= array->buf_size) break;
		
		if(8-array->bit_offset < bits)
			bits_to_read = 8-array->bit_offset;
		else
			bits_to_read = bits;
		
		ret = ret << bits_to_read;
		ret = ret + (((*array->curptr) >> array->bit_offset) & ((1 << bits_to_read) - 1));
		
		array->bit_offset += bits_to_read;
		if(array->bit_offset > 7) {
			array->bit_offset = 0;
			array->curptr++;
			array->byte_offset++;
		}
		
		bits -= bits_to_read;
	}
	
	return ret;
}

