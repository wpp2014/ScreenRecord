// �������Ļ�µ�������ϵ

#include <windows.h>

#include "utils/virtual_screen.h"

int main() {
  utils::VirtualScreen vs;
  if (!vs.IsValid()) {
    printf("get virtual screen info failed\n");
    return 0;
  }

  return 0;
}
