#include "stdafx.h"
#include "ncbutton.h"
#include "log.h"
#include <CommCtrl.h>
#include "video_alone_script_object.h"

extern Log g_Log;
extern HMODULE g_hMod;
extern Local_Message_Item g_Local_Message;

NCButton::NCButton(void) {
  is_topmost_ = false;
  Gdiplus::GdiplusStartup(&token_, &start_input_, NULL);
}

NCButton::~NCButton(void) {
  Gdiplus::GdiplusShutdown(token_);
}

void NCButton::Init(HWND parenthwnd) {
  parent_hwnd_ = parenthwnd;
  mask_dc_ = NULL;
  mask_bitmap_ = NULL;

  GetButtonRect();

  TCHAR filename[MAX_PATH];
  TCHAR png_filename[MAX_PATH];
  GetModuleFileName(g_hMod, filename, MAX_PATH);
  wchar_t* postfix = wcsrchr(filename, '\\');
  if (postfix)
    *postfix = 0;

  wsprintf(png_filename, L"%s\\resources\\tip_normal.png", filename);
  normal_image_ = new Image(png_filename);
  wsprintf(png_filename, L"%s\\resources\\tip_mouseover.png", filename);
  mouse_over_image_ = new Image(png_filename);
  wsprintf(png_filename, L"%s\\resources\\tip_mousedown.png", filename);
  mouse_down_image_ = new Image(png_filename);
  wsprintf(png_filename, L"%s\\resources\\notip_normal.png", filename);
  notip_normal_image_ = new Image(png_filename);
  wsprintf(png_filename, L"%s\\resources\\notip_mouseover.png", filename);
  notip_mouseover_image_ = new Image(png_filename);
  wsprintf(png_filename, L"%s\\resources\\notip_mousedown.png", filename);
  notip_mousedown_image_ = new Image(png_filename);

  grph_ = Graphics::FromHWND(parent_hwnd_);

  SetWindowPos(parent_hwnd_, HWND_TOPMOST, 0, 0, 0, 0,
               SWP_NOSIZE | SWP_NOMOVE);

  is_topmost_ = true;
}

void NCButton::OnPaint() {
  POINT pt;

  GetButtonRect();

  pt.x = rect_.left;
  pt.y = rect_.top;
  ScreenToClient(parent_hwnd_, &pt);
  if (grph_ != NULL)
    delete grph_;
  grph_ = Graphics::FromHWND(parent_hwnd_);

  if (mask_dc_) {
    DeleteDC(mask_dc_);
    DeleteObject(mask_bitmap_);
  }

  HDC hdc = grph_->GetHDC();
  mask_dc_ = CreateCompatibleDC(hdc);
  mask_bitmap_ = CreateCompatibleBitmap(hdc, normal_image_->GetWidth(),
                                        normal_image_->GetHeight());
  SelectObject(mask_dc_, mask_bitmap_);
  if (IsMaximized(parent_hwnd_))
    BitBlt(mask_dc_, 0, 0, normal_image_->GetWidth(), normal_image_->GetHeight(),
           hdc, pt.x-TIP_BUTTON_WIDTH-CONST_FRAME_BORDER, pt.y, SRCCOPY);
  else {
    RECT rt;
    GetClientRect(parent_hwnd_, &rt);
    BitBlt(mask_dc_, 0, 0, normal_image_->GetWidth(), normal_image_->GetHeight(),
           hdc, rt.right-TIP_BUTTON_WIDTH-CONST_FRAME_BORDER, 
           pt.y + TIP_BUTTON_HEIGHT, SRCCOPY);
  }
  grph_->ReleaseHDC(hdc);

  HDC hdc_temp = CreateCompatibleDC(mask_dc_);
  HBITMAP bitmap = CreateCompatibleBitmap(mask_dc_, normal_image_->GetWidth(), 
                                          normal_image_->GetHeight());
  SelectObject(hdc_temp, bitmap);
  BitBlt(hdc_temp, 0, 0, normal_image_->GetWidth(), normal_image_->GetWidth(),
         mask_dc_, 0, 0, SRCCOPY);
  Graphics g(hdc_temp);
  
  if (is_topmost_) {
    Status state = g.DrawImage(mouse_down_image_, 
        Rect(0, 0, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT), 0, 0, 
        mouse_down_image_->GetWidth(), mouse_down_image_->GetHeight(), 
        UnitPixel);
  } else if (button_state_ == BUTTON_STATE_MOUSEOVER) {
    Status state = g.DrawImage(notip_mouseover_image_, 
        Rect(0, 0, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT), 0, 0,
        mouse_over_image_->GetWidth(), mouse_over_image_->GetHeight(), 
        UnitPixel);
    button_state_ = BUTTON_STATE_MOUSEOVER;
  } else {
    Status state = g.DrawImage(notip_normal_image_, 
        Rect(0, 0, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT), 0, 0,
        normal_image_->GetWidth(), normal_image_->GetHeight(), 
        UnitPixel);
    button_state_ = BUTTON_STATE_NORMAL;
  }
  HDC destdc = grph_->GetHDC();
  BitBlt(destdc, pt.x, pt.y, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT, 
         hdc_temp, 0, 0, SRCCOPY);
  grph_->ReleaseHDC(destdc);
  DeleteDC(hdc_temp);
  DeleteObject(bitmap);
}

