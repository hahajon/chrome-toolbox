#include "stdafx.h"
#include "video_window.h"
#include "resource.h"
#include "log.h"

extern Log g_Log;
extern HMODULE g_hMod;
extern WNDPROC g_OldProc;

const int kMinChromeHeiht = 150;

VideoWindow::VideoWindow(void) {
}

VideoWindow::~VideoWindow(void) {
}

bool VideoWindow::InitWindow(HWND chromeHwnd, HWND videoHwnd) {
  chrome_hwnd_ = chromeHwnd;
  video_hwnd_ = videoHwnd;
  lastSizeParam = NULL;
  bFlag = NeedResizeState;
  nLoop = 9;
  tip_button_.Init(chrome_hwnd_);

  return true;
}

void VideoWindow::Unsubclass() {
  if (IsWindow(chrome_hwnd_) && g_OldProc != NULL) {
    SubclassWindow(chrome_hwnd_, g_OldProc);
  }
}

BOOL VideoWindow::WndProc(HWND hwnd, UINT& msg, WPARAM& wParam, LPARAM& lParam) {
  if (hwnd != chrome_hwnd_) {
    return FALSE;
  }  
  
  POINT pt;
  
  char logs[256]; 
  sprintf(logs, "hwnd=0x%X,msg=0x%04X,wParam=%ld,lParam=%ld,ThreadID=%ld",
          hwnd, msg, wParam, lParam, GetCurrentThreadId());
  g_Log.WriteLog("WndProc", logs);
  switch(msg) {
    case WM_PAINT:
    case WM_ACTIVATE:
      if (bFlag == ReadyResizeState || bFlag == FinishResizeState)
        tip_button_.OnPaint();
      break;
    case WM_TIMER:
      {
        if (wParam != EVENTID_FRESH)
          break;

        HWND hChildWnd = FindWindowEx(chrome_hwnd_, NULL, 
            L"Chrome_WidgetWin_0", NULL);
        int cy = CONST_FRAME_CAPTION_HEIGHT;
        if (IsMaximized(chrome_hwnd_))
          cy = 25;
        int cx = CONST_FRAME_BORDER;
        RECT rt;
        GetWindowRect(chrome_hwnd_, &rt);
        if (nLoop++ % 10 == 0) {
          if (lastSizeParam == NULL) {
            int height = rt.bottom - rt.top;
            if (height < kMinChromeHeiht)
              height = kMinChromeHeiht;
            bFlag = ReadyResizeState;
            SetWindowPos(chrome_hwnd_, NULL, 0, 0, rt.right-rt.left, height, 
                         SWP_NOMOVE | SWP_NOREPOSITION);
            lastSizeParam = MAKELPARAM(rt.right-rt.left, height);
          } else if (bFlag == NeedResizeState) {
            bFlag = ReadyResizeState;
            SendMessage(chrome_hwnd_, WM_SIZE, SIZE_RESTORED, lastSizeParam);
          }
          GetWindowRect(chrome_hwnd_, &rt);
        }

        if (hChildWnd != NULL) {
          RECT rt1;
          GetWindowRect(hChildWnd, &rt1);
          if (rt1.top != rt.top + cy) {
            MoveWindow(hChildWnd, cx, cy, rt.right-rt.left-2*cx,
                       rt.bottom-rt.top-cy-2*cx, FALSE);
            sprintf(logs, "rt1.top=%ld,rt.top=%ld,cx=%ld", rt1.top, 
                      rt.top, cy);
            g_Log.WriteLog("MoveWindow", logs);
          }
        } else
          g_Log.WriteLog("Error", "hChildWnd==NULL");
      }
      break;
    case WM_NCMOUSEMOVE:
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      tip_button_.OnMouseOver(pt);
      break;
    case WM_NCLBUTTONDOWN:
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      tip_button_.OnMouseDown(pt);
      if (wParam == HTCLOSE)
        msg = WM_NULL;
      break;
    case WM_NCLBUTTONUP:
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      if (wParam == HTCLOSE) {
        PostMessage(video_hwnd_, WM_CHROMECLOSE, (WPARAM)chrome_hwnd_, 0);
      }
      tip_button_.OnMouseUp(pt);
      break;
    case WM_NCLBUTTONDBLCLK:
      msg = WM_NULL;
      break;
    case WM_NCMOUSELEAVE:
      tip_button_.OnMouseLeave();
      break;
    case WM_DESTROY:
      g_Log.WriteLog("WM_DESTROY", "WM_DESTROY");
      break;
    case WM_CLOSE:
      g_Log.WriteLog("WM_CLOSE", "WM_CLOSE");
      break;
    case WM_QUIT:
      g_Log.WriteLog("WM_QUIT", "WM_QUIT");
      break;
    case WM_SYSCOMMAND:
      if (wParam == SC_CLOSE) {
        msg = WM_NULL;
        PostMessage(video_hwnd_, WM_CHROMECLOSE, (WPARAM)chrome_hwnd_, 0);
      }
      break;
    case WM_SIZE:
      {
        if (bFlag != ReadyResizeState) {
          msg = WM_NULL;
          bFlag = NeedResizeState;
        } else if (bFlag == ReadyResizeState) {
          bFlag = FinishResizeState;
        }

        HWND hChildWnd = FindWindowEx(chrome_hwnd_, NULL, 
            L"Chrome_WidgetWin_0", NULL);
        int cy = CONST_FRAME_CAPTION_HEIGHT;
        int cx = CONST_FRAME_BORDER;
        int width, height;
        width = LOWORD(lParam);
        height = HIWORD(lParam);
        if (height < kMinChromeHeiht) {
          SetWindowPos(chrome_hwnd_, NULL, 0, 0, width, kMinChromeHeiht, 
                       SWP_NOMOVE | SWP_NOREPOSITION);
          lParam = MAKELPARAM(width, kMinChromeHeiht);
          height = kMinChromeHeiht;
        }
        lastSizeParam = lParam;

        if (hChildWnd != NULL) {
          MoveWindow(hChildWnd, cx, cy, width-2*cx, height-cy-2*cx, FALSE);
          sprintf(logs, "width=%ld,heigth=%ld,cx=%ld,cy=%ld",
                  width, height, cx, cy);
          g_Log.WriteLog("MoveWindow", logs);
        } else
          g_Log.WriteLog("Error", "hChildWnd==NULL");
      }
      break;
  }

  return FALSE;
}