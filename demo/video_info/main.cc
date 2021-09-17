#include <stdio.h>

extern "C" {
#include "libavformat/avformat.h"
}

const int kLen = 128;
char kErrorMsg[kLen];

static AVFrame* src_frame = NULL;
static int video_frame_count = 0;

char* AVError2Str(int ret, char* buffer, size_t buf_size) {
  return av_make_error_string(buffer, buf_size, ret);
}

int OutputVideoFrame(const AVFrame* frame) {
  fprintf(stderr, "video_frame n: %d, coded n: %d, pts: %" PRId64 "\n",
          video_frame_count++, frame->coded_picture_number, frame->pts);
  return 0;
}

int DecodeVideoPacket(AVCodecContext* ctx, AVPacket* pkt) {
  int ret = 0;

  ret = avcodec_send_packet(ctx, pkt);
  if (ret < 0) {
    memset(kErrorMsg, 0, kLen);
    fprintf(stderr,
            "Error submitting a packet for decoding (%s)\n",
            AVError2Str(ret, kErrorMsg, kLen));
    return ret;
  }

  while (ret >= 0) {
    ret = avcodec_receive_frame(ctx, src_frame);
    if (ret < 0) {
      if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
        return 0;
      }

      memset(kErrorMsg, 0, kLen);
      fprintf(stderr, "Error during decoding (%s)\n",
              AVError2Str(ret, kErrorMsg, kLen));
      return ret;
    }

    OutputVideoFrame(src_frame);

    av_frame_unref(src_frame);
  }

  return 0;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("参数错误\n");
    return 0;
  }

  int ret = 0;
  int video_stream_index = -1;
  AVFormatContext* format_ctx = nullptr;
  AVStream* video_stream = nullptr;
  AVCodec* video_codec = nullptr;
  AVCodecContext* video_codec_ctx = nullptr;
  AVPacket pkt;

  ret = avformat_open_input(&format_ctx, argv[1], NULL, NULL);
  if (ret < 0) {
    printf("avformat_open_input调用失败: %d\n", ret);
    goto end;
  }

  ret = avformat_find_stream_info(format_ctx, NULL);
  if (ret < 0) {
    printf("avformat_find_stream_info调用失败: %d\n", ret);
    goto end;
  }

  av_dump_format(format_ctx, 0, argv[1], 0);

  for (uint32_t i = 0; i < format_ctx->nb_streams; ++i) {
    if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_index = static_cast<int>(i);
      break;
    }
  }
  if (video_stream_index == -1) {
    printf("未发现AVStream\n");
    goto end;
  }
  video_stream = format_ctx->streams[video_stream_index];

  video_codec_ctx = avcodec_alloc_context3(NULL);
  if (!video_codec_ctx) {
    printf("avcodec_alloc_context3调用失败\n");
    goto end;
  }

  ret = avcodec_parameters_to_context(video_codec_ctx, video_stream->codecpar);
  if (ret < 0) {
    printf("avcodec_parameters_to_context调用失败\n");
    goto end;
  }

  video_codec = avcodec_find_decoder(video_codec_ctx->codec_id);
  if (!video_codec) {
    printf("找不到解码器\n");
    goto end;
  }

  ret = avcodec_open2(video_codec_ctx, video_codec, NULL);
  if (ret < 0) {
    printf("打开解码器失败\n");
    goto end;
  }

  src_frame = av_frame_alloc();
  if (!src_frame) {
    printf("av_frame_alloc调用失败\n");
    goto end;
  }

  av_init_packet(&pkt);
  while (av_read_frame(format_ctx, &pkt) == 0) {
    if (pkt.stream_index == video_stream_index) {
      DecodeVideoPacket(video_codec_ctx, &pkt);
    }
    av_packet_unref(&pkt);
  }

end:
  if (src_frame) {
    av_frame_free(&src_frame);
  }
  if (video_codec_ctx) {
    avcodec_free_context(&video_codec_ctx);
  }
  if (format_ctx) {
    avformat_close_input(&format_ctx);
  }

  return 0;
}
