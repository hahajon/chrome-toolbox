#include "stdafx.h"
#include "api_hook.h"
#include "log.h"
#include <ImageHlp.h>
#pragma comment(lib, "ImageHlp")
#include <TlHelp32.h>

extern Log g_Log;
extern HMODULE g_hMod;
extern CRITICAL_SECTION g_CS;

// When an application runs on Windows 98 under a debugger, the debugger
// makes the module's import section point to a stub that calls the desired 
// function. To account for this, the code in this module must do some crazy 
// stuff. These variables are needed to help with the crazy stuff.


// The highest private memory address (used for Windows 98 only)
PVOID ApiHook::max_app_addr_ = NULL;
const BYTE cPushOpCode = 0x68;   // The PUSH opcode on x86 platforms


///////////////////////////////////////////////////////////////////////////////


// The head of the linked-list of ApiHook objects
ApiHook* ApiHook::head_pointer_ = NULL;

///////////////////////////////////////////////////////////////////////////////
BOOL g_IsInited = FALSE;

ApiHook::ApiHook(PSTR calleemodname, PSTR funcname, PROC pfnhook, BOOL flag) {
  if (!g_IsInited) {
    g_IsInited = TRUE;
    InitializeCriticalSection(&g_CS);
  }
  if (max_app_addr_ == NULL) {
    // Functions with address above lpMaximumApplicationAddress require
    // special processing (Windows 98 only)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    max_app_addr_ = si.lpMaximumApplicationAddress;
  }

  next_pointer_  = head_pointer_;    // The next node was at the head
  head_pointer_ = this;              // This node is now at the head

  // Save information about this hooked function
  callee_module_name_ = calleemodname;
  func_name_ = funcname;
  pfn_hook_ = pfnhook;
  exclude_hook_module_ = flag;
  pfn_orig_ = GetProcAddressRaw(GetModuleHandleA(callee_module_name_),
                                func_name_);
  
  if (pfn_orig_ > max_app_addr_) {
    // The address is in a shared DLL; the address needs fixing up 
    PBYTE pb = (PBYTE) pfn_orig_;
    if (pb[0] == cPushOpCode) {
      // Skip over the PUSH op code and grab the real address
      PVOID pv = * (PVOID*) &pb[1];
      pfn_orig_ = (PROC) pv;
    }
  }

  // Hook this function in all currently loaded modules
  ReplaceIATEntryInAllMods(callee_module_name_, pfn_orig_, pfn_hook_, 
                           exclude_hook_module_);
}


ApiHook::~ApiHook() {

  // Unhook this function from all modules
  if (pfn_orig_)
    ReplaceIATEntryInAllMods(callee_module_name_, pfn_hook_, pfn_orig_, 
        exclude_hook_module_);

  // Remove this object from the linked list
  ApiHook* p = head_pointer_; 
  if (!p)
    return;

  if (p == this) {     // Removing the head node
    head_pointer_ = p->next_pointer_; 
  } else {

    BOOL fFound = FALSE;

    // Walk list from head and fix pointers
    for (; !fFound && (p->next_pointer_ != NULL); p = p->next_pointer_) {
      if (p->next_pointer_ == this) { 
        // Make the node that points to us point to the our next node
        p->next_pointer_ = p->next_pointer_->next_pointer_; 
        break; 
      }
    }
  }
}


// NOTE: This function must NOT be inlined
FARPROC ApiHook::GetProcAddressRaw(HMODULE hmod, PCSTR procname) {

  return(::GetProcAddress(hmod, procname));
}

static HMODULE ModuleFromAddress(PVOID pv) {

  MEMORY_BASIC_INFORMATION mbi;
  return((VirtualQuery(pv, &mbi, sizeof(mbi)) != 0) 
    ? (HMODULE) mbi.AllocationBase : NULL);
}

