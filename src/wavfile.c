/*
 * Copyright (C) 2008-2018 by Erik Hofman.
 * Copyright (C) 2009-2018 by Adalin B.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice,
 *        this list of conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY ADALIN B.V. ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL ADALIN B.V. OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUTOF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Adalin B.V.
 */


#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _WIN32
# include <io.h>
#endif
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <aax/aax.h>
#include <base/types.h>

#include "driver.h"
#include "wavfile.h"

#define PRINT_DEBUG_MSG		0
#define PRINT_INFO_MSG		1

#define WAVE_HEADER_SIZE	11
#define WAVE_EXT_HEADER_SIZE	17
#define DEFAULT_OUTPUT_RATE	32000
#ifndef O_BINARY
# define O_BINARY		0
#endif

static uint32_t _oalSoftwareWaveHeader[WAVE_EXT_HEADER_SIZE];
#if 0
static const uint32_t _defaultWaveHeader[WAVE_EXT_HEADER_SIZE] =
{
    0x46464952,          /*  0. "RIFF"                                        */
    0x00000024,          /*  1. (file_length - 8)                             */
    0x45564157,          /*  2. "WAVE"                                        */

    0x20746d66,          /*  3. "fmt "                                        */
    0x00000010,          /*  4.                                               */
    0x00010002,          /*  5. PCM & stereo                                  */
    DEFAULT_OUTPUT_RATE, /*  6.                                               */
    0x0001f400,          /*  7. (sample_rate*no_tracks*bits_sample/8)         */
    0x00040010,          /*  8. (no_tracks*bits_sample/8)                     *
                          *    & 16 bits per sample                           */
/* used for both the extensible data section and data section                 */
    0x61746164,          /*  9. "data"                                        */
    0,                   /* 10. length of the data block                      *
                          *    (no_samples*no_tracks*bits_sample/8)           */
    0,0,
/* data section starts here in case of the extensible format                  */
    0x61746164,	         /* 15. "data"                                        */
    0                    /* 16. length of the data block                      *
                         *    (no_samples*no_tracks*bits_sample/8)            */
};
#endif

#define DEBUG_PRINT(a) \
    printf(" %2i: %08x \"%c%c%c%c\"\n", a, _oalSoftwareWaveHeader[a], \
             p[4*a], p[4*a+1], p[4*a+2], p[4*a+3]);

#define BSWAP16(x)	(x >> 8) | (x << 8)
#define BSWAP32H(x)	((x >> 8) & 0x00FF00FFL) | ((x << 8) & 0xFF00FF00L)
#define BSWAP32W(x)	(x >> 16) | (x << 16)
#define BSWAP32(x)	BSWAP32W(BSWAP32H(x))


static char __big_endian = 0;
static void bufferConvertMSIMA_IMA4(void*, unsigned, unsigned int, unsigned*);

/**
 * Load a canonical WAVE file into memory and return a pointer to the buffer.
 *
 * @param a pointer to the exact ascii file location
 * @param no_samples the returned number of samples per audio track
 * @param freq the returned sample frequency of the audio tracks
 * @param bits_sample the returned number of bits per sample
 * @param no_tracks the returned number of audio tracks in the buffer
 */
