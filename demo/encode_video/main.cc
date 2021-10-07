// 相关文档
//  介绍码率和比特率
//   https://zhuanlan.zhihu.com/p/75804693
//  编码相关
//   https://stackoverflow.com/questions/46444474/c-ffmpeg-create-mp4-file

#include <assert.h>
#include <stdio.h>

#include <atomic>
#include <chrono>
#include <thread>

#include <shlwapi.h>

#include "gflags/gflags.h"
#include "log.h"
#include "picture_capturer_d3d9.h"
#include "picture_capturer_dxgi.h"
#include "picture_capturer_gdi.h"
#include "picture_queue.h"
#include "video_muxer.h"

DEFINE_string(capturer, "gdi", "截屏方式");
DEFINE_int32(color_type, 0, "颜色，0: rgb, 1: argb");
DEFINE_int32(frame_count, 512, "帧数");
DEFINE_int32(fps, 30, "帧率");
DEFINE_int32(bit_rate, 1000000, "比特率");
DEFINE_string(encoder_name, "libx264", "视频编码器名称");
DEFINE_string(file_type, "mp4", "文件格式");

const int kPathLen = 2048;
const int kMaxSize = 1024 * 1024 * 1024;

static std::atomic_bool g_exit = false;
static PictureQueue<kMaxSize> g_picture_queue;

int64_t GetCurrentMilliseconds() {
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  int64_t milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  return milliseconds;
}

std::string GenerateOutDir() {
  char path[kPathLen];
  memset(path, 0, sizeof(char) * kPathLen);

  GetModuleFileNameA(NULL, path, kPathLen);
  PathRemoveFileSpecA(path);

  PathAppendA(path, "out");
  DWORD fileattr = ::GetFileAttributesA(path);
  if (fileattr != INVALID_FILE_ATTRIBUTES) {
    if ((fileattr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
      assert(false);
    }
  } else {
    if (!CreateDirectoryA(path, NULL)) {
      assert(false);
    }
  }

  PathAppendA(path, "encode_video");
  fileattr = ::GetFileAttributesA(path);
  if (fileattr != INVALID_FILE_ATTRIBUTES) {
    if ((fileattr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
      assert(false);
    }
  } else {
    if (!CreateDirectoryA(path, NULL)) {
      assert(false);
    }
  }

  strncat(path, "\\", strlen("\\"));
  return std::string(path);
}

void CaptureThread(PictureCapturer* capturer) {
  LOG("开始截屏");

  assert(capturer);

  double interval = 1000.0 / FLAGS_fps;
  double pts = 0.0;
  double delta = 0.0;

  auto abort_func = []() { return false; };

  int current_count = 0;
  auto start_time = std::chrono::high_resolution_clock::now();
  auto start = start_time;
  auto end = start_time;

  int count = 0;
  auto t1 = start_time;
  auto t2 = start_time;
  while (current_count++ < FLAGS_frame_count) {
    start = std::chrono::high_resolution_clock::now();
    pts = std::chrono::duration<double, std::milli>(start - start_time).count();

    PictureCapturer::Picture* picture = capturer->Capture();
    if (!picture) {
      continue;
    }
    picture->pts = pts;
    g_picture_queue.Push(picture, abort_func);

    end = std::chrono::high_resolution_clock::now();
    delta = std::chrono::duration<double, std::milli>(end - start).count();
    if (interval > delta) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(static_cast<int64_t>(interval - delta)));
    }

    ++count;
    t2 = std::chrono::high_resolution_clock::now();
    delta = std::chrono::duration<double>(t2 - t1).count();
    if (delta > 1) {
      fprintf(stderr, "FPS: %.3f\n", count / delta);
      count = 0;
      t1 = t2;
    }
  }

  LOG("截屏结束");

  g_exit = true;
  g_picture_queue.Notify();
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  assert(FLAGS_fps > 0);
  assert(FLAGS_bit_rate > 0);
  assert(FLAGS_frame_count > 0);
  assert(!FLAGS_capturer.empty());
  assert(!FLAGS_encoder_name.empty());
  assert(!FLAGS_file_type.empty());

  const char* color_name = nullptr;
  AVPixelFormat input_pix_fmt = AV_PIX_FMT_NONE;
  char filepath[kPathLen];

  VideoConfig config;
  config.fps = FLAGS_fps;
  config.bit_rate = FLAGS_bit_rate;
  config.file_format = FLAGS_file_type.c_str();

  PictureCapturer* capturer = nullptr;
  if (FLAGS_capturer == "gdi") {
    capturer = new PictureCapturerGdi(FLAGS_color_type);
  } else if (FLAGS_capturer == "d3d9") {
    capturer = new PictureCapturerD3D9(FLAGS_color_type);
  } else if (FLAGS_capturer == "dxgi") {
    capturer = new PictureCapturerDXGI(FLAGS_color_type);
  } else {
    assert(false && "请指定符合要求的截屏方式");
    return 1;
  }

  PictureCapturer::Picture* picture = capturer->Capture();
  assert(picture);
  config.width = picture->width;
  config.height = picture->height;
  delete picture;

  if (FLAGS_color_type == 0) {
    color_name = "rgb24";
    input_pix_fmt = AV_PIX_FMT_BGR24;
  } else if (FLAGS_color_type == 1) {
    color_name = "argb32";
    input_pix_fmt = AV_PIX_FMT_RGB32;
  }
  config.input_pix_fmt = input_pix_fmt;

  // 构造输出路径
  memset(filepath, 0, kPathLen);
  sprintf(filepath, "%s%s_%s_%s_%lld.%s",
          GenerateOutDir().c_str(),
          FLAGS_encoder_name.c_str(),
          FLAGS_capturer.c_str(),
          color_name,
          GetCurrentMilliseconds(),
          FLAGS_file_type.c_str());

  printf("\n\n");
  LOG("帧率: %d", FLAGS_fps);
  LOG("截屏帧数: %d", FLAGS_frame_count);
  LOG("截屏方式: %s", FLAGS_capturer.c_str());
  LOG("颜色值: %s", color_name);
  LOG("输出文件: %s\n\n", filepath);

  std::unique_ptr<VideoMuxer> muxer(new VideoMuxer());
  if (!muxer->Open(config, filepath)) {
    return false;
  }

  std::thread capture_thread = std::thread(CaptureThread, capturer);

  auto abort_func = []() { return g_exit ? true : false; };
  while (true) {
    if (!g_picture_queue.Pop(&picture, abort_func)) {
      break;
    }

    muxer->PushData(picture->rgb ? picture->rgb : picture->argb,
                    picture->width, picture->height, picture->len,
                    static_cast<int64_t>(picture->pts));

    delete picture;
    picture = nullptr;
  }

  capture_thread.join();

  muxer->Close();

  LOG("录制结束，输出文件: %s", filepath);
  return 0;
}
