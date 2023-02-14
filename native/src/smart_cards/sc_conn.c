/**
 * @file "native/src/smart_cards/sc_conn.c"
 * Communication with Smart Card Readers (physical or virtual peripherals).
 */

#include "smart_cards/smart_cards.h"

/**************************************************************/

VOID
SCardConnection_init(
  _Out_ SCardConnection *connection)
{
  connection->handle         = 0;
  connection->activeProtocol = 0;
  connection->ignoreCounter  = 0;
}

/**************************************************************/

BOOL
SCardConnection_open(
  _Inout_ SCardConnection *connection,
  _In_ const SCARDCONTEXT context,
  _In_ LPCTSTR readerName,
  _In_ const DWORD shareMode)
{
  if (0 != connection->handle)
  {
    return TRUE;
  }

  LONG result = SCardConnect(
    context,
    readerName,
    shareMode,
    (SCARD_SHARE_DIRECT == shareMode) ?
      0 :
      (SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1),
    &(connection->handle),
    &(connection->activeProtocol));

  if (SCARD_S_SUCCESS != result)
  {
    #if defined(_DEBUG)
    {
      OSSpecific_writeDebugMessage(
        "{SCardConnect} failed: 0x%08X (%s)",
        (uint32_t) result,
        WebCard_errorLookup(result));
    }
    #endif

    return FALSE;
  }

  return TRUE;
}

/**************************************************************/

BOOL
SCardConnection_close(
  _Inout_ SCardConnection *connection)
{
  if (0 == connection->handle)
  {
    return TRUE;
  }

  LONG result = SCardDisconnect(
    connection->handle,
    SCARD_LEAVE_CARD);

  connection->handle = 0;

  return (SCARD_S_SUCCESS == result);
}

/**************************************************************/

BOOL
SCardConnection_transceiveSingle(
  _In_ const SCardConnection *connection,
  _In_ const BYTE *input,
  _In_ const DWORD inputLength,
  _Out_ BYTE *output,
  _Inout_ DWORD *outputLengthRef)
{
  LONG result = SCardTransmit(
    connection->handle,
    (SCARD_PROTOCOL_T0 == connection->activeProtocol) ?
      SCARD_PCI_T0 :
      SCARD_PCI_T1,
    input,
    inputLength,
    NULL,
    output,
    outputLengthRef);

  if (SCARD_S_SUCCESS != result)
  {
    #if defined(_DEBUG)
    {
      OSSpecific_writeDebugMessage(
        "{SCardTransmit} failed: 0x%08X (%s)",
        (uint32_t) result,
        WebCard_errorLookup(result));
    }
    #endif

    return FALSE;
  }

  return TRUE;
}

/**************************************************************/

BOOL
SCardConnection_transceiveMultiple(
  _In_ const SCardConnection *connection,
  _Inout_ UTF8String *hexStringResult,
  _In_ const BYTE *input,
  _In_ const DWORD inputLength,
  _Out_ LPBYTE output,
  _In_ const DWORD outputLength)
{
  BOOL test_bool;
  DWORD bytesReceived = outputLength;
  DWORD *bytesReceivedRef = &(bytesReceived);
  BYTE getResponseApdu[5] = {0x00, 0xC0, 0x00, 0x00};

  test_bool = SCardConnection_transceiveSingle(
    connection,
    input,
    inputLength,
    output,
    bytesReceivedRef);

  if (!test_bool) { return FALSE; }

  /* Check if "Status Word 1" is "Response bytes still available" */

  while ((outputLength >= 2) && (0x61 == output[outputLength - 2]))
  {
    test_bool = UTF8String_pushBytesAsHex(
      hexStringResult,
      bytesReceived - 2,
      output);

    if (!test_bool) { return FALSE; }

    /* Move "Status Word 2" into "GET RESPONSE" APDU */

    getResponseApdu[4] = output[bytesReceived - 1];

    /* Continue transmission */

    bytesReceived = outputLength;

    test_bool = SCardConnection_transceiveSingle(
      connection,
      getResponseApdu,
      5,
      output,
      bytesReceivedRef);

    if (!test_bool) { return FALSE; }
  }

  /* The last or the only block */

  return UTF8String_pushBytesAsHex(
    hexStringResult,
    bytesReceived,
    output);
}

/**************************************************************/
