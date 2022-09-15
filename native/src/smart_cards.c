/**
 * @file "native/src/smart_cards.c"
 * Communication with Smart Card Readers (physical or virtual peripherals).
 */

#include "smart_cards.h"

/**************************************************************/

VOID
SCardConnection_init(
  _Out_ SCardConnection connection)
{
  connection->handle         = 0;
  connection->activeProtocol = 0;
  connection->ignoreCounter  = 0;
}

/**************************************************************/

BOOL
SCardConnection_open(
  _Inout_ SCardConnection connection,
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
  _Inout_ SCardConnection connection)
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
  _In_ ConstSCardConnection connection,
  _In_ LPCBYTE input,
  _In_ const DWORD inputLength,
  _Out_ LPBYTE output,
  _Inout_ LPDWORD outputLengthPointer)
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
    outputLengthPointer);

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
  _In_ ConstSCardConnection connection,
  _Inout_ UTF8String hexStringResult,
  _In_ LPCBYTE input,
  _In_ const DWORD inputLength,
  _Out_ LPBYTE output,
  _In_ const DWORD outputLength)
{
  BOOL test_bool;
  DWORD bytesReceived = outputLength;
  LPDWORD bytesReceivedPointer = &(bytesReceived);
  BYTE getResponseApdu[5] = {0x00, 0xC0, 0x00, 0x00};

  test_bool = SCardConnection_transceiveSingle(
    connection,
    input,
    inputLength,
    output,
    bytesReceivedPointer);

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
      bytesReceivedPointer);

    if (!test_bool) { return FALSE; }
  }

  /* The last or the only block */

  return UTF8String_pushBytesAsHex(
    hexStringResult,
    bytesReceived,
    output);
}

/**************************************************************/

VOID
SCardReaderDB_init(
  _Out_ SCardReaderDB database)
{
  database->count = 0;
  database->states = NULL;
  database->connections = NULL;
}

/**************************************************************/

VOID
SCardReaderDB_destroy(
  _Inout_ SCardReaderDB database)
{
  DWORD i;

  if (NULL != database->states)
  {
    for (i = 0; i < database->count; i++)
    {
      if (NULL != database->states[i].szReader)
      {
        free((LPVOID) database->states[i].szReader);
      }
    }

    free(database->states);
  }

  if (NULL != database->connections)
  {
    for (i = 0; i < database->count; i++)
    {
      SCardConnection_close(&(database->connections[i]));
    }

    free(database->connections);
  }
}

/**************************************************************/

BOOL
SCardReaderDB_load(
  _Out_ SCardReaderDB database,
  _In_ LPCTSTR readerNames)
{
  DWORD bytesize;
  DWORD name_length;

  LPCTSTR next_reader;
  LPSCARD_READERSTATE test_readerstate;
  SCardConnection test_connection;

  /* Initialize outgoing `SCardReaderDB` structure */

  SCardReaderDB_init(database);

  /* Iterate through every Smart Card Reader */

  next_reader = readerNames;

  while (next_reader[0])
  {
    /* Expand the "Smart Card Reader State" list */

    bytesize = sizeof(SCARD_READERSTATE) * (1 + database->count);
    test_readerstate = realloc(database->states, bytesize);
    if (NULL == test_readerstate) { return FALSE; }

    database->states = test_readerstate;
    test_readerstate = &(database->states[database->count]);

    /* Initialize "Smart Card Reader State" structure for current reader */

    test_readerstate->szReader = NULL;
    test_readerstate->dwCurrentState = SCARD_STATE_UNAWARE;
    test_readerstate->cbAtr = 0;

    /* Expand the "Smart Card Connection" list */

    bytesize = sizeof(struct scard_connection_t) * (1 + database->count);
    test_connection = realloc(database->connections, bytesize);
    if (NULL == test_connection) { return FALSE; }

    database->connections = test_connection;
    test_connection = &(database->connections[database->count]);

    /* Initialize "Smart Card Connection" structure for current reader */

    SCardConnection_init(test_connection);

    /* Both lists have "+1" valid (initialized) structure */

    database->count += 1;

    /* Get Smart Card Reader name's length */

    name_length = _tcslen(next_reader);

    /* Clone Smart Card Reader name */

    bytesize = sizeof(TCHAR) * (1 + name_length);
    test_readerstate->szReader = malloc(bytesize);
    if (NULL == test_readerstate->szReader) { return FALSE; }

    memcpy((LPVOID) test_readerstate->szReader, next_reader, bytesize);

    /* Move to the next entry in a multi-string list */

    next_reader = &(next_reader[1 + name_length]);
  }

  return TRUE;
}

/**************************************************************/

