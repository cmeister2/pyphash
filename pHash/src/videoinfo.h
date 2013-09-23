/*
 * videoinfo.h
 *
 *  Created on: 23 Sep 2013
 *      Author: cmeister2
 */

#ifndef VIDEOINFO_H_
#define VIDEOINFO_H_

#define cimg_display 0
#define cimg_debug 0

#include "CImg.h"

#define __STDC_CONSTANT_MACROS

extern "C" {
        #include "./libavformat/avformat.h"
        #include "./libavcodec/avcodec.h"
        #include "./libswscale/swscale.h"
}

using namespace cimg_library;

#include <assert.h>
#include <pHash.h>

typedef struct video_info {
  //Video filename
  const char *filename;

  //Format I/O context
  AVFormatContext *format_ctx;

  //Video stream ID
  int stream_id;
  AVStream *stream;

  //main external API structure.
  AVCodecContext *codec_ctx;

  //Video stream decoder.
  AVCodec *codec;

  //Video width
  int width;

  //Video height
  int height;

  //Destination pixel format
  PixelFormat dest_pix_fmt;

} VIDEOINFO;

typedef struct callback_parms
{
  //Video info
  VIDEOINFO *vfinfo;

  //Scaling context
  SwsContext *sws_cxt;

  //Original frame
  AVFrame *original;

  //Frame for conversion
  AVFrame *converted;

  //User data
  void *user_data;

} CALLBACK_PARMS;

int videoinfo_init(const char *, VIDEOINFO *);
void videoinfo_term(VIDEOINFO *);

int callback_parms_init(CALLBACK_PARMS *, VIDEOINFO *);
void callback_parms_term(CALLBACK_PARMS *);

typedef void (frame_callback)(CALLBACK_PARMS *);

void each_frame(VIDEOINFO *, frame_callback *, void *);
void vi_keyframes(CALLBACK_PARMS *);

typedef struct T_doubleArray {
    double *array;
    int used, size;
} doubleArray;

typedef struct T_ulong64Array {
    ulong64 *array;
    int used, size;
} ulong64Array;

#define DefineArray(TYPE) \
void init_##TYPE##Array(TYPE##Array *a, size_t initialSize) {\
  a->array = (TYPE *)malloc(initialSize * sizeof(TYPE));\
  a->used = 0;\
  a->size = initialSize;\
}\
void insert_##TYPE##Array(TYPE##Array *a, TYPE element) {\
  if (a->used == a->size) {\
    a->size += 100;\
    a->array = (TYPE *)realloc(a->array, a->size * sizeof(TYPE));\
  }\
  a->array[a->used] = element;\
  a->used++;\
}\
void free_##TYPE##Array(TYPE##Array *a) {\
  free(a->array);\
  a->array = NULL;\
  a->used = a->size = 0;\
}

typedef struct hashdata
{
  //Framelist for storing the keyframes
  //CImgList<uint8_t> *framelist;

  //Array for storing the presentation timestamps
  doubleArray pts;

  //Ulong array for storing the hashes
  ulong64Array hashes;

  //Keyframe count
  int keyframes;

  //DCT matrix and transpose
  //CImg<float> *C;

} HASHDATA;

HASHDATA *ph_max_videohash(char *filename);

#endif /* VIDEOINFO_H_ */
