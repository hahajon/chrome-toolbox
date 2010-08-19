#include "stdafx.h"
#include "Log.h"

CLog::CLog(void)
{
  file_ = NULL;
}

CLog::~CLog(void)
{
  if (file_ != NULL)
    CloseLog();
}

bool CLog::OpenLog(LPCSTR header)
{
  if (file_ != NULL)
    return false;

  char filename[MAX_PATH];
  GetLocalTime(&time_);
  sprintf_s(filename,"D:\\Log\\%s_%d%02d%02d_%d.log",
          header,time_.wYear,time_.wMonth,time_.wDay,
          GetProcessId(GetCurrentProcess()));
  file_ = fopen(filename,"a");
  if (file_ == NULL)
    return false;
  else
    return true;
}

bool CLog::WriteLog(LPCSTR title, LPCSTR contents)
{
  if (file_ == NULL)
  {
    return false;
  }

  GetLocalTime(&time_);
  sprintf_s(buffer_,"[%02d:%02d:%02d %03d] [%s] %s\n",
          time_.wHour,time_.wMinute,time_.wSecond,time_.wMilliseconds,
    title,contents);
  int nLen = strlen(buffer_);
  if (fwrite(buffer_,nLen,1,file_) == 1)
  {
    fflush(file_);
    return true;
  }
  else
  {
    return false;
  }
}

bool CLog::CloseLog()
{
  if (file_ != NULL) {
    fclose(file_);
    file_ = NULL;
  }
  return true;
}