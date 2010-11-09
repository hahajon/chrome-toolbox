#include "stdafx.h"
#include "video_alone_plugin.h"
#include "npapi.h"
#include "script_object_factory.h"
#include "video_alone_script_object.h"

WNDPROC VideoAlonePlugin::old_proc_ = NULL;

VideoAlonePlugin::VideoAlonePlugin(void) {
  Gdiplus::GdiplusStartup(&token_, &start_input_, NULL);
}

VideoAlonePlugin::~VideoAlonePlugin(void) {
  Gdiplus::GdiplusShutdown(token_);
}

NPError VideoAlonePlugin::Init(NPP instance, uint16_t mode, int16_t argc,
                               char *argn[], char *argv[], 
                               NPSavedData *saved) {
  script_object_ = NULL;
  old_proc_ = NULL;
  instance->pdata = this;
  return PluginBase::Init(instance, mode, argc, argn, argv, saved);
}

NPError VideoAlonePlugin::UnInit(NPSavedData **save) {
  PluginBase::UnInit(save);
  script_object_ = NULL;
  old_proc_ = NULL;
  return NPERR_NO_ERROR;
}

NPError VideoAlonePlugin::GetValue(NPPVariable variable, void *value) {
  switch(variable) {
    case NPPVpluginScriptableNPObject:
      if (script_object_ == NULL) {
        script_object_ = ScriptObjectFactory::CreateObject(npp_,
            VideoAloneScriptObject::Allocate);
      }
      if (script_object_ != NULL) {
        *(NPObject**)value = script_object_;
      }
      else
        return NPERR_OUT_OF_MEMORY_ERROR;
      break;
    default:
      return NPERR_GENERIC_ERROR;
  }

  return NPERR_NO_ERROR;
}

NPError VideoAlonePlugin::SetWindow(NPWindow* window) {
  PluginBase::SetWindow(window);

  if (hwnd_ == NULL && old_proc_ != NULL) {
    SubclassWindow(hwnd_, old_proc_);
    old_proc_ = NULL;
  }

  if (hwnd_ != NULL && old_proc_ == NULL) {
    old_proc_ = SubclassWindow(hwnd_, WndProc);
    SetWindowLong(hwnd_, GWLP_USERDATA, (LONG)this);
  }

  return NPERR_NO_ERROR;
}

LRESULT VideoAlonePlugin::WndProc(HWND hWnd, UINT Msg, 
                                   WPARAM wParam, LPARAM lParam) {
  switch(Msg) {
    case WM_CHROMECLOSE:
      {
        VideoAlonePlugin* pPlugin = (VideoAlonePlugin*)GetWindowLong(hWnd, 
            GWLP_USERDATA);
        VideoAloneScriptObject* pVObject = (VideoAloneScriptObject*)
              pPlugin->script_object_;

        void* pValue = NULL;
        NPN_GetValue(pPlugin->npp_, NPNVWindowNPObject, &pValue);

        NPObject* pObject = (NPObject*)pValue;
        VideoAloneScriptObject::WindowMap::iterator item = pVObject->
            window_list_.find(HWND(wParam));
        if (item != pVObject->window_list_.end()) {
          NPVariant params[3];
          INT32_TO_NPVARIANT(item->second.parent_window_id, params[0]);
          INT32_TO_NPVARIANT(item->second.window_id, params[1]);
          INT32_TO_NPVARIANT(item->second.tab_id, params[2]);

          NPVariant varObject;
          NPVariant vRet;
          NPN_GetProperty(pPlugin->npp_, pObject,
                          NPN_GetStringIdentifier("videoAlone"), &varObject);
          NPN_Invoke(pPlugin->npp_, varObject.value.objectValue,
                     NPN_GetStringIdentifier("restore"), params, 3, &vRet);
          NPN_ReleaseVariantValue(&vRet);
          NPN_ReleaseVariantValue(&varObject);
        }
      }
      return 1;
    default:
      return CallWindowProc(old_proc_, hWnd, Msg, wParam, lParam);
  }
}