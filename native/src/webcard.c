/**
 * @file "native/src/webcard.c"
 * WebCard Native App :: Entry Point
 */

#include "smart_cards.h"

#if defined(_WIN32)
  #include <stdio.h>
  #include <io.h>
  #include <fcntl.h>
#endif

/**************************************************************/

BOOL
validateInputOutputPipes(void)
{
  os_specific_stream_t stdin_stream;
  os_specific_stream_t stdout_stream;

  #if defined(_WIN32)
  {
    _setmode(_fileno(stdin),  _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    stdin_stream = GetStdHandle(STD_INPUT_HANDLE);
    stdout_stream = GetStdHandle(STD_OUTPUT_HANDLE);
  }
  #elif defined(__linux__)
  {
    stdin_stream = STDIN_FILENO;
    stdout_stream = STDOUT_FILENO;
  }
  #endif

  return OSSpecific_validateTypesOfStreams(
    stdin_stream,
    stdout_stream);
}

/**************************************************************/

int
main(void)
{
  if (!validateInputOutputPipes())
  {
    return EXIT_FAILURE;
  }

  #if defined(_DEBUG)
  if (!OSSpecific_registerTerminationHandler())
  {
    OSSpecific_writeDebugMessage(
      "Failed to register a {termination handler}");

    return EXIT_FAILURE;
  }
  #endif

  #if defined(_DEBUG)
  OSSpecific_writeDebugMessage(
    "Starting {WebCard Native App}");
  #endif

  WebCard_run();

  #if defined(_DEBUG)
  OSSpecific_writeDebugMessage(
    "{ WebCard Native App } shutting down");
  #endif

  return EXIT_SUCCESS;
}

/**************************************************************/
