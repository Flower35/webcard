/**
 * @file "native/src/os_specific.c"
 * Operating-System-specific functions
 */

#include "os_specific.h"

/**************************************************************/

#if defined(_WIN32)

  /**
   * @brief This is a termination handler.
   *
   * This function that exists only when the target OS is Windows.
   * @param ctrlType Parameter for this type of termination handler.
   * @return `TRUE` if the `ctrlType` is recognized by the handler.
   * Otherwise the OS handles this event.
   */
  BOOL
  Win32_terminationHandler(DWORD ctrlType)
  {
    #if defined(_DEBUG)
    {
      switch (ctrlType)
      {
        case CTRL_C_EVENT:
          OSSpecific_writeDebugMessage(
            "{termination handler}: {CTRL_C_EVENT}");
          return TRUE;

        case CTRL_CLOSE_EVENT:
          OSSpecific_writeDebugMessage(
            "{termination handler}: {CTRL_CLOSE_EVENT}");
          return TRUE;

        case CTRL_BREAK_EVENT:
          OSSpecific_writeDebugMessage(
            "{termination handler}: {CTRL_BREAK_EVENT}");
          return FALSE;
      }
    }
    #endif

    return FALSE;
  }

#endif

/**************************************************************/

#if defined(__linux__)

  /**
   * @brief This is a termination handler.
   *
   * This function that exists only when the target OS is Linux.
   * @param signalNumber Parameter for this type of termination handler.
   */
  VOID
  Linux_terminationHandler(int signalNumber)
  {
    #if defined(_DEBUG)
    {
      switch (signalNumber)
      {
        case SIGINT:
          OSSpecific_writeDebugMessage(
            "{termination handler}: {SIGINT}");
          exit(EXIT_FAILURE);
          break;
      }
    }
    #endif
  }

#endif

/**************************************************************/

BOOL
OSSpecific_registerTerminationHandler(void)
{
  #if defined(_WIN32)
  {
    return SetConsoleCtrlHandler(Win32_terminationHandler, TRUE);
  }
  #elif defined(__linux__)
  {
    sighandler_t signal_handler;

    signal_handler = signal(SIGINT, Linux_terminationHandler);
    if (SIG_ERR == signal_handler) { return FALSE; }

    signal_handler = signal(SIGPIPE, SIG_IGN);
    return (SIG_ERR != signal_handler);
  }
  #else
  {
    return FALSE;
  }
  #endif
}

/**************************************************************/

#if defined(__linux__)

  size_t
  wcslen(_In_z_ LPCWSTR string)
  {
    size_t size = 0;

    while ('\0' != string[size])
    {
      size++;
    }

    return size;
  }

#endif

/**************************************************************/

BOOL
OSSpecific_validateTypesOfStreams(
  _In_ const os_specific_stream_t inputStream,
  _In_ const os_specific_stream_t outputStream)
{
  #if defined(_WIN32)
  {
    if (FILE_TYPE_PIPE != GetFileType(inputStream))
    {
      #if defined(_DEBUG)
      OSSpecific_writeDebugMessage(
        "Expected {Standard Input} type to be a pipe");
      #endif

      return FALSE;
    }

    if (FILE_TYPE_PIPE != GetFileType(outputStream))
    {
      #if defined(_DEBUG)
      OSSpecific_writeDebugMessage(
        "Expected {Standard Output} type to be a pipe");
      #endif

      return FALSE;
    }

    return TRUE;
  }
  #elif defined(__linux__)
  {
    struct stat file_status;

    if (0 != fstat(inputStream, &(file_status)))
    {
      #if defined(_DEBUG)
      OSSpecific_writeDebugMessage(
        "{fstat} failed: 0x%08X",
        errno);
      #endif

      return FALSE;
    }

    if (!S_ISFIFO(file_status.st_mode))
    {
      #if defined(_DEBUG)
      OSSpecific_writeDebugMessage(
        "Expected {Standard Input} type to be a pipe");
      #endif

      return FALSE;
    }

    if (0 != fstat(outputStream, &(file_status)))
    {
      #if defined(_DEBUG)
      OSSpecific_writeDebugMessage(
        "{fstat} failed: 0x%08X",
        errno);
      #endif

      return FALSE;
    }

    if (!S_ISFIFO(file_status.st_mode))
    {
      #if defined(_DEBUG)
      OSSpecific_writeDebugMessage(
        "Expected {Standard Output} type to be a pipe");
      #endif

      return FALSE;
    }

    return TRUE;
  }
  #else
  {
    return FALSE;
  }
  #endif
}

/**************************************************************/