enum webcard_fetchreaders_enum
SCardReaderDB_fetch(
  _Inout_ SCardReaderDB database,
  _In_ const SCARDCONTEXT context,
  _In_ const BOOL firstFetch)
{
  enum webcard_fetchreaders_enum fetch_result;
  LONG result;
  DWORD bytesize;
  DWORD test_length;
  BOOL test_bool;

  LPTSTR reader_names;
  struct scard_reader_db_t test_database;

  /* Get total length of the multi-string list */

  result = SCardListReaders(
    context,
    NULL,
    NULL,
    &(test_length));

  if (SCARD_S_SUCCESS != result)
  {
    if (SCARD_E_NO_READERS_AVAILABLE == result)
    {
      /* Ignore this error, as readers can be plugged-in */
      /* later while the Native App is still running */
      test_length = 0;
    }
    else
    {
      #if defined(_DEBUG)
      {
        OSSpecific_writeDebugMessage(
          "{SCardListReaders} failed: 0x%08X (%s)",
          (uint32_t) result,
          WebCard_errorLookup(result));
      }
      #endif

      return WEBCARD_FETCHREADERS__FAIL;
    }
  }

  /* Are there any Smart Card Readers even available? */

  if (0 == test_length)
  {
    if (firstFetch)
    {
      /* Signal no errors on the first database look-up */
      return WEBCARD_FETCHREADERS__IGNORE;
    }

    if (0 != database->count)
    {
      /* Some readers were connected before */
      SCardReaderDB_destroy(database);
      SCardReaderDB_init(database);

      return WEBCARD_FETCHREADERS__LESS;
    }

    /* No readers connected before or after the fetching */
    return WEBCARD_FETCHREADERS__IGNORE;
  }

  /* Allocate memory for the multi-string list */

  bytesize = sizeof(TCHAR) * test_length;
  reader_names = malloc(bytesize);
  if (NULL == reader_names) { return WEBCARD_FETCHREADERS__FAIL; }

  /* Get Smart Card Reader names */

  result = SCardListReaders(
    context,
    NULL,
    reader_names,
    &(test_length));

  if (SCARD_S_SUCCESS != result)
  {
    free(reader_names);
    return WEBCARD_FETCHREADERS__FAIL;
  }

  /* Check if the array should be refreshed */

  if (firstFetch)
  {
    test_bool = TRUE;
    fetch_result = WEBCARD_FETCHREADERS__MORE;
  }
  else
  {
    test_length = Misc_multiStringList_elementCount(reader_names);

    if (test_length != database->count)
    {
      test_bool = TRUE;
      fetch_result = (test_length > database->count) ?
        WEBCARD_FETCHREADERS__MORE :
        WEBCARD_FETCHREADERS__LESS;
    }
    else
    {
      test_bool = FALSE;
    }
  }

  if (!test_bool)
  {
    free(reader_names);
    return WEBCARD_FETCHREADERS__IGNORE;
  }

  /* Prepare local Smart Card Readers array */

  test_bool = SCardReaderDB_load(&(test_database), reader_names);

  if (NULL != reader_names)
  {
    free(reader_names);
  }

  if (!test_bool)
  {
    SCardReaderDB_destroy(&(test_database));
    return WEBCARD_FETCHREADERS__FAIL;
  }

  /* Destroy previous Smart Card Readers array */

  SCardReaderDB_destroy(database);

  /* Replace outgoing array with the local array */
  /* (direct assignment: local destructor should not be called) */

  database[0] = test_database;

  return fetch_result;
}

/**************************************************************/

BOOL
WebCard_init(
  _Out_ SCardReaderDB resultDatabase,
  _Out_ LPSCARDCONTEXT resultContext)
{
  SCardReaderDB_init(resultDatabase);

  resultContext[0] = 0;

  LONG result = SCardEstablishContext(
    SCARD_SCOPE_USER,
    NULL,
    NULL,
    resultContext);

  if (SCARD_S_SUCCESS != result)
  {
    #if defined(_DEBUG)
    {
      OSSpecific_writeDebugMessage(
        "{SCardEstablishContext} failed: 0x%08X (%s)",
        (uint32_t) result,
        WebCard_errorLookup(result));
    }
    #endif

    return FALSE;
  }

  enum webcard_fetchreaders_enum fetch_result = SCardReaderDB_fetch(
      resultDatabase,
      resultContext[0],
      TRUE);

  if (WEBCARD_FETCHREADERS__FAIL == fetch_result)
  {
    #if defined(_DEBUG)
    {
      OSSpecific_writeDebugMessage(
        "{SCardReaderDB::fetch} failed");
    }
    #endif

    return FALSE;
  }

  return TRUE;
}

/**************************************************************/

