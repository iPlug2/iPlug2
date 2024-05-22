/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#pragma once

#include <IPlugPlatform.h>

#include <string>
#include <wdlstring.h>

#ifdef OS_WIN
#include <IPlugUtilities.h>
#include <windows.h>

typedef struct _stat64i32 StatType;

static inline int statUTF8(const char* path, StatType* pStatbuf)
{
  return _wstat(iplug::UTF8AsUTF16(path).Get(), pStatbuf);
}

static HMENU GetCurrentMenu(HWND hwnd)
{
  return GetMenu(hwnd);
}

class ItemStringUTF8
{
public:
  ItemStringUTF8(const char* str) : mStr(str) {}
  ItemStringUTF8(const WDL_String& str) : mStr(str.Get()) {}
  ItemStringUTF8(const std::string& str) : mStr(str.c_str()) {}
  
  operator LPARAM() { return (LPARAM) mStr.Get(); }
  
private:
  iplug::UTF8AsUTF16 mStr;
};

static LRESULT SendDlgItemMessageUTF8(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return SendDlgItemMessageW(hDlg, nIDDlgItem, Msg, wParam, lParam);
}

static void GetComboStringUTF8(WDL_String& str, HWND hDlg, int item, WPARAM idx)
{
  std::wstring tempString;
  tempString.reserve(static_cast<long>(SendDlgItemMessageUTF8(hDlg, item, CB_GETLBTEXTLEN, idx, 0)) + 1);
  SendDlgItemMessageUTF8(hDlg, item, CB_GETLBTEXT, idx, (LPARAM) tempString.data());
  iplug::UTF16ToUTF8(str, tempString.c_str());
}

static int GetPrivateProfileIntUTF8(const char* appname, const char* keyname, int def, const char* fn)
{
  return GetPrivateProfileIntA(appname, keyname, def, fn);
}

static DWORD GetPrivateProfileStringUTF8(const char* appname, const char* keyname, const char* def, char *ret, int retsize, const char* fn)
{
  return GetPrivateProfileStringA(appname, keyname, def, ret, retsize, fn);
}

static BOOL WritePrivateProfileStringUTF8(const char* appname, const char* keyname, const char* val, const char* fn)
{
  return WritePrivateProfileStringA(appname, keyname, val, fn);
}

static int MessageBoxUTF8(HWND hwndParent, const char* text, const char* caption, int type)
{
  return MessageBoxW(hwndParent, iplug::UTF8AsUTF16(text).Get(), iplug::UTF8AsUTF16(caption).Get(), type);
}

#else

#include <sys/stat.h>
#include "IPlugSWELL.h"

typedef struct stat StatType;

static inline int statUTF8(const char* path, StatType* pStatbuf)
{
  return stat(path, pStatbuf);
}

static HMENU GetCurrentMenu(HWND hwnd)
{
  return SWELL_GetCurrentMenu();
}

class ItemStringUTF8
{
public:
  ItemStringUTF8(const char* str) : mStr(str) {}
  ItemStringUTF8(const WDL_String& str) : mStr(str.Get()) {}
  ItemStringUTF8(const std::string& str) : mStr(str.c_str()) {}
  
  operator LPARAM() { return (LPARAM) mStr; }
  
private:
  const char* mStr;
};

static LRESULT SendDlgItemMessageUTF8(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return SendDlgItemMessage(hDlg, nIDDlgItem, Msg, wParam, lParam);
}

static void GetComboStringUTF8(WDL_String& str, HWND hDlg, int item, WPARAM idx)
{
  std::string tempString;
  tempString.reserve(static_cast<long>(SendDlgItemMessageUTF8(hDlg, item, CB_GETLBTEXTLEN, idx, 0)) + 1);
  SendDlgItemMessageUTF8(hDlg, item, CB_GETLBTEXT, idx, (LPARAM) tempString.data());
  str.Set(tempString.c_str());
}

static int GetPrivateProfileIntUTF8(const char* appname, const char* keyname, int def, const char* fn)
{
  return GetPrivateProfileInt(appname, keyname, def, fn);
}

static DWORD GetPrivateProfileStringUTF8(const char* appname, const char* keyname, const char* def, char *ret, int retsize, const char* fn)
{
  return GetPrivateProfileString(appname, keyname, def, ret, retsize, fn);
}

static BOOL WritePrivateProfileStringUTF8(const char* appname, const char* keyname, const char* val, const char* fn)
{
  return WritePrivateProfileString(appname, keyname, val, fn);
}

static int MessageBoxUTF8(HWND hwndParent, const char* text, const char* caption, int type)
{
  return MessageBox(hwndParent, text, caption, type);
}

#endif
