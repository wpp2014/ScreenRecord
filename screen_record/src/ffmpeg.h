﻿#ifndef SCREEN_RECORD_SRC_FFMPEG_H_
#define SCREEN_RECORD_SRC_FFMPEG_H_

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4819)

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"

#ifdef __cplusplus
}
#endif

#pragma warning(pop)

#endif  // SCREEN_RECORD_SRC_FFMPEG_H_