BOOL
OSSpecific_peekStream(
  _In_ const os_specific_stream_t stream,
  _Out_ size_t * streamSize)
{
  #if defined(_WIN32)
  {
    BOOL test_bool;
    DWORD pipe_length;

    test_bool = PeekNamedPipe(
      stream,
      NULL,
      0,
      NULL,
      &(pipe_length),
      NULL);

    /* If function fails, `GetLastError()` returns: */
    /* - `ERROR_INVALID_FUNCTION` when STDIN is a console or a file */
    /* - `ERROR_BROKEN_PIPE` when other end of the pipe was closed (e.g.: */
    /*  extension was disabled by a Web Browser; parent process was closed) */

    if (test_bool)
    {
      streamSize[0] = (size_t) pipe_length;
    }
    #if defined(_DEBUG)
    else
    {
      OSSpecific_writeDebugMessage(
        "{PeekNamedPipe} failed: 0x%08X",
        GetLastError());
    }
    #endif

    return test_bool;
  }
  #elif defined(__linux__)
  {
    int32_t result;

    struct pollfd fds =
    {
      .fd = stream,
      .events = (POLLIN | POLLRDHUP)
    };

    result = poll(&(fds), 1, 0);

    if ((-1) == result)
    {
      #if defined(_DEBUG)
      OSSpecific_writeDebugMessage(
        "{poll} failed: 0x%08X",
        errno);
      #endif

      return FALSE;
    }
    else if (0 == result)
    {
      /* Polling timeout - ignore */

      streamSize[0] = 0;
      return TRUE;
    }
    else if (POLLIN & fds.revents)
    {
      /* There is data to read - peek amount of available bytes */

      if ((-1) == ioctl(stream, FIONREAD, &(result)))
      {
        #if defined(_DEBUG)
        OSSpecific_writeDebugMessage(
          "{ioctl} failed: 0x%08X",
          errno);
        #endif

        return FALSE;
      }

      streamSize[0] = (size_t) result;

      return TRUE;
    }
    #if defined(_DEBUG)
    else if (POLLHUP & fds.revents)
    {
      /* Writing end of the pipe was closed */

      OSSpecific_writeDebugMessage(
        "{poll} failed: Hung up");
    }
    #endif

    return FALSE;
  }
  #else
  {
    return FALSE;
  }
  #endif
}

/**************************************************************/

BOOL
OSSpecific_readBytesFromStream(
  _In_ const os_specific_stream_t stream,
  _Out_ LPVOID output,
  _In_ const size_t size)
{
  #if defined(_WIN32)
  {
    BOOL test_bool;
    DWORD test_dword;

    test_bool = ReadFile(
      stream,
      output,
      size,
      &(test_dword),
      NULL);

    #if defined(_DEBUG)
    {
      if (test_bool && (size == test_dword)) { return TRUE; }

      OSSpecific_writeDebugMessage(
        "{ReadFile} failed: 0x%08X",
        GetLastError());

      return FALSE;
    }
    #else
    {
      return (test_bool && (size == test_dword));
    }
    #endif
  }
  #elif defined(__linux__)
  {
    #if defined(_DEBUG)
    {
      ssize_t result = read(stream, output, size);
      if (size == result) { return TRUE; }

      OSSpecific_writeDebugMessage(
        "{read} failed: 0x%08X",
        errno);

      return FALSE;
    }
    #else
    {
      return (size == read(stream, output, size));
    }
    #endif
  }
  #else
  {
    return FALSE;
  }
  #endif
}

/**************************************************************/

BOOL
OSSpecific_writeBytesToStream(
  _In_ const os_specific_stream_t stream,
  _In_ LPCVOID input,
  _In_ const size_t size)
{
  #if defined(_WIN32)
  {
    BOOL test_bool;
    DWORD test_dword;

    test_bool = WriteFile(
      stream,
      input,
      size,
      &(test_dword),
      NULL);

    if (test_bool && (size == test_dword))
    {
      #if defined(_DEBUG)
      {
        test_bool = FlushFileBuffers(stream);

        if (!test_bool)
        {
          test_dword = GetLastError();

          if (ERROR_INVALID_HANDLE == test_dword)
          {
            return TRUE;
          }

          OSSpecific_writeDebugMessage(
            "{FlushFileBuffers} failed: 0x%08X",
            GetLastError());
        }

        return test_bool;
      }
      #else
      {
        return FlushFileBuffers(stream);
      }
      #endif
    }

    #if defined(_DEBUG)
    {
      OSSpecific_writeDebugMessage(
        "{WriteFile} failed: 0x%08X",
        GetLastError());
    }
    #endif

    return FALSE;
  }
  #elif defined(__linux__)
  {
    #if defined(_DEBUG)
    {
      ssize_t result = write(stream, input, size);
      if (size == result) { return TRUE; }

      OSSpecific_writeDebugMessage(
        "{write} failed");

      return FALSE;
    }
    #else
    {
      return (size == write(stream, input, size));
    }
    #endif
  }
  #else
  {
    return FALSE;
  }
  #endif
}

/**************************************************************/

#if defined(_DEBUG)

  VOID
  OSSpecific_writeDebugMessage(
    _In_z_ LPCSTR message,
    ...)
  {
    CHAR buffer[512];
    va_list args;
    os_specific_stream_t stderr_stream;

    va_start(args, message);
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);

    #if defined(_WIN32)
    {
      stderr_stream = GetStdHandle(STD_ERROR_HANDLE);

      if (INVALID_HANDLE_VALUE == stderr_stream)
      {
        return;
      }
    }
    #elif defined(__linux__)
    {
      stderr_stream = STDERR_FILENO;
    }
    #endif

    OSSpecific_writeBytesToStream(
      stderr_stream,
      DEBUG_MESSAGE_START,
      sizeof(CHAR) * strlen(DEBUG_MESSAGE_START));

    OSSpecific_writeBytesToStream(
      stderr_stream,
      buffer,
      sizeof(CHAR) * strlen(buffer));

    OSSpecific_writeBytesToStream(
      stderr_stream,
      DEBUG_MESSAGE_END,
      sizeof(CHAR) * strlen(DEBUG_MESSAGE_END));

    #if defined(_WIN32)
    {
      OutputDebugStringA(buffer);
    }
    #endif
  }

#endif

/**************************************************************/