#if defined(_DEBUG)

  LPCSTR
  WebCard_errorLookup(
    _In_ const LONG errorCode)
  {
    switch (errorCode)
    {
      #if defined(_WIN32)
        case ERROR_BROKEN_PIPE:
          return "ERROR_BROKEN_PIPE";
        case SCARD_E_NO_PIN_CACHE:
          return "SCARD_E_NO_PIN_CACHE";
        case SCARD_E_PIN_CACHE_EXPIRED:
          return "SCARD_E_PIN_CACHE_EXPIRED";
        case SCARD_E_READ_ONLY_CARD:
          return "SCARD_E_READ_ONLY_CARD";
        case SCARD_E_UNEXPECTED:
          return "SCARD_E_UNEXPECTED";
        case SCARD_W_CACHE_ITEM_NOT_FOUND:
          return "SCARD_W_CACHE_ITEM_NOT_FOUND";
        case SCARD_W_CACHE_ITEM_STALE:
          return "SCARD_W_CACHE_ITEM_STALE";
        case SCARD_W_CACHE_ITEM_TOO_BIG:
          return "SCARD_W_CACHE_ITEM_TOO_BIG";
      #endif
      case SCARD_E_BAD_SEEK:
        return "SCARD_E_BAD_SEEK";
      case SCARD_E_CANCELLED:
        return "SCARD_E_CANCELLED";
      case SCARD_E_CANT_DISPOSE:
        return "SCARD_E_CANT_DISPOSE";
      case SCARD_E_CARD_UNSUPPORTED:
        return "SCARD_E_CARD_UNSUPPORTED";
      case SCARD_E_CERTIFICATE_UNAVAILABLE:
        return "SCARD_E_CERTIFICATE_UNAVAILABLE";
      case SCARD_E_COMM_DATA_LOST:
        return "SCARD_E_COMM_DATA_LOST";
      case SCARD_E_DIR_NOT_FOUND:
        return "SCARD_E_DIR_NOT_FOUND";
      case SCARD_E_DUPLICATE_READER:
        return "SCARD_E_DUPLICATE_READER";
      case SCARD_E_FILE_NOT_FOUND:
        return "SCARD_E_FILE_NOT_FOUND";
      case SCARD_E_ICC_CREATEORDER:
        return "SCARD_E_ICC_CREATEORDER";
      case SCARD_E_ICC_INSTALLATION:
        return "SCARD_E_ICC_INSTALLATION";
      case SCARD_E_INSUFFICIENT_BUFFER:
        return "SCARD_E_INSUFFICIENT_BUFFER";
      case SCARD_E_INVALID_ATR:
        return "SCARD_E_INVALID_ATR";
      case SCARD_E_INVALID_CHV:
        return "SCARD_E_INVALID_CHV";
      case SCARD_E_INVALID_HANDLE:
        return "SCARD_E_INVALID_HANDLE";
      case SCARD_E_INVALID_PARAMETER:
        return "SCARD_E_INVALID_PARAMETER";
      case SCARD_E_INVALID_TARGET:
        return "SCARD_E_INVALID_TARGET";
      case SCARD_E_INVALID_VALUE:
        return "SCARD_E_INVALID_VALUE";
      case SCARD_E_NO_ACCESS:
        return "SCARD_E_NO_ACCESS";
      case SCARD_E_NO_DIR:
        return "SCARD_E_NO_DIR";
      case SCARD_E_NO_FILE:
        return "SCARD_E_NO_FILE";
      case SCARD_E_NO_KEY_CONTAINER:
        return "SCARD_E_NO_KEY_CONTAINER";
      case SCARD_E_NO_MEMORY:
        return "SCARD_E_NO_MEMORY";
      case SCARD_E_NO_READERS_AVAILABLE:
        return "SCARD_E_NO_READERS_AVAILABLE";
      case SCARD_E_NO_SERVICE:
        return "SCARD_E_NO_SERVICE";
      case SCARD_E_NO_SMARTCARD:
        return "SCARD_E_NO_SMARTCARD";
      case SCARD_E_NO_SUCH_CERTIFICATE:
        return "SCARD_E_NO_SUCH_CERTIFICATE";
      case SCARD_E_NOT_READY:
        return "SCARD_E_NOT_READY";
      case SCARD_E_NOT_TRANSACTED:
        return "SCARD_E_NOT_TRANSACTED";
      case SCARD_E_PCI_TOO_SMALL:
        return "SCARD_E_PCI_TOO_SMALL";
      case SCARD_E_PROTO_MISMATCH:
        return "SCARD_E_PROTO_MISMATCH";
      case SCARD_E_READER_UNAVAILABLE:
        return "SCARD_E_READER_UNAVAILABLE";
      case SCARD_E_READER_UNSUPPORTED:
        return "SCARD_E_READER_UNSUPPORTED";
      case SCARD_E_SERVER_TOO_BUSY:
        return "SCARD_E_SERVER_TOO_BUSY";
      case SCARD_E_SERVICE_STOPPED:
        return "SCARD_E_SERVICE_STOPPED";
      case SCARD_E_SHARING_VIOLATION:
        return "SCARD_E_SHARING_VIOLATION";
      case SCARD_E_SYSTEM_CANCELLED:
        return "SCARD_E_SYSTEM_CANCELLED";
      case SCARD_E_TIMEOUT:
        return "SCARD_E_TIMEOUT";
      case SCARD_E_UNKNOWN_CARD:
        return "SCARD_E_UNKNOWN_CARD";
      case SCARD_E_UNKNOWN_READER:
        return "SCARD_E_UNKNOWN_READER";
      case SCARD_E_UNKNOWN_RES_MNG:
        return "SCARD_E_UNKNOWN_RES_MNG";
      case SCARD_E_UNSUPPORTED_FEATURE:
        return "SCARD_E_UNSUPPORTED_FEATURE";
      case SCARD_E_WRITE_TOO_MANY:
        return "SCARD_E_WRITE_TOO_MANY";
      case SCARD_F_COMM_ERROR:
        return "SCARD_F_COMM_ERROR";
      case SCARD_F_INTERNAL_ERROR:
        return "SCARD_F_INTERNAL_ERROR";
      case SCARD_F_UNKNOWN_ERROR:
        return "SCARD_F_UNKNOWN_ERROR";
      case SCARD_F_WAITED_TOO_LONG:
        return "SCARD_F_WAITED_TOO_LONG";
      case SCARD_P_SHUTDOWN:
        return "SCARD_P_SHUTDOWN";
      case SCARD_S_SUCCESS:
        return "SCARD_S_SUCCESS";
      case SCARD_W_CANCELLED_BY_USER:
        return "SCARD_W_CANCELLED_BY_USER";
      case SCARD_W_CARD_NOT_AUTHENTICATED:
        return "SCARD_W_CARD_NOT_AUTHENTICATED";
      case SCARD_W_CHV_BLOCKED:
        return "SCARD_W_CHV_BLOCKED";
      case SCARD_W_EOF:
        return "SCARD_W_EOF";
      case SCARD_W_REMOVED_CARD:
        return "SCARD_W_REMOVED_CARD";
      case SCARD_W_RESET_CARD:
        return "SCARD_W_RESET_CARD";
      case SCARD_W_SECURITY_VIOLATION:
        return "SCARD_W_SECURITY_VIOLATION";
      case SCARD_W_UNPOWERED_CARD:
        return "SCARD_W_UNPOWERED_CARD";
      case SCARD_W_UNRESPONSIVE_CARD:
        return "SCARD_W_UNRESPONSIVE_CARD";
      case SCARD_W_UNSUPPORTED_CARD:
        return "SCARD_W_UNSUPPORTED_CARD";
      case SCARD_W_WRONG_CHV:
        return "SCARD_W_WRONG_CHV";
      default:
        return "";
    }
  }