void ApiHook::ReplaceIATEntryInAllMods(PCSTR calleemodulename, PROC pfnorig,
                                       PROC pfnhook, BOOL flag) {
  HMODULE thismod = flag ? ModuleFromAddress(ReplaceIATEntryInAllMods) : NULL;

  // Get the list of modules in this process
  HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 
                                      GetCurrentProcessId());
  if (h == INVALID_HANDLE_VALUE) {
    g_Log.WriteLog("Error", "CreateToolhelp32Snapshot Failed.");
    return;
  }

  MODULEENTRY32 me = { sizeof(me) };
  for (BOOL fOk = Module32First(h, &me); fOk; fOk = Module32Next(h, &me)) {
    // NOTE: We don't hook functions in our own module
    if (me.hModule != thismod) {
      // Hook this function in this module
      if (_tcsicmp(me.szModule, _T("convenience.dll")) == 0)
        continue;
      EnterCriticalSection(&g_CS);
      ReplaceIATEntryInOneMod(calleemodulename, pfnorig, pfnhook, me.hModule);
      LeaveCriticalSection(&g_CS);
    }
  }
}

void ApiHook::ReplaceIATEntryInOneMod(PCSTR calleemodulename, PROC pfnorig,
                                      PROC pfnhook, HMODULE hmodcaller) {
  // Get the address of the module's import section
  ULONG ulSize;
  PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)
      ImageDirectoryEntryToData(hmodcaller, TRUE, 
      IMAGE_DIRECTORY_ENTRY_IMPORT, &ulSize);
  
  if (pImportDesc == NULL)
    return;  // This module has no import section
  
  // Find the import descriptor containing references to callee's functions
  for (; pImportDesc->Name; pImportDesc++) {
    PSTR pszModName = (PSTR) ((PBYTE) hmodcaller + pImportDesc->Name);
    if (lstrcmpiA(pszModName, calleemodulename) == 0) 
      break;   // Found
  }
  
  if (pImportDesc->Name == 0)
    return;  // This module doesn't import any functions from this callee

  // Get caller's import address table (IAT) for the callee's functions
  PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA) 
      ((PBYTE) hmodcaller + pImportDesc->FirstThunk);

  // Replace current function address with new function address
  for (; pThunk->u1.Function; pThunk++) {
    // Get the address of the function address
    PROC* ppfn = (PROC*) &pThunk->u1.Function;
    // Is this the function we're looking for?
    BOOL fFound = (*ppfn == pfnorig);
    if (!fFound && (*ppfn > max_app_addr_)) {
      // If this is not the function and the address is in a shared DLL, 
      // then maybe we're running under a debugger on Windows 98. In this 
      // case, this address points to an instruction that may have the 
      // correct address.
      PBYTE pbInFunc = (PBYTE) *ppfn;
      if (pbInFunc[0] == cPushOpCode) {
        // We see the PUSH instruction, the real function address follows
        ppfn = (PROC*) &pbInFunc[1];
        // Is this the function we're looking for?
        fFound = (*ppfn == pfnorig);
      }
    }

    if (fFound) {
      // The addresses match, change the import section address
      DWORD oldAccess, newAccess;
      if (VirtualProtect(ppfn, sizeof(pfnhook), PAGE_EXECUTE_READWRITE, &oldAccess)) {
        if (!WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnhook, 
            sizeof(pfnhook), NULL)) {
          char logs[256];
          sprintf(logs, "WriteProcessMemory Failed,GetLastError=%ld, ppfn=%ld",
              GetLastError(),ppfn);
          g_Log.WriteLog("ReplaceIATError", logs);
        }
        if (!VirtualProtect(ppfn, sizeof(pfnhook), oldAccess, &newAccess)) {
          char logs[256];
          sprintf(logs, "VirtualProtect Failed 1, GetLastError=%ld", GetLastError());
          g_Log.WriteLog("Error", logs);
        }
      } else {
        char logs[256];
        sprintf(logs, "VirtualProtect Failed 2, GetLastError=%ld", GetLastError());
        g_Log.WriteLog("Error", logs);
      }
      return;  // We did it, get out
    }
  }
  // If we get to here, the function is not in the caller's import section
}

ApiHook ApiHook::sm_LoadLibraryA("Kernel32.dll", "LoadLibraryA",   
                                 (PROC) ApiHook::LoadLibraryA, TRUE);