void NCButton::OnMouseLeave() {
  OnPaint();
}

void NCButton::OnMouseDown(POINT pt) {
  GetButtonRect();

  if (PtInRect(&rect_, pt)) {
    POINT pt;
    pt.x = rect_.left;
    pt.y = rect_.top;
    ScreenToClient(parent_hwnd_, &pt);
    HDC hdc = grph_->GetHDC();
    BitBlt(hdc, pt.x, pt.y, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT, 
           mask_dc_, 0, 0, SRCCOPY);
    grph_->ReleaseHDC(hdc);
    if (is_topmost_) {
      SetWindowPos(parent_hwnd_, HWND_NOTOPMOST, 0, 0, 0, 0, 
                   SWP_NOSIZE | SWP_NOMOVE);

      Status state = grph_->DrawImage(mouse_down_image_, 
          Rect(pt.x, pt.y, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT), 0, 0,
          normal_image_->GetWidth(), normal_image_->GetHeight(),UnitPixel);
      is_topmost_ = false;
    } else {
      SetWindowPos(parent_hwnd_, HWND_TOPMOST, 0, 0, 0, 0, 
                   SWP_NOSIZE | SWP_NOMOVE);
      Status state = grph_->DrawImage(notip_mousedown_image_, 
          Rect(pt.x, pt.y, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT), 0, 0,
          mouse_down_image_->GetWidth(), mouse_down_image_->GetHeight(),
          UnitPixel);
      is_topmost_ = true;
    }
    button_state_ = BUTTON_STATE_MOUSEDOWN;
  }
}

