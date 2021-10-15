#ifndef SCREEN_RECORD_SRC_ARGUMENT_H_
#define SCREEN_RECORD_SRC_ARGUMENT_H_

#include "gflags/gflags.h"
#include "screen_record/src/setting/setting_manager.h"

DECLARE_int32(fps);
DECLARE_string(capturer);

extern SettingManager* g_setting_manager;

#endif  // SCREEN_RECORD_SRC_ARGUMENT_H_
