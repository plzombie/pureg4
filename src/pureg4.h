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

#ifndef PUREG4_H
#define PUREG4_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

enum {
	PUREG4_MMR_FLAG_MIN_IS_BLACK = 0x1,
	PUREG4_MMR_FLAG_STRIPPED = 0x2
};

typedef struct {
	uint16_t width;
	uint16_t height;
	uint8_t flags;
} pureg4_mmrheader_t;

size_t pureg4GetDecodedImageSize(uint16_t width, uint16_t height);
size_t pureg4GetEncodedImageSize(uint16_t width, uint16_t height);
size_t pureg4DecodeMMRHeader(uint8_t *buf, size_t buf_size, pureg4_mmrheader_t *mmrheader);
size_t pureg4DecodeImage(uint8_t *buf, size_t buf_size, uint16_t width, uint16_t height, uint8_t flags, uint8_t *imgbuf);


#ifdef __cplusplus
}
#endif

#endif
