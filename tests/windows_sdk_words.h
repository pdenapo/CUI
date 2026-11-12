/* Words the Windows SDK defines as macros, as a full <windows.h> (without
 * WIN32_LEAN_AND_MEAN or NOMINMAX) leaves them. CI force-includes this file when
 * it compiles the single-file distribution, so a variable, field or parameter
 * named after one of them fails on Linux too, not only for a Windows user who
 * includes <windows.h> first (issue #17). The values follow the SDK's headers. */
#ifndef CUI_WINDOWS_SDK_WORDS_H
#define CUI_WINDOWS_SDK_WORDS_H
/* rpcndr.h */
#define small char
#define hyper long long
/* minwindef.h, windef.h */
#define near
#define far
#define NEAR
#define FAR
#define pascal
#define cdecl
#define IN
#define OUT
#define OPTIONAL
#define CONST const
#define VOID void
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
/* combaseapi.h, objbase.h */
#define interface struct
#define PURE = 0
#define THIS void
/* wingdi.h */
#define ERROR 0
#define TRANSPARENT 1
#define OPAQUE 2
#define ABSOLUTE 1
#define RELATIVE 2
#define RGB(r, g, b) ((unsigned long)(((unsigned char)(r) | ((unsigned short)((unsigned char)(g)) << 8)) | (((unsigned long)(unsigned char)(b)) << 16)))
/* winnt.h, winbase.h, winuser.h */
#define DELETE (0x00010000L)
#define SYNCHRONIZE (0x00100000L)
#define IGNORE 0
#define INFINITE 0xFFFFFFFF
#define TEXT(quote) quote
#define MAX_PATH 260
#define CALLBACK
#define WINAPI
#define APIENTRY
#endif