void *
fileLoad(const char *file, unsigned int *no_samples, 
#if !_OPENAL_SUPPORT
         unsigned *block,
#endif
         int *freq, char *bits_sample, char *no_tracks, unsigned int *format)
{
    static const unsigned int _t = 1;
    unsigned int buflen, blocksz = 1;
    char buf[4];
    void *data;
    int i, fd;
    int res;

#if !_OPENAL_SUPPORT
    *block = 1;
#endif

    __big_endian = (*(char *)&_t == 0);

    fd = open(file, O_RDONLY);
    if (fd < 0) return 0;

    res = read(fd, &_oalSoftwareWaveHeader, WAVE_EXT_HEADER_SIZE*4);
    if (__big_endian)
    {
        for (i=0; i<WAVE_EXT_HEADER_SIZE; i++) {
            _oalSoftwareWaveHeader[i] = BSWAP32(_oalSoftwareWaveHeader[i]);
        }
    }

#if PRINT_DEBUG_MSG
    do
    {
        char *p = (char *)&_oalSoftwareWaveHeader;
        printf("Load:\n");
        for (i=0; i<WAVE_EXT_HEADER_SIZE; i++)
            DEBUG_PRINT(i);
    }
    while (0);
#endif

    /* Is it a RIFF file? */
    if (_oalSoftwareWaveHeader[0] == 0x46464952)              /* RIFF */
    {
        char extfmt;

	// EXTENSIBLE_WAVE_FORMAT?
        *format = _oalSoftwareWaveHeader[5] & 0xFFFF;
        extfmt = (*format == 0xFFFE) ? 1 : 0;

        if (_oalSoftwareWaveHeader[2] == 0x45564157 &&	/* WAVE */
            _oalSoftwareWaveHeader[3] == 0x20746d66)	/* fmt  */
        {
            *freq = _oalSoftwareWaveHeader[6];
            *no_tracks = _oalSoftwareWaveHeader[5] >> 16;
            *bits_sample = extfmt ? _oalSoftwareWaveHeader[9] >> 16 :
                                    _oalSoftwareWaveHeader[8] >> 16;
            *format = extfmt ? _oalSoftwareWaveHeader[11] :
                               _oalSoftwareWaveHeader[5] & 0xFFFF;
            blocksz = _oalSoftwareWaveHeader[8] & 0xFFFF;
#if !_OPENAL_SUPPORT
            *block = blocksz;
#endif
        }
    }

    /* search for the data chunk */
    lseek(fd, 32L, SEEK_SET);
    do
    {
        res = read(fd, buf, 1);
        if (res > 0)
        {
           if (buf[0] == 'd')
           {
               res = read(fd, buf+1, 3);
               if (res > 0 && buf[0] == 'd' && buf[1] == 'a'
                    && buf[2] == 't' && buf[3] == 'a')
               {
                   res = read(fd, &buflen, 4); /* chunk size */
                   if (__big_endian) buflen = BSWAP32(buflen);
                   break;
               }
           }
       }
    }
    while (1);
    *no_samples = (buflen * 8) / (*no_tracks * *bits_sample);

#if PRINT_INFO_MSG
    do
    {
        float duration;

        printf("Audio file: %s\n", file);
        if (buflen < 10240)
            printf("Size:\t\t\t%i bytes\n", buflen);
        else
            printf("Size:\t\t\t%i kb (%u bytes)\n", buflen / 1024, buflen);
        printf("Sample rate:\t\t%i kHz\n", *freq / 1000);
        printf("No. tracks:\t\t%i\n", *no_tracks);
        printf("Bits per sample:\t%i\n", *bits_sample);

        printf("Data format:\t\t");
        switch (*format)
        {
        case 1:
            printf("PCM\n");
            break;
        case 2:
            printf("Microsoft ADPCM\n");
            break;
        case 3:
            printf("PCM Floating point\n");
            break;
        case 6:
            printf("G.711 a-law\n");
            break;
        case 7:
            printf("G.711 mulaw\n");
            break;
        case 17:
            printf("IMA4 ADPCM\n");
            break;
        case 0xfffe:
            printf("Extensible format (0x%X)\n",
                   _oalSoftwareWaveHeader[11] >> 16);
            break;
        default:
            printf("unknown (0x%X)\n", *format);
        }

        printf("Samples per block:\t%i\n", blocksz);
        printf("No. samples:\t\t%i\n", *no_samples);

        duration = (float)(buflen * 8);
        duration /= (*freq * *no_tracks * *bits_sample);
        printf("Duration:\t\t%5.3f sec.\n", duration);
    } while (0);
#endif

    data = malloc(buflen);
    if (data)
    {
        buflen = read(fd, data, buflen);

#if _OPENAL_SUPPORT
        /* OpenAL only, AeonWave does the conversion for us */
        if (__big_endian && (*bits_sample > 8))
        {
            uint32_t i, *p = (uint32_t *)data;

            if (*bits_sample == 16)
            {
                for(i=0; i < buflen/4; i++) {
                    p[i] = BSWAP32H(p[i]);
                }
                if ((buflen-(buflen/4)*4) > 0)
                {
                    i++;
                    p[i] = BSWAP16(p[i]);
                }
            }
            else if (*bits_sample == 32)
            {
                for(i=0; i < buflen/4; i++) {
                    p[i] = BSWAP32(p[i]);
                }
            }
        }
#endif
    }
    close(fd);

    return data;
}


