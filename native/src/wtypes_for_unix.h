/**
 * @file "native/src/wtypes_for_unix.h"
 * List of Windows Data Types.
 * `<stdint.h>` must be included before including this header file.
 */

/**
 * Same guard as in "PCSC Lite" library,
 * to avoid conflits and typedef redefinitions.
 */
#ifndef __wintypes_h__
#define __wintypes_h__

/**************************************************************/

#define FALSE  0
#define TRUE   1

typedef void VOID;
typedef VOID * LPVOID;
typedef VOID const * LPCVOID;

typedef int BOOL;

typedef int32_t LONG;

typedef float FLOAT;

typedef uint32_t DWORD;
typedef DWORD * LPDWORD;

typedef unsigned char BYTE;
typedef BYTE * LPBYTE;

typedef char CHAR;
typedef CHAR * LPSTR;
typedef CHAR const * LPCSTR;

typedef uint16_t WCHAR;
typedef WCHAR * LPWSTR;
typedef WCHAR const * LPCWSTR;

typedef char TCHAR;
typedef LPSTR LPTSTR;
typedef LPCSTR LPCTSTR;
#define _tcslen  strlen

/** Ignore "SAL" (Microsoft "Source Code Annotation") */
#define _In_
#define _Out_
#define _Inout_
#define _In_z_
#define _In_opt_
#define _Outptr_result_maybenull_

/**************************************************************/

#endif  /* __wintypes_h__ */