#endif

/**************************************************************/

VOID
WebCard_close(
  _Inout_ SCardReaderDB database,
  _In_ const SCARDCONTEXT context)
{
  SCardReaderDB_destroy(database);

  if (0 != context)
  {
    SCardReleaseContext(context);
  }
}

/**************************************************************/

VOID
WebCard_run(void)
{
  SCARDCONTEXT context;
  struct scard_reader_db_t database;
  enum json_byte_stream_enum byte_stream_status;
  enum webcard_fetchreaders_enum fetch_result;

  struct json_byte_stream_t json_stream;
  struct json_object_t json_request;
  struct json_object_t json_response;

  clock_t cpu_time_start = clock();
  clock_t cpu_time_end;
  double cpu_time_elapsed;

  BOOL active = WebCard_init(&(database), &(context));

  while (active)
  {
    cpu_time_end = clock();
    cpu_time_elapsed =
      ((double)(cpu_time_end - cpu_time_start)) / CLOCKS_PER_SEC;

    /* Do the fetching every 1.0 second */

    if (cpu_time_elapsed >= 1.0)
    {
      cpu_time_start = cpu_time_end;

      /* 1) Fetch list of Smart Card Readers */
      /* (detecting plugging and unplugging) */

      fetch_result = SCardReaderDB_fetch(&(database), context, FALSE);

      if ((WEBCARD_FETCHREADERS__FAIL != fetch_result) &&
        (WEBCARD_FETCHREADERS__IGNORE != fetch_result))
      {
        WebCard_sendReaderEvent(
          NULL,
          0,
          (WEBCARD_FETCHREADERS__MORE == fetch_result) ?
            WEBCARD_READEREVENT__READERS_MORE :
            WEBCARD_READEREVENT__READERS_LESS,
          &(json_response));

        JsonObject_destroy(&(json_response));
      }
    }

    /* 2) Update Smart Card Reader Status list */
    /* (detecting existence of smart cards) */

    WebCard_handleStatusChange(&(database), context);

    /* 3) Parse commands from Standard Input */

    byte_stream_status = JsonByteStream_loadFromStandardInput(&(json_stream));

    if (JSON_BYTESTREAM_VALID == byte_stream_status)
    {
      WebCard_handleRequest(
        &(json_stream),
        &(json_request),
        &(json_response),
        &(database),
        context);

      JsonObject_destroy(&(json_request));
      JsonObject_destroy(&(json_response));
    }
    else if (JSON_BYTESTREAM_NOMORE == byte_stream_status)
    {
      active = FALSE;
    }
  }

  WebCard_close(&(database), context);
}

