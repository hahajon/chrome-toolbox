#pragma once

#include "video_window.h"
#include <map>

#define MAX_VIDEOWINDOW   1024

class VideoWindowManager {
public:
  VideoWindowManager(void);
  ~VideoWindowManager(void);

  bool AddNewVideoWindow(HWND chromeHwnd, HWND videoHwnd);
  BOOL WndProc(HWND hwnd, UINT& msg, WPARAM& wParam, LPARAM& lParam);

private:
  typedef std::map<HWND, VideoWindow*> VideoMap;
  typedef std::pair<HWND, VideoWindow*> VideoWinPair;

private:
  VideoMap video_map_;
  
};