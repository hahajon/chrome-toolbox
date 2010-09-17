#pragma once

#include <windows.h>
#include "ncbutton.h"

class VideoWindow {
public:
  VideoWindow(void);
  ~VideoWindow(void);

  bool InitWindow(HWND chromeHwnd, HWND videoHwnd);
  bool IsEnqual(HWND hwnd) { return hwnd == chrome_hwnd_; }
  BOOL WndProc(HWND hwnd, UINT& msg, WPARAM& wParam, LPARAM& lParam);
  void Unsubclass();

private:
  HWND chrome_hwnd_;
  HWND video_hwnd_;
  NCButton tip_button_;
  LPARAM lastSizeParam;

};