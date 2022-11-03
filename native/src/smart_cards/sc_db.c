/**
 * @file "native/src/smart_cards/sc_db.c"
 * Communication with Smart Card Readers (physical or virtual peripherals).
 */

#include "smart_cards/smart_cards.h"

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

BOOL
SCardReaderDB_hasReaderNamed(
  _In_ ConstSCardReaderDB database,
  _In_ LPCTSTR readerName)
{
  DWORD i;

  for (i = 0; i < database->count; i++)
  {
    if (0 == _tcscmp(database->states[i].szReader, readerName))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**************************************************************/

enum webcard_fetchreaders_enum
SCardReaderDB_fetch(
  _Inout_ SCardReaderDB database,
  _Out_ JsonArray jsonReaderNames,
  _In_ const SCARDCONTEXT context,
  _In_ const BOOL firstFetch)
{
  enum webcard_fetchreaders_enum fetch_result;
  LONG result;
  DWORD bytesize;
  DWORD test_length;
  BOOL test_bool;

  DWORD i;
  LPTSTR reader_names;
  struct scard_reader_db_t test_database;

  if (NULL != jsonReaderNames)
  {
    JsonArray_init(jsonReaderNames);
  }

  /* Get total length of the multi-string list */

  result = SCardListReaders(
    context,
    NULL,
    NULL,
    &(test_length));

  if (SCARD_S_SUCCESS != result)
  {
    if (SCARD_E_SERVICE_STOPPED == result)
    {
      /* Last reader unplugged */
      return WEBCARD_FETCHREADERS__SERVICE_STOPPED;
    }
    else if (SCARD_E_NO_READERS_AVAILABLE == result)
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
      if (NULL != jsonReaderNames)
      {
        for (i = 0; i < database->count; i++)
        {
          WebCard_pushReaderNameToJsonArray(
            &(database->states[i]),
            jsonReaderNames);
        }
      }
      SCardReaderDB_destroy(database);
      SCardReaderDB_init(database);

      return WEBCARD_FETCHREADERS__LESS_READERS;
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
    fetch_result = WEBCARD_FETCHREADERS__MORE_READERS;
  }
  else
  {
    test_length = Misc_multiStringList_elementCount(reader_names);

    if (test_length != database->count)
    {
      test_bool = TRUE;
      fetch_result = (test_length > database->count) ?
        WEBCARD_FETCHREADERS__MORE_READERS :
        WEBCARD_FETCHREADERS__LESS_READERS;
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

  if (NULL != jsonReaderNames)
  {
    /* Find differences in two databases */

    if (WEBCARD_FETCHREADERS__MORE_READERS == fetch_result)
    {
      /* Look for the names in OLD (SMALLER) database */
      /* that are not found in the NEW (LARGER) database */
      for (i = 0; i < test_database.count; i++)
      {
        test_bool = SCardReaderDB_hasReaderNamed(
          database,
          test_database.states[i].szReader);

        if (!test_bool)
        {
          WebCard_pushReaderNameToJsonArray(
            &(test_database.states[i]),
            jsonReaderNames);
        }
      }
    }
    else
    {
      /* Look for the names in NEW (SMALLER) database */
      /* that are not found in the OLD (LARGER) database */
      for (i = 0; i < database->count; i++)
      {
        test_bool = SCardReaderDB_hasReaderNamed(
          &(test_database),
          database->states[i].szReader);

        if (!test_bool)
        {
          WebCard_pushReaderNameToJsonArray(
            &(database->states[i]),
            jsonReaderNames);
        }
      }
    }
  }

  /* Destroy previous Smart Card Readers array */

  SCardReaderDB_destroy(database);

  /* Replace outgoing array with the local array */
  /* (direct assignment: local destructor should not be called) */

  database[0] = test_database;

  return fetch_result;
}

/**************************************************************/