ApiHook ApiHook::sm_LoadLibraryW("Kernel32.dll", "LoadLibraryW",   
                                 (PROC) ApiHook::LoadLibraryW, TRUE);

ApiHook ApiHook::sm_LoadLibraryExA("Kernel32.dll", "LoadLibraryExA", 
                                   (PROC) ApiHook::LoadLibraryExA, TRUE);

ApiHook ApiHook::sm_LoadLibraryExW("Kernel32.dll", "LoadLibraryExW", 
                                   (PROC) ApiHook::LoadLibraryExW, TRUE);

ApiHook ApiHook::sm_GetProcAddress("Kernel32.dll", "GetProcAddress", 
                                   (PROC) ApiHook::GetProcAddress, TRUE);

ApiHook ApiHook::sm_CreateProcessA("Kernel32.dll", "CreateProcessA", 
                                   (PROC) ApiHook::CreateProcessA, TRUE);

ApiHook ApiHook::sm_CreateProcessW("Kernel32.dll", "CreateProcessW", 
                                   (PROC) ApiHook::CreateProcessW, TRUE);

void ApiHook::FixupNewlyLoadedModule(HMODULE hmod, DWORD dwFlags) {

  // If a new module is loaded, hook the hooked functions
  EnterCriticalSection(&g_CS);
  if ((hmod != NULL) && ((dwFlags & LOAD_LIBRARY_AS_DATAFILE) == 0)) {
    for (ApiHook* p = head_pointer_; p != NULL; p = p->next_pointer_) {
      if (!p->pfn_orig_) {
        p->pfn_orig_ = GetProcAddressRaw(GetModuleHandleA(
            p->callee_module_name_), p->func_name_);
      }
      if (p->pfn_orig_) {
        ReplaceIATEntryInOneMod(p->callee_module_name_, 
          p->pfn_orig_, p->pfn_hook_, hmod);
      }
    }
  }
  LeaveCriticalSection(&g_CS);
}

HMODULE WINAPI ApiHook::LoadLibraryA(PCSTR pszModulePath) {
  HMODULE hmod = ::LoadLibraryA(pszModulePath);
  if (strstr(pszModulePath, "convenience.dll") != NULL)
    return hmod;
  if (hmod != ModuleFromAddress(ReplaceIATEntryInAllMods))
    FixupNewlyLoadedModule(hmod, 0);
  return(hmod);
}

HMODULE WINAPI ApiHook::LoadLibraryW(PCWSTR pszModulePath) {
  HMODULE hmod = ::LoadLibraryW(pszModulePath);
  if (wcsstr(pszModulePath, L"convenience.dll") != NULL)
    return hmod;
  if (hmod != ModuleFromAddress(ReplaceIATEntryInAllMods))
    FixupNewlyLoadedModule(hmod, 0);
  return(hmod);
}

HMODULE WINAPI ApiHook::LoadLibraryExA(PCSTR pszModulePath, 
                                       HANDLE hFile, DWORD dwFlags) {
  HMODULE hmod = ::LoadLibraryExA(pszModulePath, hFile, dwFlags);
  if (hmod != ModuleFromAddress(ReplaceIATEntryInAllMods))
    FixupNewlyLoadedModule(hmod, dwFlags);
  return(hmod);
}

HMODULE WINAPI ApiHook::LoadLibraryExW(PCWSTR pszModulePath, 
                                       HANDLE hFile, DWORD dwFlags) {
  HMODULE hmod = ::LoadLibraryExW(pszModulePath, hFile, dwFlags);
  if (hmod != ModuleFromAddress(ReplaceIATEntryInAllMods))
    FixupNewlyLoadedModule(hmod, dwFlags);
  return(hmod);
}