aaxBuffer
bufferFromFile(aaxConfig config, const char *infile)
{
#if 1
   return aaxBufferReadFromStream(config, infile);
#else
    aaxBuffer buffer = NULL;
    unsigned int fmt, no_samples;
    enum aaxFormat format;
    char bps, channels;
    unsigned block = 1;
    int res, freq;
    void *data;

    data = fileLoad(infile, &no_samples,
#if !_OPENAL_SUPPORT
                    &block,
#endif
                    &freq, &bps, &channels, &fmt);
    format = getFormatFromFileFormat(fmt, bps);
    if (data && format != AAX_FORMAT_NONE)
    {
        buffer = aaxBufferCreate(config, no_samples, channels, format);

        if (format == AAX_IMA4_ADPCM)
        {
            bufferConvertMSIMA_IMA4(data, channels, no_samples, &block);

            res = aaxBufferSetSetup(buffer, AAX_BLOCK_ALIGNMENT, block);
            testForState(res, "aaxBufferSetSetup(AAX_BLOCK_ALIGNMENT)");
        }

        res = aaxBufferSetSetup(buffer, AAX_FREQUENCY, freq);
        testForState(res, "aaxBufferSetSetup(AAX_FREQUENCY)");

        res = aaxBufferSetData(buffer, data);
        testForState(res, "aaxBufferSetData");
    }
    free(data);

    return buffer;
#endif
}


/**
 * process a canonical WAVE file in memory and return a pointer to the buffer.
 *
 * @param a pointer to the wave file buffer
 * @ve file param no_samples the returned number of samples per audio track
 * @param freq the returned sample frequency of the audio tracks
 * @param bits_sample the returned number of bits per sample
 * @param no_tracks the returned number of audio tracks in the buffer
 */
void*
dataLoad(const unsigned char *data, unsigned int *no_samples,
#if !_OPENAL_SUPPORT
         unsigned *block,
#endif
         int *freq, char *bits_sample, char *no_tracks, unsigned int *format)
{
    static const unsigned int _t = 1;
    unsigned int buflen, blocksz;
    char *ptr = (char*)data;
    int i;

#if !_OPENAL_SUPPORT
    *block = 1;
#endif

    __big_endian = (*(char *)&_t == 0);

    memcpy(&_oalSoftwareWaveHeader, ptr, WAVE_EXT_HEADER_SIZE*4);
    if (__big_endian)
    {
        for (i=0; i<WAVE_EXT_HEADER_SIZE; i++) {
            _oalSoftwareWaveHeader[i] = BSWAP32(_oalSoftwareWaveHeader[i]);
        }
    }

    *freq = _oalSoftwareWaveHeader[6];
    *no_tracks = _oalSoftwareWaveHeader[5] >> 16;
    *format = _oalSoftwareWaveHeader[5] & 0xFFFF;
    *bits_sample = _oalSoftwareWaveHeader[8] >> 16;
    blocksz = _oalSoftwareWaveHeader[8] & 0xFFFF;
#if !_OPENAL_SUPPORT
    *block = blocksz;
#endif

    /* search for the data chunk */
    ptr += 32L;
    do
    {
        if (*ptr++ == 'd' && *ptr++ == 'a' && *ptr++ == 't' && *ptr++ == 'a')
        {
            memcpy(&buflen, ptr, 4);
            ptr += 4;
            break;
        }
    }
    while (1);
    *no_samples = (buflen * 8) / (*no_tracks * *bits_sample);

    return ptr;
}


