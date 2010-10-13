#include "stdafx.h"
#include "log.h"
#include "api_hook.h"
#include "mydsound.h"
#include "apihook.h"
#include <Mmdeviceapi.h>
#include <Audiopolicy.h>

HMODULE g_hMod;
Log g_Log;

#pragma data_seg("Shared")
BOOL g_BrowserMute = FALSE;
#pragma data_seg()
#pragma comment(linker, "/Section:Shared,rws")

BOOL g_exist = FALSE;
CRITICAL_SECTION g_CS;

#define CHECK_RESULT(hr)     if (FAILED(hr)) {\
  Sleep(100);\
  continue;\
}


DWORD WINAPI Muter_Thread(void* param) {
  CoInitialize(NULL);

  HRESULT hr = E_FAIL;
  IMMDeviceEnumerator* device_enumerator;
  IMMDevice* defaultdevice;
  IAudioSessionManager2* audio_session_mamanger2;
  IAudioSessionEnumerator* audio_session_enumerator;
  ISimpleAudioVolume* simple_audio_volume;

  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
    CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&device_enumerator);
  if (FAILED(hr))
    return hr;

  while(!g_exist) {
    CHECK_RESULT(device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, 
        &defaultdevice));

    CHECK_RESULT(defaultdevice->Activate(__uuidof(IAudioSessionManager2), 
        CLSCTX_ALL, NULL, (void**)&audio_session_mamanger2));

    CHECK_RESULT(audio_session_mamanger2->
        GetSessionEnumerator(&audio_session_enumerator));

    int count;
    CHECK_RESULT(audio_session_enumerator->GetCount(&count));

    for (int i = 0; i < count; i++) {
      IAudioSessionControl* audio_session_control;
      IAudioSessionControl2* audio_session_control2;
      CHECK_RESULT(audio_session_enumerator->
          GetSession(i, &audio_session_control));
      CHECK_RESULT(audio_session_control->
          QueryInterface(__uuidof(IAudioSessionControl2), 
                         (void**)&audio_session_control2));
      DWORD processid;
      CHECK_RESULT(audio_session_control2->GetProcessId(&processid));
      if (processid == GetCurrentProcessId()) {
        CHECK_RESULT(audio_session_control2->
            QueryInterface(__uuidof(ISimpleAudioVolume), 
                           (void**) &simple_audio_volume));
        CHECK_RESULT(simple_audio_volume->SetMute(g_BrowserMute, NULL));
        simple_audio_volume->Release();
      }
      audio_session_control->Release();
      audio_session_control2->Release();
    }

    audio_session_mamanger2->Release();
    audio_session_enumerator->Release();
    defaultdevice->Release();
    Sleep(1000);
  }

  return 0;
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
  g_hMod = hModule;
  OSVERSIONINFO versionInfo = {0};

  switch(reason) {
    case DLL_PROCESS_ATTACH:
      InitializeCriticalSection(&g_CS);
      g_Log.OpenLog("APIHOOK");
      versionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      GetVersionEx(&versionInfo);
      if (versionInfo.dwMajorVersion >= 6) 
        CreateThread(NULL, 0, Muter_Thread, 0, 0, NULL);
      g_Log.WriteLog("Msg", "DLL_PROCESS_ATTACH");
      break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      DeleteCriticalSection(&g_CS);
      g_exist = TRUE;
      g_Log.WriteLog("Msg", "DLL_PROCESS_DETACH");
      break;
  }
  return TRUE;
}

typedef int (WINAPI* PfnWaveOutWrite)(HWAVEOUT hwo, LPWAVEHDR pwh, 
                                      UINT cbwh);
typedef int (WINAPI* PfnMidiStreamOut)(HMIDISTRM hms, LPMIDIHDR pmh, 
                                       UINT cbmh);
typedef int (WINAPI* PfnDirectSoundCreate)(LPCGUID pcGuidDevice, 
                                           LPDIRECTSOUND *ppDS, 
                                           LPUNKNOWN pUnkOuter);
typedef int (WINAPI* PfnDirectSoundCreate8)(LPCGUID pcGuidDevice, 
                                            LPDIRECTSOUND8 *ppDS, 
                                            LPUNKNOWN pUnkOuter);

extern ApiHook g_waveOutWrite;
extern ApiHook g_DirectSoundCreate;
extern ApiHook g_DirectSoundCreate8;
extern ApiHook g_midiStreamOut;

MMRESULT WINAPI Hook_waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {
  if (g_BrowserMute) {
    memset(pwh->lpData, 0 , pwh->dwBufferLength);
  }

  MMRESULT result = ((PfnWaveOutWrite)(PROC)g_waveOutWrite)(hwo, pwh, cbwh);
  return result;
}

HRESULT WINAPI Hook_DirectSoundCreate(LPCGUID pcGuidDevice, 
                                      LPDIRECTSOUND *ppDS, 
                                      LPUNKNOWN pUnkOuter) {
  g_Log.WriteLog("msg", "Hook_DirectSoundCreate");
  HRESULT hr = ((PfnDirectSoundCreate)(PROC)g_DirectSoundCreate)
      (pcGuidDevice, ppDS, pUnkOuter);
  if (SUCCEEDED(hr)) {
    MyDirectSound* p = new MyDirectSound;
    p->direct_sound_ = *ppDS;
    *ppDS = p;
    g_Log.WriteLog("msg", "Hook_DirectSoundCreate Success");
  }
  return hr;
}

HRESULT WINAPI Hook_DirectSoundCreate8(LPCGUID pcGuidDevice, 
                                       LPDIRECTSOUND8 *ppDS, 
                                       LPUNKNOWN pUnkOuter) {
  g_Log.WriteLog("msg", "Hook_DirectSoundCreate8");
  HRESULT hr = ((PfnDirectSoundCreate8)(PROC)g_DirectSoundCreate8)
      (pcGuidDevice, ppDS, pUnkOuter);
  if (SUCCEEDED(hr)) {
    MyDirectSound8* p = new MyDirectSound8;
    p->direct_sound_ = *ppDS;
    *ppDS = p;
    g_Log.WriteLog("msg", "Hook_DirectSoundCreate8 Success");
  }
  return hr;
}

MMRESULT WINAPI Hook_midiStreamOut(HMIDISTRM hms, LPMIDIHDR pmh, UINT cbmh) {
  if (g_BrowserMute) {
    memset(pmh->lpData, 0 , pmh->dwBufferLength);
  }

  MMRESULT result = ((PfnMidiStreamOut)(PROC)g_midiStreamOut)(hms, pmh, cbmh);
  return result;
}

ApiHook g_DirectSoundCreate("dsound.dll", "DirectSoundCreate", 
                            (PROC)Hook_DirectSoundCreate, TRUE);
ApiHook g_DirectSoundCreate8("dsound.dll", "DirectSoundCreate8", 
                             (PROC)Hook_DirectSoundCreate8, TRUE);
ApiHook g_waveOutWrite("winmm.dll", "waveOutWrite", 
                       (PROC) Hook_waveOutWrite, TRUE);
ApiHook g_midiStreamOut("winmm.dll", "midiStreamOut", 
                        (PROC)Hook_midiStreamOut, TRUE);


void SetBrowserMute(BOOL flag) {
  g_BrowserMute = flag;
}