void InjectIntoProcess(HANDLE hprocess) {
  LPVOID p = VirtualAllocEx(hprocess, NULL, 
      MAX_PATH*sizeof(TCHAR), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

  char logs[256];
  if (p) {
    g_Log.WriteLog("Msg", "VirtualAllocEx");
    TCHAR dllpath[MAX_PATH];
    GetModuleFileName(g_hMod, dllpath, MAX_PATH);
    SIZE_T n ;
    g_Log.WriteLog("Msg", "WriteProcessMemory");
    BOOL ret = WriteProcessMemory(hprocess, p, dllpath, 
                                  MAX_PATH*sizeof(TCHAR), &n);
    if (!ret) {
      sprintf(logs, "WriteProcessMemory Failed,GetLastError=%ld", 
              GetLastError());
      g_Log.WriteLog("Error", logs);
    }
    g_Log.WriteLog("Msg", "CreateRemoteThread");
    HANDLE h = CreateRemoteThread(hprocess, NULL, 0,
        (LPTHREAD_START_ROUTINE)LoadLibrary, p, 0, NULL);
    if (h) {
      g_Log.WriteLog("Msg", "CreateRemoteThread Success");
      CloseHandle(h);
    } else {
      sprintf(logs, "CreateRemoteThread Failed,GetLastError=%ld", GetLastError());
      g_Log.WriteLog("Error", logs);
    }
  } else {
    sprintf(logs, "VirtualAllocEx Failed,GetLastError=%ld", GetLastError());
    g_Log.WriteLog("Error", logs);
  }
}

BOOL WINAPI ApiHook::CreateProcessA(LPCSTR lpApplicationName, 
                                    LPSTR lpCommandLine, 
                                    LPSECURITY_ATTRIBUTES lpProcessAttributes, 
                                    LPSECURITY_ATTRIBUTES lpThreadAttributes, 
                                    BOOL bInheritHandles, 
                                    DWORD dwCreationFlags,
                                    LPVOID lpEnvironment, 
                                    LPCSTR lpCurrentDirectory,
                                    LPSTARTUPINFOA lpStartupInfo,
                                    LPPROCESS_INFORMATION lpProcessInformation) {
  BOOL ret = ::CreateProcessA(lpApplicationName, lpCommandLine, 
      lpProcessAttributes, lpThreadAttributes, bInheritHandles, 
      dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,
      lpProcessInformation);
  if (ret) {
    char logs[256];
    sprintf(logs, "CreateProcessA, ProcessID=%ld", lpProcessInformation->dwProcessId);
    g_Log.WriteLog("Msg", logs);
    if (lpCommandLine != NULL && strstr(lpCommandLine, "--type=plugin") != 0)
      InjectIntoProcess(lpProcessInformation->hProcess);
  }
  return ret;
}

BOOL WINAPI ApiHook::CreateProcessW(LPCWSTR lpApplicationName, 
                                    LPWSTR lpCommandLine, 
                                    LPSECURITY_ATTRIBUTES lpProcessAttributes, 
                                    LPSECURITY_ATTRIBUTES lpThreadAttributes, 
                                    BOOL bInheritHandles, 
                                    DWORD dwCreationFlags,
                                    LPVOID lpEnvironment, 
                                    LPCWSTR lpCurrentDirectory,
                                    LPSTARTUPINFOW lpStartupInfo,
                                    LPPROCESS_INFORMATION lpProcessInformation) {
  BOOL ret = ::CreateProcessW(lpApplicationName, lpCommandLine, 
      lpProcessAttributes, lpThreadAttributes, bInheritHandles, 
      dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,
      lpProcessInformation);
  if (ret) {
    char logs[256];
    sprintf(logs, "CreateProcessW, ProcessID=%ld", lpProcessInformation->dwProcessId);
    g_Log.WriteLog("Msg", logs);
    if (lpCommandLine != NULL && wcsstr(lpCommandLine, L"--type=plugin") != 0)
      InjectIntoProcess(lpProcessInformation->hProcess);
  }
  return ret;
}

FARPROC WINAPI ApiHook::GetProcAddress(HMODULE hmod, PCSTR pszProcName) {

  // Get the true address of the function
  FARPROC pfn = GetProcAddressRaw(hmod, pszProcName);
  // Is it one of the functions that we want hooked?
  ApiHook* p = head_pointer_;
  for (; (pfn != NULL) && (p != NULL); p = p->next_pointer_) {

    if (pfn == p->pfn_orig_) {

      // The address to return matches an address we want to hook
      // Return the hook function address instead
      pfn = p->pfn_hook_;
      break;
    }
  }

  return(pfn);
}