/**************************************************************/

VOID
WebCard_handleRequest(
  _Inout_ JsonByteStream jsonStream,
  _Out_ JsonObject jsonRequest,
  _Out_ JsonObject jsonResponse,
  _In_ ConstSCardReaderDB database,
  _In_ const SCARDCONTEXT context)
{
  BOOL test_bool;
  struct json_value_t json_value;
  struct utf8_string_t utf8_string;
  size_t command;

  /* Initialize JSON response object */
  /* (it will be destroyed by caller) */

  JsonObject_init(jsonResponse);

  /* Initialize and load JSON request object */
  /* Destroy `jsonStream` after parsing the JSON object */

  test_bool = JsonObject_parse(
    &(jsonRequest),
    FALSE,
    jsonStream);

  JsonByteStream_destroy(jsonStream);

  if (!test_bool)
  {
    #if defined(_DEBUG)
      OSSpecific_writeDebugMessage(
        "{JSON Request} parsing error!");
    #endif

    return;
  }

  /* Try to find the "i" key (unique message identifier) */

  test_bool = JsonObject_getValue(
    jsonRequest,
    &(json_value),
    "i");

  if (!test_bool || (JSON_VALUETYPE_STRING != json_value.type))
  {
    return;
  }

  /* Try to append the "i" key to JSON response */

  test_bool = JsonObject_appendKeyValue(
    jsonResponse,
    "i",
    &(json_value));

  if (!test_bool) { return; }

  /* Try to find the "c" key (request command) */

  test_bool = JsonObject_getValue(
    jsonRequest,
    &(json_value),
    "c");

  if (!test_bool || (JSON_VALUETYPE_NUMBER != json_value.type))
  {
    return;
  }

  /* Handle requested command */

  command = (size_t) (((FLOAT *) json_value.value)[0]);

  switch (command)
  {
    case WEBCARD_COMMAND__LIST_READERS:
    {
      test_bool = WebCard_pushReadersListToJsonResponse(
        jsonResponse,
        database);

      break;
    }

    case WEBCARD_COMMAND__CONNECT:
    {
      test_bool = WebCard_tryConnectingToReader(
        jsonRequest,
        jsonResponse,
        database,
        context);

      break;
    }

    case WEBCARD_COMMAND__DISCONNECT:
    {
      test_bool = WebCard_tryDisconnectingFromReader(
        jsonRequest,
        database);

      /* "Empty" response (JSON object containing the "i" key only) */
      /* will be required to resolve a "JavaScript Promise" */
      break;
    }

    case WEBCARD_COMMAND__TRANSCEIVE:
    {
      test_bool = WebCard_transmitAndReceive(
        jsonRequest,
        jsonResponse,
        database);

      break;
    }

    case WEBCARD_COMMAND__GET_VERSION:
    {
      UTF8String_makeTemporary(&(utf8_string), WEBCARD_VERSION);

      json_value.type = JSON_VALUETYPE_STRING;
      json_value.value = &(utf8_string);

      test_bool = JsonObject_appendKeyValue(
        jsonResponse,
        "verNat",
        &(json_value));

      break;
    }

    default:
    {
      test_bool = TRUE;
    }
  }

  /* Try to always send a JSON Response (so that a JavaScript Promise */
  /* won't hang), even if a WebCard's command-handling function has failed */

  if (!test_bool)
  {
    /* Append an optional key-value "incomplete=true" */

    json_value.type = JSON_VALUETYPE_TRUE;
    json_value.value = NULL;

    JsonObject_appendKeyValue(jsonResponse, "incomplete", &(json_value));
  }

  /* Stringify JSON response and send it through the STDOUT stream */

  UTF8String_init(&(utf8_string));

  test_bool = JsonObject_toString(jsonResponse, &(utf8_string));

  if (test_bool)
  {
    UTF8String_writeToStandardOutput(&(utf8_string));
  }

  UTF8String_destroy(&(utf8_string));
}

/**************************************************************/

BOOL
WebCard_pushReaderNameToJsonObject(
  _In_ SCARD_READERSTATE const * reader,
  _Inout_ JsonObject jsonObject,
  _In_ LPCSTR key)
{
  BOOL test_bool;
  struct json_value_t json_value;
  struct utf8_string_t utf8_reader_name;

  #ifdef _UNICODE
    struct utf16_string_t utf16_reader_name;
  #endif

  /* Copy Smart Card Reader's name into a JSON string */

  UTF8String_init(&(utf8_reader_name));

  #ifdef _UNICODE
  {
    UTF16String_init(&(utf16_reader_name));

    test_bool = UTF16String_pushText(
      &(utf16_reader_name),
      reader->szReader,
      0);

    if (test_bool)
    {
      test_bool = UTF16String_toUTF8(
        &(utf16_reader_name),
        &(utf8_reader_name));
    }

    UTF16String_destroy(&(utf16_reader_name));
  }
  #else
  {
    test_bool = UTF8String_pushText(
      &(utf8_reader_name),
      reader->szReader,
      0);
  }
  #endif

  if (!test_bool)
  {
    UTF8String_destroy(&(utf8_reader_name));
    return FALSE;
  }

  /* Add "Reader Name" under a specified key */

  json_value.type = JSON_VALUETYPE_STRING;
  json_value.value = &(utf8_reader_name);

  test_bool = JsonObject_appendKeyValue(
    jsonObject,
    key,
    &(json_value));

  UTF8String_destroy(&(utf8_reader_name));

  return test_bool;
}

