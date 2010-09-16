#include "stdafx.h"
#include "background_script_object.h"
#include "resource.h"
#include "log.h"
#include "script_object_factory.h"
#include <atlenc.h>

extern HMODULE g_hMod;
extern Log g_Log;

BackgroundScriptObject::BackgroundScriptObject(void) {
  image_ = NULL;
  stream_ = NULL;
}

BackgroundScriptObject::~BackgroundScriptObject(void) {

}

NPObject* BackgroundScriptObject::Allocate(NPP npp, NPClass *aClass) {
  BackgroundScriptObject* background_object = new BackgroundScriptObject;
  char logs[256];
  sprintf(logs, "this=%ld", background_object);
  g_Log.WriteLog("Allocate", logs);
  if (background_object != NULL) {
    background_object->SetPlugin((PluginBase*)npp->pdata);
    Function_Item item;
    strcpy(item.function_name, "SetWallPaper");
    item.function_pointer = ON_INVOKEHELPER(&BackgroundScriptObject::
        SetWallPaper);
    background_object->AddFunction(item);
    strcpy(item.function_name, "ApplyWallPaper");
    item.function_pointer = ON_INVOKEHELPER(&BackgroundScriptObject::
        ApplyWallPaper);
    background_object->AddFunction(item);
    strcpy(item.function_name, "RestoreWallPaper");
    item.function_pointer = ON_INVOKEHELPER(&BackgroundScriptObject::
        RestoreWallPaper);
    background_object->AddFunction(item);
  }
  return background_object;
}

void BackgroundScriptObject::Deallocate() {
  char logs[256];
  sprintf(logs, "this=%ld", this);
  g_Log.WriteLog("Deallocate", logs);
  delete this;
}

void BackgroundScriptObject::Invalidate() {

}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
  UINT  num = 0;          // number of image encoders
  UINT  size = 0;         // size of the image encoder array in bytes

  ImageCodecInfo* pImageCodecInfo = NULL;

  GetImageEncodersSize(&num, &size);
  if(size == 0)
    return -1;  // Failure

  pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
  if(pImageCodecInfo == NULL)
    return -1;  // Failure

  GetImageEncoders(num, size, pImageCodecInfo);

  for(UINT j = 0; j < num; ++j) {
    if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 ) {
      *pClsid = pImageCodecInfo[j].Clsid;
      free(pImageCodecInfo);
      return j;  // Success
    }
  }

  free(pImageCodecInfo);
  return -1;  // Failure
}

HWND g_WallPapperWindow = NULL;

void WINAPI TimerProc(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime) {
  TCHAR class_name[256];
  if (!GetClassName(g_WallPapperWindow, class_name, 256))
    return;

  if (wcscmp(class_name, L"Chrome_WidgetWin_0") == 0) {
    HWND hChildWnd = FindWindowEx(g_WallPapperWindow, NULL, 
      L"Chrome_AutocompleteEditView", NULL);
    if (hChildWnd)
      SendMessage(hChildWnd, WM_CLOSE, 0, 0);
    hChildWnd = FindWindowEx(g_WallPapperWindow, NULL, 
      L"Chrome_WidgetWin_0", NULL);
    int cx, cy;
    cx = CONST_FRAME_BORDER;
    cy = 25;
    static OSVERSIONINFO versionInfo = {0};
    if (versionInfo.dwMajorVersion == 0) {
      versionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      GetVersionEx(&versionInfo);
    }
    if (versionInfo.dwMajorVersion >= 6) {
      cx = 0;
      cy = 0;
    }

    SetWindowLong(g_WallPapperWindow, GWL_STYLE, 
      WS_CAPTION | WS_VISIBLE | DS_FIXEDSYS | WS_OVERLAPPED | WS_CLIPCHILDREN
      | WS_CLIPSIBLINGS | WS_SYSMENU | WS_MINIMIZEBOX);

    int width, height;
    static RECT rt = {0};
    if (rt.left == rt.right && rt.top == rt.bottom) {
      GetWindowRect(g_WallPapperWindow, &rt);
      width = rt.right - rt.left;
      height = rt.bottom - rt.top;
    } else {
      width = rt.right - rt.left;
      height = rt.bottom - rt.top;
      SetWindowPos(g_WallPapperWindow, NULL, 0, 0, width, height, 
                   SWP_NOMOVE | SWP_NOREPOSITION);
    }

    if (hChildWnd != NULL) {
      MoveWindow(hChildWnd, cx, cy, width-2*cx, height-cy-2*cx, TRUE);
    }
  }
}
 
