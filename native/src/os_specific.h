/**
 * @file "native/src/os_specific.h"
 * Operating-System-specific definitions and declarations
 */

#ifndef H_WEBCARD__OS_SPECIFIC
#define H_WEBCARD__OS_SPECIFIC

#ifdef __cplusplus
  extern "C" {
#endif


/**************************************************************/
/* HEADER FILES FOR SPECIFIC TARGETS (OPERATING SYSTEMS)      */
/**************************************************************/

#if defined(_WIN32)

  /** UTF-16 encoding + generic-type text support. */
  #define _UNICODE 1
  #define UNICODE 1
  #include <tchar.h>

  /** Windows API */
  #include <windows.h>

#elif defined(__linux__)

  #define _GNU_SOURCE 1

  /**
   * Standard C library, includes:
   *  `EXIT_SUCCESS`, `EXIT_FAILURE`,
   *  `malloc()`, `realloc()`, `free()`.
   */
  #include <stdlib.h>

  /** Linux File operations */
  #include <fcntl.h>
  #include <sys/stat.h>
  #include <sys/ioctl.h>
  #include <poll.h>
  #include <unistd.h>

  /** Linux Signals */
  #include <signal.h>

  /**
   * C library for strings, includes:
   *  `strlen()`, `memcpy()`.
   */
  #include <string.h>

#else
  #error("Unsupported operating system, sorry!")
#endif


/**************************************************************/
/* MORE STANDARD LIBRARIES, TYPE DEFINITIONS                  */
/**************************************************************/

/** Timing */
#include <time.h>

/** Fixed-width integer type definitions */
#include <stdint.h>

#if defined(_DEBUG)

  /** Standard C Input-output declarations. Required for `vsnprintf()`. */
  #include <stdio.h>

  /** Accessing variable-argument lists. */
  #include <stdarg.h>

  /** Printing-out error codes. */
  #include <errno.h>

#endif

#if defined(_WIN32)
  typedef HANDLE os_specific_stream_t;

#elif defined(__linux__)
  typedef int os_specific_stream_t;
  #include "wtypes_for_unix.h"

#endif

#ifndef _LPCBYTE_DEFINED
#define _LPCBYTE_DEFINED
typedef BYTE const * LPCBYTE;
#endif


/**************************************************************/
/* CUSTOM WIDE CHARS (exactly 16-bit wide)                    */
/**************************************************************/

#if defined(__linux__)

  extern size_t
  wcslen(_In_z_ LPCWSTR string);

#endif


/**************************************************************/
/* TERMINATING THE APPLICATION                                */
/**************************************************************/

/**
 * @brief Registers a termination handler.
 *
 * Termination handler is called when the application
 * is unexpectedly closed (forcefully terminated).
 * @return `TRUE` when OS-specific handler was successfully
 * registered, otherwise `FALSE`.
 */
extern BOOL
OSSpecific_registerTerminationHandler(void);


/**************************************************************/
/* PIPE OPERATIONS                                            */
/**************************************************************/

/**
 * @brief Validates if the process was launched with correct
 * types of standard streams.
 *
 * Both the Input Stream and the Output Stream should be PIPEs
 * (not disk files, nor keyboard/console).
 * @param[in] inputStream OS-specific stream that corresponds to `STDIN`.
 * @param[in] outputStream OS-specific stream that corresponds to `STDOUT`.
 * @return `TRUE` on successful validation, otherwise `FALSE`.
 */
extern BOOL
OSSpecific_validateTypesOfStreams(
  _In_ const os_specific_stream_t inputStream,
  _In_ const os_specific_stream_t outputStream);

/**
 * @brief Peeks how many bytes are available in a given stream.
 *
 * @param[in] stream OS-specific stream descriptor, open for reading.
 * @param[out] streamSize pointer to a location that receives number
 * of pending bytes.
 * @return `TRUE` on successful peek, `FALSE` on pipe-access errors.
 */
extern BOOL
OSSpecific_peekStream(
  _In_ const os_specific_stream_t stream,
  _Out_ size_t * streamSize);

/**
 * @brief Reads bytes from a stream.
 *
 * @param[in] stream OS-specific stream descriptor, open for reading.
 * @param[out] output Memory location where `size` of bytes will be stored.
 * @param[in] size Constant number of bytes to read from the stream.
 * @return `TRUE` on success, `FALSE` on any stream error.
 */
extern BOOL
OSSpecific_readBytesFromStream(
  _In_ const os_specific_stream_t stream,
  _Out_ LPVOID output,
  _In_ const size_t size);

/**
 * @brief Writes bytes to a stream.
 *
 * @param[in] stream OS-specific stream descriptor, open for writing.
 * @param[in] input Memory location from where `size` of bytes will be loaded.
 * @param[in] size Constant number of bytes to write to the stream.
 * @return `TRUE` on success, `FALSE` on any stream error.
 */
extern BOOL
OSSpecific_writeBytesToStream(
  _In_ const os_specific_stream_t stream,
  _In_ LPCVOID input,
  _In_ const size_t size);


/**************************************************************/
/* DEBUG DEFINITIONS AND DECLARATIONS                         */
/**************************************************************/

#if defined(_DEBUG)

  #define DEBUG_MESSAGE_START "-- "
  #define DEBUG_MESSAGE_END " --\n"

  /**
   * @brief Outputs a debug message (info or error).
   *
   * On all operating systems, the message is passed to `STDERR` stream.
   * On Windows, the message is also passed to `OutputDebugStringA` function.
   * @param[in] message A NULL-terminated UTF-8 string
   * that acts as a message format.
   * @param[in] ... variable arguments for message formatting.
   */
  extern VOID
  OSSpecific_writeDebugMessage(
    _In_z_ LPCSTR message,
    ...);

#endif

/**************************************************************/

#ifdef __cplusplus
  }
#endif

#endif  /* H_WEBCARD__OS_SPECIFIC */