/**************************************************************/

BOOL
WebCard_convertReaderStateToJsonObject(
  _In_ SCARD_READERSTATE const * reader,
  _Out_ JsonObject jsonReaderObject)
{
  BOOL test_bool;

  /* Initialize JSON reader object */
  /* (it will be destroyed by caller) */

  JsonObject_init(jsonReaderObject);

  /* Add key "n" (Reader Name) */

  test_bool = WebCard_pushReaderNameToJsonObject(
    reader,
    jsonReaderObject,
    "n");

  if (!test_bool) { return FALSE; }

  /* Add key "a" (card Answer To Reset) */

  return WebCard_pushReaderAtrToJsonObject(
    reader,
    jsonReaderObject,
    "a");
}

/**************************************************************/

BOOL
WebCard_pushReaderAtrToJsonObject(
  _In_ SCARD_READERSTATE const * reader,
  _Inout_ JsonObject jsonObject,
  _In_ LPCSTR key)
{
  BOOL test_bool;
  struct json_value_t json_value;
  struct utf8_string_t utf8_string;

  /* Copy Smart Card "ATR" identifier (bytearray to text) */

  UTF8String_init(&(utf8_string));

  test_bool = UTF8String_pushBytesAsHex(
    &(utf8_string),
    reader->cbAtr,
    reader->rgbAtr);

  if (!test_bool)
  {
    UTF8String_destroy(&(utf8_string));
    return FALSE;
  }

  /* Add "card Answer To Reset" under a specified key */

  json_value.type = JSON_VALUETYPE_STRING;
  json_value.value = &(utf8_string);

  test_bool = JsonObject_appendKeyValue(
    jsonObject,
    key,
    &(json_value));

  UTF8String_destroy(&(utf8_string));

  return test_bool;
}

/**************************************************************/

BOOL
WebCard_convertReaderStatesToJsonArray(
  _In_ ConstSCardReaderDB database,
  _Out_ JsonArray jsonReadersArray)
{
  BOOL test_bool;
  struct json_value_t json_value;
  struct json_object_t json_reader_object;

  json_value.type = JSON_VALUETYPE_OBJECT;
  json_value.value = &(json_reader_object);

  /* Initialize JSON reader object */
  /* (it will be destroyed by caller) */

  JsonArray_init(jsonReadersArray);

  /* Enumerate Smart Card Readers */

  for (size_t i = 0; i < database->count; i++)
  {
    test_bool = WebCard_convertReaderStateToJsonObject(
      &(database->states[i]),
      &(json_reader_object));

    if (test_bool)
    {
      test_bool = JsonArray_append(
        jsonReadersArray,
        &(json_value));
    }

    JsonObject_destroy(&(json_reader_object));

    if (!test_bool)
    {
      return FALSE;
    }
  }

  return TRUE;
}

/**************************************************************/

BOOL
WebCard_pushReadersListToJsonResponse(
  _Inout_ JsonObject jsonResponse,
  _In_ ConstSCardReaderDB database)
{
  BOOL test_bool;
  struct json_array_t json_readers_array;
  struct json_value_t json_value;

  test_bool = WebCard_convertReaderStatesToJsonArray(
    database,
    &(json_readers_array));

  if (!test_bool)
  {
    JsonArray_destroy(&(json_readers_array));
    return FALSE;
  }

  json_value.type = JSON_VALUETYPE_ARRAY;
  json_value.value = &(json_readers_array);

  test_bool = JsonObject_appendKeyValue(
    jsonResponse,
    "d",
    &(json_value));

  JsonArray_destroy(&(json_readers_array));
  return test_bool;
}

/**************************************************************/

