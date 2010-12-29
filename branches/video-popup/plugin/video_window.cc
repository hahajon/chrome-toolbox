#include "stdafx.h"
#include "video_window.h"
#include "resource.h"
#include "log.h"

#include <Uxtheme.h>
#include <dwmapi.h>

extern Log g_Log;
extern HMODULE g_hMod;
extern WNDPROC g_OldProc;
extern int g_Chrome_MajorVersion;
extern BOOL g_Enable_DWM;

const int kMinChromeHeight = 150;
const int kBorderWidth = 8;
const int kOffsetSize = 8;
const int kIconSize = 16;
const int kCaptionHeight = 30;

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

void VideoWindow::PaintCustomCaption(BOOL drawicon)
{
  RECT rcClient;
  GetWindowRect(chrome_hwnd_, &rcClient);
  HDC hdc = GetWindowDC(chrome_hwnd_);
  HTHEME hTheme = OpenThemeData(NULL, L"CompositedWindow::Window");
  if (hTheme) {
    HDC hdcPaint = CreateCompatibleDC(hdc);
    if (hdcPaint) {
      int cx = rcClient.right - rcClient.left;
      int cy = rcClient.bottom -rcClient.top;

      // Define the BITMAPINFO structure used to draw text.
      // Note that biHeight is negative. This is done because
      // DrawThemeTextEx() needs the bitmap to be in top-to-bottom
      // order.
      BITMAPINFO dib = { 0 };
      dib.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
      dib.bmiHeader.biWidth           = cx;
      dib.bmiHeader.biHeight          = -cy;
      dib.bmiHeader.biPlanes          = 1;
      dib.bmiHeader.biBitCount        = 32;
      dib.bmiHeader.biCompression     = BI_RGB;

      HBITMAP hbm = CreateDIBSection(hdc, &dib, DIB_RGB_COLORS, NULL, NULL, 0);
      if (hbm) {
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcPaint, hbm);

        // Setup the theme drawing options.
        DTTOPTS DttOpts = {sizeof(DTTOPTS)};
        DttOpts.dwFlags = DTT_COMPOSITED | DTT_GLOWSIZE;
        DttOpts.iGlowSize = 15;

        // Select a font.
        LOGFONT lgFont;
        HFONT hFontOld = NULL;
        if (SUCCEEDED(GetThemeSysFont(hTheme, 801, &lgFont))) {
          HFONT hFont = CreateFontIndirect(&lgFont);
          hFontOld = (HFONT) SelectObject(hdcPaint, hFont);
        }

        // Draw the title.
        TCHAR szTitle[1024];
        GetWindowText(chrome_hwnd_, szTitle, 1024);
        RECT rcPaint = rcClient;
        rcPaint.top  = kOffsetSize;
        rcPaint.right = cx - VISTA_OFFSET_LEN - 2*TIP_BUTTON_WIDTH;
        rcPaint.left = kCaptionHeight;
        rcPaint.bottom = kCaptionHeight;
        DrawThemeTextEx(hTheme, hdcPaint, 0, 0, szTitle, -1, 
                        DT_LEFT | DT_WORD_ELLIPSIS, &rcPaint, 
                        &DttOpts);

        DrawIconEx(hdcPaint, kOffsetSize, kOffsetSize, 
                   (HICON)SendMessage(chrome_hwnd_, WM_GETICON, 0, 0), 
                   kIconSize, kIconSize, 0, NULL, DI_NORMAL | DI_COMPAT);
        tip_button_.OnPaint(hdcPaint);
        if (drawicon) {
          cx = kCaptionHeight;
          cy = kCaptionHeight;
          BitBlt(hdc, 0, 0, cx, cy, hdcPaint, 0, 0, SRCCOPY);
        } else {
          BitBlt(hdc, 0, 0, cx, kCaptionHeight, hdcPaint, 0, 0, SRCCOPY);
          BitBlt(hdc, 0, 0, kBorderWidth, cy, hdcPaint, 0, 0, SRCCOPY);
          BitBlt(hdc, cx-kBorderWidth, 0, kBorderWidth, cy, hdcPaint,
                 cx-kBorderWidth, 0, SRCCOPY);
          BitBlt(hdc, 0, cy-kBorderWidth, cx, kBorderWidth, hdcPaint, 0,
                 cy-kBorderWidth, SRCCOPY);
        }

        SelectObject(hdcPaint, hbmOld);
        if (hFontOld) {
          SelectObject(hdcPaint, hFontOld);
        }
        DeleteObject(hbm);
      }
      DeleteDC(hdcPaint);
    }
    CloseThemeData(hTheme);
  }
  ReleaseDC(chrome_hwnd_, hdc);
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
    case WM_NCPAINT:
      if (g_Chrome_MajorVersion >= 10 && g_Enable_DWM) {
        PaintCustomCaption(FALSE);
      }
      break;
    case WM_PAINT:
    case WM_ACTIVATE:
      if (g_Chrome_MajorVersion >= 10 || bFlag == ReadyResizeState ||
          bFlag == FinishResizeState)
        tip_button_.OnPaint();
      break;
    case WM_SETICON:
      if (g_Chrome_MajorVersion >= 10 && g_Enable_DWM) {
        PaintCustomCaption(TRUE);
      }
      break;
    case WM_DWMCOMPOSITIONCHANGED:
      {
        if (DwmIsCompositionEnabled(&g_Enable_DWM)) {
          DwmSetWindowAttribute(hwnd, DWMWA_ALLOW_NCPAINT, &g_Enable_DWM,
                                sizeof(g_Enable_DWM));
          if (g_Enable_DWM)
            SendMessage(hwnd, WM_NCPAINT, 0, 0);
        }
      }
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
            if (height < kMinChromeHeight)
              height = kMinChromeHeight;
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
      if (g_Chrome_MajorVersion < 10)
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
        if (g_Chrome_MajorVersion >= 10)
          break;

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
        if (height < kMinChromeHeight) {
          SetWindowPos(chrome_hwnd_, NULL, 0, 0, width, kMinChromeHeight, 
                       SWP_NOMOVE | SWP_NOREPOSITION);
          lParam = MAKELPARAM(width, kMinChromeHeight);
          height = kMinChromeHeight;
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