void NCButton::OnMouseOver(POINT pt) {  
  static HWND hwnd_tooltip = NULL;
  static TOOLINFO toolinfo = {0};

  GetButtonRect();

  if (PtInRect(&rect_, pt) && button_state_ == BUTTON_STATE_NORMAL) {
    POINT pt;
    pt.x = rect_.left;
    pt.y = rect_.top;
    ScreenToClient(parent_hwnd_, &pt);
    HDC hdc = grph_->GetHDC();
    BitBlt(hdc, pt.x, pt.y, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT, 
           mask_dc_, 0, 0, SRCCOPY);
    grph_->ReleaseHDC(hdc);
    if (is_topmost_)
      grph_->DrawImage(mouse_over_image_, 
          Rect(pt.x, pt.y, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT), 0, 0,
          mouse_over_image_->GetWidth(), mouse_over_image_->GetHeight(),
          UnitPixel);
    else
      grph_->DrawImage(notip_mouseover_image_, 
          Rect(pt.x, pt.y, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT), 0, 0,
          notip_mouseover_image_->GetWidth(), notip_mouseover_image_->GetHeight(),
          UnitPixel);

    if (!hwnd_tooltip) {
      INITCOMMONCONTROLSEX iccex; 

      iccex.dwICC = ICC_WIN95_CLASSES;
      iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
      InitCommonControlsEx(&iccex);

      hwnd_tooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
          WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, 
          CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, 
          g_hMod, NULL);

      SetWindowPos(hwnd_tooltip, HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    if (hwnd_tooltip) {
      toolinfo.cbSize = sizeof(TOOLINFO);
      toolinfo.uFlags = TTF_SUBCLASS;
      toolinfo.hwnd = parent_hwnd_;
      toolinfo.hinst = g_hMod;
      toolinfo.uId = 0;
      toolinfo.lpszText = g_Local_Message.msg_always_on_top;
      toolinfo.rect.left = pt.x;
      toolinfo.rect.top = pt.y;
      pt.x = rect_.right;
      pt.y = rect_.bottom;
      ScreenToClient(parent_hwnd_, &pt);
      toolinfo.rect.right = pt.x;
      toolinfo.rect.bottom = pt.y;
      SendMessage(hwnd_tooltip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &toolinfo);	
    }

    button_state_ = BUTTON_STATE_MOUSEOVER;
  } else if (!PtInRect(&rect_, pt) && button_state_ != BUTTON_STATE_NORMAL) {
    toolinfo.hwnd = parent_hwnd_;
    SendMessage(hwnd_tooltip, TTM_DELTOOL, 0, (LPARAM) (LPTOOLINFO) &toolinfo);	
    button_state_ = BUTTON_STATE_NORMAL;
    OnPaint();
  }
}

void NCButton::OnMouseUp(POINT pt) {  
  GetButtonRect();

  if (PtInRect(&rect_, pt)) {
    POINT pt;
    pt.x = rect_.left;
    pt.y = rect_.top;
    ScreenToClient(parent_hwnd_, &pt);
    HDC hdc = grph_->GetHDC();
    BitBlt(hdc, pt.x, pt.y, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT,
           mask_dc_, 0, 0, SRCCOPY);
    grph_->ReleaseHDC(hdc);
    if (is_topmost_)
      grph_->DrawImage(mouse_over_image_, 
          Rect(pt.x, pt.y, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT), 0, 0,
          mouse_over_image_->GetWidth(), mouse_over_image_->GetHeight(),
          UnitPixel);
    else
      grph_->DrawImage(notip_mouseover_image_, 
          Rect(pt.x, pt.y, TIP_BUTTON_WIDTH, TIP_BUTTON_HEIGHT), 0, 0,
          notip_mouseover_image_->GetWidth(), notip_mouseover_image_->GetHeight(),
          UnitPixel);
    button_state_ = BUTTON_STATE_MOUSEOVER;
  }
}

void NCButton::GetButtonRect() {
  RECT rt;

  static OSVERSIONINFO versionInfo = { 0 };
  if (versionInfo.dwMajorVersion == 0) {
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&versionInfo);
  }

  GetWindowRect(parent_hwnd_, &rt);
  if (IsMaximized(parent_hwnd_)) {
    if (versionInfo.dwMajorVersion >= 6)
      rect_.right = rt.right - VISTA_OFFSET_LEN - 2*CONST_FRAME_BORDER;
    else
      rect_.right = rt.right - OFFSET_LEN - 2*CONST_FRAME_BORDER;
    rt.top = 0;
  } else {
    if (versionInfo.dwMajorVersion >= 6)
      rect_.right = rt.right - VISTA_OFFSET_LEN; 
    else
      rect_.right = rt.right - OFFSET_LEN; 
    rt.top++;
  }
  rect_.top = rt.top;
  rect_.left = rect_.right - TIP_BUTTON_WIDTH;
  rect_.bottom = rect_.top + TIP_BUTTON_HEIGHT;
}