BOOL
WebCard_tryConnectingToReader(
  _In_ ConstJsonObject jsonRequest,
  _Inout_ JsonObject jsonResponse,
  _In_ ConstSCardReaderDB database,
  _In_ const SCARDCONTEXT context)
{
  BOOL test_bool;
  size_t reader_index;
  SCARD_READERSTATE const * reader;
  DWORD share_mode = SCARD_SHARE_SHARED;
  struct json_value_t json_value;

  /* Try to find the "r" key (reader index) */

  test_bool = JsonObject_getValue(
    jsonRequest,
    &(json_value),
    "r");

  if (!test_bool || (JSON_VALUETYPE_NUMBER != json_value.type))
  {
    #if defined(_DEBUG)
    {
      OSSpecific_writeDebugMessage(
        "{WebCard::tryConnectingToReader} failed: " \
        "missing \"r\" key!"
      );
    }
    #endif

    return FALSE;
  }

  reader_index = (size_t) (((FLOAT *) json_value.value)[0]);

  if (reader_index >= database->count)
  {
    #if defined(_DEBUG)
    {
      OSSpecific_writeDebugMessage(
        "{WebCard::tryConnectingToReader} failed: " \
        "invalid reader index!"
      );
    }
    #endif

    return FALSE;
  }

  /* Try to find the "p" key (optional share mode param) */

  test_bool = JsonObject_getValue(
    jsonRequest,
    &(json_value),
    "p");

  if (test_bool && (JSON_VALUETYPE_NUMBER == json_value.type))
  {
    share_mode = (DWORD) (((FLOAT *) json_value.value)[0]);
  }

  /* Try to open a connection to active Smart Card */

  reader = &(database->states[reader_index]);

  test_bool = SCardConnection_open(
    &(database->connections[reader_index]),
    context,
    reader->szReader,
    share_mode);

  if (!test_bool) { return FALSE; }

  /* Add key "d" (card Answer To Reset) */

  return WebCard_pushReaderAtrToJsonObject(
    reader,
    jsonResponse,
    "d");
}

/**************************************************************/

BOOL
WebCard_tryDisconnectingFromReader(
  _In_ ConstJsonObject jsonRequest,
  _In_ ConstSCardReaderDB database)
{
  BOOL test_bool;
  size_t reader_index;
  struct json_value_t json_value;

  /* Try to find the "r" key (reader index) */

  test_bool = JsonObject_getValue(
    jsonRequest,
    &(json_value),
    "r");

  if (!test_bool || (JSON_VALUETYPE_NUMBER != json_value.type))
  {
    #if defined(_DEBUG)
    {
      OSSpecific_writeDebugMessage(
        "{WebCard::tryDisconnectingFromReader} failed: " \
        "missing \"r\" key!"
      );
    }
    #endif

    return FALSE;
  }

  reader_index = (size_t) (((FLOAT *) json_value.value)[0]);

  if (reader_index >= database->count)
  {
    #if defined(_DEBUG)
    {
      OSSpecific_writeDebugMessage(
        "{WebCard::tryDisconnectingFromReader} failed: " \
        "invalid reader index!"
      );
    }
    #endif

    return FALSE;
  }

  /* Try to close a connection to active Smart Card */

  return SCardConnection_close(
    &(database->connections[reader_index]));
}

/**************************************************************/

BOOL
WebCard_transmitAndReceive(
  _In_ ConstJsonObject jsonRequest,
  _Inout_ JsonObject jsonResponse,
  _In_ ConstSCardReaderDB database)
{
  BOOL test_bool;
  size_t reader_index;
  LPBYTE input_bytes;
  size_t input_bytes_length;
  LPBYTE output_bytes;
  struct json_value_t json_value;
  struct utf8_string_t utf8_hex_apdu_response;
  SCardConnection connection;

  /* Try to find the "r" key (reader index) */

  test_bool = JsonObject_getValue(
    jsonRequest,
    &(json_value),
    "r");

  if (!test_bool || (JSON_VALUETYPE_NUMBER != json_value.type))
  {
    #if defined(_DEBUG)
    {
      OSSpecific_writeDebugMessage(
        "{WebCard::transmitAndReceive} failed: " \
        "missing \"r\" key!"
      );
    }
    #endif

    return FALSE;
  }

  reader_index = (size_t) (((FLOAT *) json_value.value)[0]);

  if (reader_index >= database->count)
  {
    #if defined(_DEBUG)
    {
      OSSpecific_writeDebugMessage(
        "{WebCard::transmitAndReceive} failed: " \
        "invalid reader index!"
      );
    }
    #endif

    return FALSE;
  }

  /* Make sure that a connection to the Smart Card is still active */

  connection = &(database->connections[reader_index]);

  if (0 == connection->handle)
  {
    #if defined(_DEBUG)
    {
      OSSpecific_writeDebugMessage(
        "{WebCard::transmitAndReceive} failed: " \
        "no connection!"
      );
    }
    #endif

    return FALSE;
  }

  /* Try to find the "a" key (Application Protocol Data Unit) */

  test_bool = JsonObject_getValue(
    jsonRequest,
    &(json_value),
    "a");

  if (!test_bool || (JSON_VALUETYPE_STRING != json_value.type))
  {
    return FALSE;
  }

  /* Prepare input and output byte buffers */

  test_bool = UTF8String_hexToByteArray(
    json_value.value,
    &(input_bytes_length),
    &(input_bytes));

  if (!test_bool)
  {
    if (NULL != input_bytes)
    {
      free(input_bytes);
    }
    return FALSE;
  }

  output_bytes = malloc(sizeof(BYTE) * MAX_APDU_SIZE);
  if (NULL == output_bytes)
  {
    free(input_bytes);
    return FALSE;
  }

  /* Transmit and receive */

  UTF8String_init(&(utf8_hex_apdu_response));

  test_bool = SCardConnection_transceiveMultiple(
    connection,
    &(utf8_hex_apdu_response),
    input_bytes,
    input_bytes_length,
    output_bytes,
    MAX_APDU_SIZE);

  if (test_bool)
  {
    /* Add key "d" (Smart Card APDU response) */

    json_value.type = JSON_VALUETYPE_STRING;
    json_value.value = &(utf8_hex_apdu_response);

    test_bool = JsonObject_appendKeyValue(
      jsonResponse,
      "d",
      &(json_value));
  }

  UTF8String_destroy(&(utf8_hex_apdu_response));

  return test_bool;
}