bool BackgroundScriptObject::SetWallPaper(const NPVariant *args,
                                          uint32_t argCount,
                                          NPVariant *result) {
  BOOLEAN_TO_NPVARIANT(FALSE, *result);
  HRESULT hr;
  IActiveDesktop* active_desktop;
  g_WallPapperWindow = GetForegroundWindow();

  SetTimer(plugin_->get_hwnd(), 1, 100, TimerProc);

  hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER,
    IID_IActiveDesktop, (void**)&active_desktop);
  if (FAILED(hr))
    return false;

  hr = active_desktop->GetWallpaper(ori_wallpaper_, MAX_PATH, AD_GETWP_BMP);
  if (FAILED(hr))
    return false;
  ori_opt_.dwSize = sizeof(ori_opt_);
  hr = active_desktop->GetWallpaperOptions(&ori_opt_, 0);
  if (FAILED(hr))
    return false;

  active_desktop->Release();
  BOOLEAN_TO_NPVARIANT(TRUE, *result);

  return true; 
}

bool BackgroundScriptObject::Construct(const NPVariant *args,
                                       uint32_t argCount,
                                       NPVariant *result){
  return true;
}

bool BackgroundScriptObject::ApplyWallPaper(const NPVariant *args,
                                            uint32_t argCount,
                                            NPVariant *result) {
  BOOLEAN_TO_NPVARIANT(FALSE, *result);
  if (argCount != 2 || !NPVARIANT_IS_STRING(args[0]) ||
      !NPVARIANT_IS_STRING(args[1]))
    return false;
  
  char* type = (char*)NPVARIANT_TO_STRING(args[1]).UTF8Characters; 
  char* base64 = strstr((char*)NPVARIANT_TO_STRING(args[0]).UTF8Characters,
                        "base64,");
  if (!base64)
    return false;
  base64 += 7;
  int base64size = NPVARIANT_TO_STRING(args[0]).UTF8Length - 7;

  int byteLength = Base64DecodeGetRequiredLength(base64size);
  HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE,byteLength);
  if (!handle)
    return false;
  LPVOID bytes = GlobalLock(handle);
  if (!bytes)
    return false;
  Base64Decode(base64, base64size, (BYTE*)bytes, &byteLength);
  CreateStreamOnHGlobal(handle, FALSE, &stream_);
  image_ = new Image(stream_);
  stream_->Release();
  GlobalUnlock(handle);
  GlobalFree(handle);

  TCHAR current_path[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, current_path);
  TCHAR file_name[MAX_PATH];
  wsprintf(file_name, L"%s\\ExtensionWallPaper.bmp", current_path);
  CLSID bmp_clsid;
  GetEncoderClsid(L"image/bmp", &bmp_clsid);
  image_->Save(file_name,&bmp_clsid);

  delete image_;

  HRESULT hr;
  IActiveDesktop* active_desktop;

  hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER,
    IID_IActiveDesktop, (void**)&active_desktop);
  if (FAILED(hr))
    return false;

  hr = active_desktop->SetWallpaper(file_name, 0);
  if (FAILED(hr))
    return false;

  WALLPAPEROPT opt;
  opt.dwSize = sizeof(WALLPAPEROPT);
  if (_stricmp(type, "Center") == 0) {
    opt.dwStyle = WPSTYLE_CENTER;
    active_desktop->SetWallpaperOptions(&opt, 0);
  } else if (_stricmp(type, "Stretch") == 0) {
    opt.dwStyle = WPSTYLE_STRETCH;
    active_desktop->SetWallpaperOptions(&opt, 0);
  } else if (_stricmp(type, "Tile") == 0) {
    opt.dwStyle = WPSTYLE_TILE;
    active_desktop->SetWallpaperOptions(&opt, 0);
  }

  hr = active_desktop->ApplyChanges(AD_APPLY_ALL);
  if (FAILED(hr))
    return false;

  active_desktop->Release();
  BOOLEAN_TO_NPVARIANT(TRUE, *result);
  return true;
}

bool BackgroundScriptObject::RestoreWallPaper(const NPVariant *args,
                                              uint32_t argCount,
                                              NPVariant *result) {
  BOOLEAN_TO_NPVARIANT(FALSE, *result);
  HRESULT hr;
  IActiveDesktop *active_desktop;

  hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER,
    IID_IActiveDesktop, (void**)&active_desktop);
  if (FAILED(hr))
    return false;

  hr = active_desktop->SetWallpaper(ori_wallpaper_, 0);
  if (FAILED(hr))
    return false;

  hr = active_desktop->SetWallpaperOptions(&ori_opt_, 0);
  if (FAILED(hr))
    return false;

  hr = active_desktop->ApplyChanges(AD_APPLY_ALL);
  if (FAILED(hr))
    return false;

  active_desktop->Release();

  BOOLEAN_TO_NPVARIANT(TRUE, *result);
  return true;
}