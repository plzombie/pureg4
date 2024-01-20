/*
BSD 2-Clause License

Copyright (c) 2024, Mikhail Morozov

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

#include "../pureg4.h"

#include "test_util.h"

#include "third_party/g4code.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOF_TESTS 100
#define MAX_TESTED_WIDTH 1024

static bool TestDecoder(uint16_t width, uint16_t height);
static bool EncodeImageForTest(uint16_t width, uint16_t height, uint8_t *imgbuf, uint8_t **buf, size_t *buf_size);

int main(int argc, char **argv)
{
	size_t i;
	size_t tests_failed = 0, tests_passed = 0, tests_total = 0;

	for(i = 0; i < NOF_TESTS; i++) {
		uint16_t width, height;

		width = 1 + rand() % MAX_TESTED_WIDTH;
		height = 1 + rand() % MAX_TESTED_WIDTH;

		if(TestDecoder(width, height))
			tests_passed++;
		else
			tests_failed++;

		tests_total++;
	}

	tests_total++;
	if(TestDecoder(1, 1)) tests_passed++; else tests_failed++;

	tests_total++;
	if(TestDecoder(1, UINT16_MAX)) tests_passed++; else tests_failed++;

	tests_total++;
	if(TestDecoder(UINT16_MAX, 1)) tests_passed++; else tests_failed++;

	tests_total++;
	if(TestDecoder(MAX_TESTED_WIDTH, MAX_TESTED_WIDTH)) tests_passed++; else tests_failed++;

	printf("Total %u tests, %u passed, %u failed\n", (unsigned int)tests_total, (unsigned int)tests_passed, (unsigned int)tests_failed);

	return (tests_total == tests_passed)?(EXIT_SUCCESS):(EXIT_FAILURE);
}

bool TestDecoder(uint16_t width, uint16_t height)
{
	pureg4_mmrheader_t mmrheader;
	uint8_t *buf = 0, *origimgbuf = 0, *newimgbuf = 0;
	size_t i, buf_size = 0, mmrhead_size, newimg_size;
	bool result = false;

	// Generate test image with desired size
	origimgbuf = GenTestImage(width, height);
	if(!origimgbuf) goto FINAL;

	// Encode test image
	if(!EncodeImageForTest(width, height, origimgbuf, &buf, &buf_size)) goto FINAL;

	// Read mmr header of encoded image and compare dimensions
	mmrhead_size = pureg4DecodeMMRHeader(buf, buf_size, &mmrheader);
	if(!mmrhead_size) goto FINAL;

	if(buf_size <= mmrhead_size) goto FINAL;

	if(mmrheader.width != width || mmrheader.height != height) goto FINAL;

	// Decode image with pureg4
	newimg_size = pureg4GetDecodedImageSize(width, height);
	if(!newimg_size) goto FINAL;

	newimgbuf = malloc(newimg_size);
	if(!newimgbuf) goto FINAL;

	if(!pureg4DecodeImage(buf, buf_size-mmrhead_size, width, height, mmrheader.flags, newimgbuf)) goto FINAL;

	// Compare decoded image with original image
	for(i = 0; i < newimg_size; i++)
		if(origimgbuf[i] != newimgbuf[i]) goto FINAL;

	// All done, return true
	result = true;

FINAL:
	if(origimgbuf) free(origimgbuf);
	if(newimgbuf) free(newimgbuf);
	if(buf) free(buf);

	return result;
}

typedef struct {
	uint8_t *buf;
	size_t bufsize, offset;
} writebuf_t;

static int G4WriteFunc(writebuf_t *writebuf, unsigned char *buf, int len)
{
	if(SIZE_MAX - writebuf->offset < (size_t)len) return 1;
	if(writebuf->offset+len > writebuf->bufsize) return 1;

	memcpy(writebuf->buf+writebuf->offset, buf, len);

	writebuf->offset += len;

	return 0;
}

static bool EncodeImageForTest(uint16_t width, uint16_t height, uint8_t *imgbuf, uint8_t **buf, size_t *buf_size)
{
	size_t new_buf_size;
	uint8_t *new_buf, *p;
	uint8_t bwbuf[8192];
	writebuf_t writebuf;
	G4STATE *g4_state;

	if(SIZE_MAX / width / 2 < height) return false;

	new_buf_size = (size_t)width*(size_t)height*2;

	if(SIZE_MAX - 8 < new_buf_size) return false;

	new_buf_size += 8;

	new_buf = malloc(new_buf_size);
	if(!new_buf) return false;

	new_buf[0] = 'M';
	new_buf[1] = 'M';
	new_buf[2] = 'R';
	new_buf[3] = 0; // Flags
	new_buf[4] = (width / 256) % 256;
	new_buf[5] = width % 256;
	new_buf[6] = (height / 256) % 256;
	new_buf[7] = height % 256;

	*buf = new_buf;
	*buf_size = new_buf_size;

	writebuf.buf = new_buf;
	writebuf.bufsize = new_buf_size;
	writebuf.offset = 8;

	g4_state = init_g4_write(-1, width, (WRITEFUNC)G4WriteFunc, &writebuf);

	p = imgbuf;
	for(uint16_t i = 0; i < height; i++) {
		memset(bwbuf, 0, 8192);
		for(uint16_t j = 0; j < width; j++) {
			size_t byte_offset, bit_offset;

			byte_offset = j / 8;
			bit_offset = j % 8;

			if(*p == 0) // Min is white
				bwbuf[byte_offset] |= bit_offset;
			
			p++;
		}
		encode_g4(g4_state, bwbuf);
	}
	encode_g4(g4_state, NULL);
	free_g4(g4_state);

	return true;
}