aaxBuffer
bufferFromData(aaxConfig config, const unsigned char *indata)
{
    aaxBuffer buffer = NULL;
    unsigned int fmt, no_samples;
    enum aaxFormat format;
    char bps, channels;
    unsigned block = 1;
    int res, freq;
    void *data;

    data = dataLoad(indata, &no_samples,
#if !_OPENAL_SUPPORT
                    &block,
#endif
                    &freq, &bps, &channels, &fmt);
    format = getFormatFromFileFormat(fmt, bps);
    if (data && format != AAX_FORMAT_NONE)
    {
        void *ptr = data;

        buffer = aaxBufferCreate(config, no_samples, channels, format);

        if (format == AAX_IMA4_ADPCM)
        {
            unsigned int size = channels*no_samples*bps/8;

            ptr = malloc(size);
            memcpy(ptr, data, size);
            bufferConvertMSIMA_IMA4(ptr, channels, no_samples, &block);

            res = aaxBufferSetSetup(buffer, AAX_BLOCK_ALIGNMENT, block);
            testForState(res, "aaxBufferSetSetup(AAX_BLOCK_ALIGNMENT)");
        }

        res = aaxBufferSetSetup(buffer, AAX_FREQUENCY, freq);
        testForState(res, "aaxBufferSetSetup(AAX_FREQUENCY)");

        res = aaxBufferSetData(buffer, ptr);
        testForState(res, "aaxBufferSetData");
 
        if (ptr != data) free(ptr);
    }

    return buffer;
}


#define REFRESH_RATE		250

int
playAudioTune(int argc, char **argv)
{
    char *ret = getCommandLineOption(argc, argv, "-p");
    if (ret) {
        aaxPlaySoundLogo( getDeviceName(argc, argv) );
    }
    return ret ? -1 : 0;
}


/**
 * Convert asound buffer from separated multichannel to interleaved format.
 *
 * @param in data input buffer
 * @param no_tracks the number of audio tracks in the buffer
 * @param bits_sample bytes per sample for the emitter buffer
 * @param no_samples number of samples per audio track
 */
void *
fileDataConvertToInterleaved(void *sbuf, char no_tracks, char bits_sample,
                             unsigned int no_samples)
{
    const unsigned int tracklen_bytes = no_samples*bits_sample;
    void *dbuf;
    
    dbuf = malloc(no_tracks * tracklen_bytes);
    if (dbuf && (no_tracks == 1)) {
        memcpy(dbuf, sbuf, tracklen_bytes);
    }
    else if (dbuf)
    {
        unsigned int frame_size = no_tracks*bits_sample;
        uint8_t *sptr, *dptr;
        int t;

        sptr = sbuf;
        for (t=0; t<no_tracks; t++)
        {
            unsigned int i;

            dptr = dbuf;
            dptr += t*bits_sample;

            for (i=0; i<no_samples; i++)
            {
                memcpy(dptr, sptr, bits_sample);
                sptr += bits_sample;
                dptr += frame_size;
            }
        }
    }

    return dbuf;
}

enum aaxFormat
getFormatFromFileFormat(unsigned int format, int  bps)
{
    enum aaxFormat rv = AAX_FORMAT_NONE;
    switch (format)
    {
    case 1:
        if (bps == 8) rv = AAX_PCM8U;
        else if (bps == 16) rv = AAX_PCM16S_LE;
        else if (bps == 32) rv = AAX_PCM32S_LE;
        break;
    case 3:
        if (bps == 32) rv = AAX_FLOAT_LE;
        else if (bps == 64) rv = AAX_DOUBLE_LE;
        break;
    case 6:
        rv = AAX_ALAW;
        break;
    case 7:
        rv = AAX_MULAW;
        break;
    case 17:
        rv = AAX_IMA4_ADPCM;
        break;
    case 65534:
        printf("Extenden WAV format not yet implemented\n");
        break;
    default:
        printf("Unsupported WAV format: %i\n", format);
        break;
    }
    return rv;
}

void
bufferConvertMSIMA_IMA4(void *data, unsigned channels, unsigned int no_samples, unsigned *blocksz)
{
    unsigned int blocksize = *blocksz;
    int32_t* buf;

    if (channels < 2) return;

    buf = malloc(blocksize);
    if (buf)
    {
        unsigned b, blocks, block_bytes, chunks;
        int32_t* dptr = (int32_t*)data;

        blocks = no_samples/blocksize;
        block_bytes = blocksize/channels;
        chunks = block_bytes/sizeof(int32_t);

        for (b=0; b<blocks; b++)
        {
            unsigned int t, i;

            /* block shuffle */
            memcpy(buf, dptr, blocksize);
            for (t=0; t<channels; t++)
            {
                int32_t* src = (int32_t*)buf + t;
                for (i=0; i<chunks; i++)
                {
                    *dptr++ = *src;
                    src += channels;
                }
            }
        }
        free(buf);
        *blocksz = block_bytes;
    }
}