/**************************************************************/

VOID
WebCard_sendReaderEvent(
  _In_ SCARD_READERSTATE const * reader,
  _In_ const size_t readerIndex,
  _In_ const enum webcard_readerevent_enum readerEvent,
  _Out_ JsonObject jsonResponse)
{
  BOOL test_bool;
  FLOAT test_float;
  struct json_value_t json_value;
  struct utf8_string_t utf8_string;

  #if defined(_DEBUG)
  {
    OSSpecific_writeDebugMessage(
      "Sending a ReaderEvent '%d' (ReaderIndex '%d')",
      readerEvent,
      readerIndex);
  }
  #endif

  /* Initialize JSON response object */
  /* (it will be destroyed by caller) */

  JsonObject_init(jsonResponse);

  json_value.type = JSON_VALUETYPE_NUMBER;
  json_value.value = &(test_float);

  /* Add key "e" (reader event) */

  test_float = (FLOAT) readerEvent;

  test_bool = JsonObject_appendKeyValue(
    jsonResponse,
    "e",
    &(json_value));

  if (!test_bool) { return; }

  if (NULL != reader)
  {
    /* Add key "r" (reader index for reader events) */

    test_float = (FLOAT) readerIndex;

    test_bool = JsonObject_appendKeyValue(
      jsonResponse,
      "r",
      &(json_value));

    if (!test_bool) { return; }

    /* Add key "d" (card Answer To Reset) on CARD INSERT event */

    if (WEBCARD_READEREVENT__CARD_INSERTION == readerEvent)
    {
      test_bool = WebCard_pushReaderAtrToJsonObject(
        reader,
        jsonResponse,
        "d");

      if (!test_bool) { return; }
    }
  }

  /* Stringify JSON response and send it through the STDOUT stream */

  UTF8String_init(&(utf8_string));

  test_bool = JsonObject_toString(jsonResponse, &(utf8_string));

  if (test_bool)
  {
    UTF8String_writeToStandardOutput(&(utf8_string));
  }

  UTF8String_destroy(&(utf8_string));
}

/**************************************************************/

VOID
WebCard_handleStatusChange(
  _Inout_ SCardReaderDB database,
  _In_ const SCARDCONTEXT context)
{
  struct json_object_t json_response;

  LONG result = SCardGetStatusChange(
    context,
    0,
    database->states,
    database->count);

  if (SCARD_S_SUCCESS != result) { return; }

  /* Enumerate Smart Card Readers */

  for (size_t i = 0; i < database->count; i++)
  {
    LPSCARD_READERSTATE reader = &(database->states[i]);
    SCardConnection connection = &(database->connections[i]);

    if (reader->dwEventState & SCARD_STATE_CHANGED)
    {
      if (connection->ignoreCounter > 0)
      {
        connection->ignoreCounter -= 1;
      }
      else
      {
        enum webcard_readerevent_enum reader_event = WEBCARD_READEREVENT__NONE;

        if ((reader->dwCurrentState & SCARD_STATE_EMPTY) &&
          (reader->dwEventState & SCARD_STATE_PRESENT))
        {
          reader_event = WEBCARD_READEREVENT__CARD_INSERTION;
        }
        else if ((reader->dwCurrentState & SCARD_STATE_PRESENT) &&
        (reader->dwEventState & SCARD_STATE_EMPTY))
        {
          reader_event = WEBCARD_READEREVENT__CARD_REMOVAL;

          /* Invalidate connection */
          connection->handle = 0;
        }

        if (WEBCARD_READEREVENT__NONE != reader_event)
        {
          WebCard_sendReaderEvent(
            reader,
            i,
            reader_event,
            &(json_response));

          JsonObject_destroy(&(json_response));
        }
      }

      reader->dwCurrentState = (reader->dwEventState & (~SCARD_STATE_CHANGED));
    }
  }
}

/**************************************************************/
