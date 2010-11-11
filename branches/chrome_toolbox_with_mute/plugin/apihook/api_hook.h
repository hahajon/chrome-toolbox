#pragma once

class ApiHook {
public:
  // Hook a function in all modules
  ApiHook(PSTR calleemodname, PSTR funcname, PROC pfnhook, 
          BOOL flag);

  // Unhook a function from all modules
  ~ApiHook();

  // Returns the original address of the hooked function
  operator PROC() { return(pfn_orig_); }


public:
  // Calls the real GetProcAddress 
  static FARPROC WINAPI GetProcAddressRaw(HMODULE hmod, PCSTR procname);

private:
  static PVOID max_app_addr_;       // Maximum private memory address
  static ApiHook* head_pointer_;    // Address of first object
  ApiHook* next_pointer_;            // Address of next  object

  PCSTR callee_module_name_;        // Module containing the function (ANSI)
  PCSTR func_name_;                 // Function name in callee (ANSI)
  PROC  pfn_orig_;                  // Original function address in callee
  PROC  pfn_hook_;                  // Hook function address
  BOOL  exclude_hook_module_;       // Hook module w/CAPIHook implementation?

private:
  // Replaces a symbol's address in a module's import section
  static void WINAPI ReplaceIATEntryInAllMods(PCSTR calleemodulename,
      PROC pfnorig, PROC pfnhook, BOOL flag);

  // Replaces a symbol's address in all module's import sections
  static void WINAPI ReplaceIATEntryInOneMod(PCSTR calleemodulename, 
      PROC pfnorig, PROC pfnhook, HMODULE hmodcaller);

private:
  // Used when a DLL is newly loaded after hooking a function
  static void WINAPI FixupNewlyLoadedModule(HMODULE hmod, DWORD flags);

  // Used to trap when DLLs are newly loaded
  static HMODULE WINAPI LoadLibraryA(PCSTR  pszModulePath);
  static HMODULE WINAPI LoadLibraryW(PCWSTR pszModulePath);
  static HMODULE WINAPI LoadLibraryExA(PCSTR  pszModulePath, 
      HANDLE hFile, DWORD dwFlags);
  static HMODULE WINAPI LoadLibraryExW(PCWSTR pszModulePath, 
      HANDLE hFile, DWORD dwFlags);
  static BOOL WINAPI CreateProcessA(LPCSTR lpApplicationName,
      LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
      LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
      DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,
      LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
  static BOOL WINAPI CreateProcessW(LPCWSTR lpApplicationName,
      LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
      LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
      DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
      LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

  // Returns address of replacement function if hooked function is requested
  static FARPROC WINAPI GetProcAddress(HMODULE hmod, PCSTR pszProcName);

private:
  // Instantiates hooks on these functions
  static ApiHook sm_LoadLibraryA;
  static ApiHook sm_LoadLibraryW;
  static ApiHook sm_LoadLibraryExA;
  static ApiHook sm_LoadLibraryExW;
  static ApiHook sm_CreateProcessA;
  static ApiHook sm_CreateProcessW;
  static ApiHook sm_GetProcAddress;

};

void InjectIntoProcess(HANDLE hprocess);