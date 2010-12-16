#include "stdafx.h"
#include "video_window_manager.h"
#include "log.h"

extern Log g_Log;

VideoWindowManager::VideoWindowManager(void) {
}

VideoWindowManager::~VideoWindowManager(void) {
  VideoMap::iterator iter;
  for(iter = video_map_.begin(); iter != video_map_.end(); iter++) {
    iter->second->Unsubclass();
  }
}

bool VideoWindowManager::AddNewVideoWindow(HWND chromeHwnd, HWND videoHwnd) {
  bool bFlag = false;

  VideoMap::iterator pair = video_map_.find(chromeHwnd);
  if (pair == video_map_.end()) {
    char logs[256];
    sprintf(logs, "AddNewVideoWindow,chromeHwnd=0x%X,videohwnd=0x%X",
      chromeHwnd, videoHwnd);
    g_Log.WriteLog("AddNewVideoWindow", logs);
    VideoWindow* pVideo = new VideoWindow();
    pVideo->InitWindow(chromeHwnd, videoHwnd);
    video_map_.insert(VideoWinPair(chromeHwnd, pVideo));
    bFlag = true;
  }

  return bFlag;
}

BOOL VideoWindowManager::WndProc(HWND hwnd, UINT& msg, 
                                 WPARAM& wParam, LPARAM& lParam) {
  BOOL bRet = FALSE;
  VideoMap::iterator pair = video_map_.find(hwnd);
  if (pair != video_map_.end()) {
    bRet = pair->second->WndProc(hwnd, msg, wParam, lParam);
  }
  return FALSE;
}