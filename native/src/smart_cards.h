/**
 * @file "native/src/smart_cards.h"
 * Communication with Smart Card Readers (physical or virtual peripherals).
 */

#ifndef H_WEBCARD__SMART_CARDS
#define H_WEBCARD__SMART_CARDS

#include "os_specific.h"
#include "misc.h"
#include "utf.h"
#include "json.h"

#if defined(_WIN32)
  #include <winscard.h>

#elif defined(__linux__)
  #include "wtypes_for_unix.h"
  #include <PCSC/winscard.h>

#endif

#ifdef __cplusplus
  extern "C" {
#endif


/**************************************************************/
/* WEBCARD DEFINITIONS                                        */
/**************************************************************/

#define WEBCARD_VERSION  "0.3.1"

#define MAX_APDU_SIZE  0x7FFF

/**
 * Possible "Reader Event" values.
 */
enum webcard_readerevent_enum
{
  WEBCARD_READEREVENT__NONE           = 0,
  WEBCARD_READEREVENT__CARD_INSERTION = 1,
  WEBCARD_READEREVENT__CARD_REMOVAL   = 2,
  WEBCARD_READEREVENT__READERS_MORE   = 3,
  WEBCARD_READEREVENT__READERS_LESS   = 4
};

/**
 * Possible "Webcard Command" values.
 */
enum webcard_command_enum
{
  WEBCARD_COMMAND__NONE         =  0,
  WEBCARD_COMMAND__LIST_READERS =  1,
  WEBCARD_COMMAND__CONNECT      =  2,
  WEBCARD_COMMAND__DISCONNECT   =  3,
  WEBCARD_COMMAND__TRANSCEIVE   =  4,
  WEBCARD_COMMAND__GET_VERSION  = 10,
};

/**
 * Possible return values for `SCardReaderDB_fetch` function.
 */
enum webcard_fetchreaders_enum
{
  WEBCARD_FETCHREADERS__FAIL   = 0,
  WEBCARD_FETCHREADERS__IGNORE = 1,
  WEBCARD_FETCHREADERS__MORE   = 2,
  WEBCARD_FETCHREADERS__LESS   = 3
};


/**************************************************************/
/* SMART CARD CONNECTION                                      */
/**************************************************************/

/**
 * `SCardConnection` is a reference to any
 * `scard_connection_t` structure.
 */
typedef struct scard_connection_t * SCardConnection;

/**
 * `SCardConnection` is a reference to a CONSTANT
 * `scard_connection_t` structure.
 */
typedef struct scard_connection_t const * ConstSCardConnection;

/**
 * Connection parameters for disgnated Smart Card reader.
 */
struct scard_connection_t
{
  /** A handle that identifies the connection to the Smart Card in the designated reader. */
  SCARDHANDLE handle;

  /** A flag that indicates the established active protocol. */
  DWORD activeProtocol;

  /** How many incoming Reader State Changes should be ignored. */
  DWORD ignoreCounter;
};

/**
 * @brief `SCardConnection` constructor.
 *
 * @param[out] connection Reference to an UNINITIALIZED
 * `SCardConnection` object.
 */
extern VOID
SCardConnection_init(
  _Out_ SCardConnection connection);

/**
 * @brief Opens connection to a Smart Card Reader.
 *
 * @param[in,out] connection Reference to a VALID `SCardConnection` object.
 * @param[in] context A handle that identifies the resource manager context.
 * @param[in] readerName The name of the reader that contains the target card.
 * @param[in] shareMode A flag that indicates whether other applications
 * may form connections to the card.
 * @return `TRUE` on success (or if the connection is already open),
 * `FALSE` if the connection was NOT established.
 */
extern BOOL
SCardConnection_open(
  _Inout_ SCardConnection connection,
  _In_ const SCARDCONTEXT context,
  _In_ LPCTSTR readerName,
  _In_ const DWORD shareMode);

/**
 * @brief Closes connection to a Smart Card Reader.
 *
 * @param[in,out] connection Reference to a VALID `SCardConnection` object.
 * @return `TRUE` on success (or if the connection is already closed),
 * `FALSE` if any Smart Card error has occurred.
 */
extern BOOL
SCardConnection_close(
  _Inout_ SCardConnection connection);

/**
 * @brief Sends a service request to the smart card
 * and expects to receive data back from the card.
 *
 * @param[in] connection Reference to a VALID and CONSTANT
 * `SCardConnection` object.
 * @param[in] input Data to be written to the card.
 * @param[in] inputLength The length of `input` buffer, in bytes.
 * @param[out] output Data returned from the card.
 * @param[in,out] outputLengthPointer Supplies the length, in bytes,
 * of the `output` buffer, and receives the actual number of bytes
 * received from the smart card.
 * @return `TRUE` on success (one APDU sent and one APDU received),
 * `FALSE` if any Smart Card error has occurred.
 */
extern BOOL
SCardConnection_transceiveSingle(
  _In_ ConstSCardConnection connection,
  _In_ LPCBYTE input,
  _In_ const DWORD inputLength,
  _Out_ LPBYTE output,
  _Inout_ LPDWORD outputLengthPointer);

/**
 * @brief Sends a large APDU to the smart card
 * and concatenates response to a one large string of data.
 *
 * @param[in] connection Reference to a VALID and CONSTANT
 * `SCardConnection` object.
 * @param[in,out] hexStringResult Refernce to a VALID `UTF8String` object.
 * Reponse in form of hex-string will be appended at the end of this param.
 * @param[in] input Data to be written to the card.
 * @param[in] inputLength The length of `input` buffer, in bytes.
 * @param[out] output Buffer that can be used to collect reponse data.
 * @param[in] outputLength The length of `output` buffer, in bytes.
 * @return `TRUE` on success (one APDU sent and multiple APDUs received),
 * `FALSE` if any Smart Card error has occurred.
 */
extern BOOL
SCardConnection_transceiveMultiple(
  _In_ ConstSCardConnection connection,
  _Inout_ UTF8String hexStringResult,
  _In_ LPCBYTE input,
  _In_ const DWORD inputLength,
  _Out_ LPBYTE output,
  _In_ const DWORD outputLength);


/**************************************************************/
/* SMART CARD READER DATABASE                                 */
/**************************************************************/

/**
 * `SCardReaderDB` is a reference to any
 * `scard_reader_db_t` structure.
 */
typedef struct scard_reader_db_t * SCardReaderDB;

/**
 * `SCardReaderDB` is a reference to a CONSTANT
 * `scard_reader_db_t` structure.
 */
typedef struct scard_reader_db_t const * ConstSCardReaderDB;

/**
 * Database of Smart Card Readers.
 */
struct scard_reader_db_t
{
  /** Number of allocated Smart Card Readers. */
  DWORD count;

  /**
   * Array of `SCARD_READERSTATE` structures, needed for
   * `SCardGetStatusChange()` function.
   */
  LPSCARD_READERSTATE states;

  /**
   * Array of `scard_connection_t` structures, needed for
   * establishing connections and for data transmission.
   */
  SCardConnection connections;
};

/**
 * @brief `SCardReaderDB` constructor.
 *
 * @param[out] database Reference to an UNINITIALIZED `SCardReaderDB` object.
 */
extern VOID
SCardReaderDB_init(
  _Out_ SCardReaderDB database);

/**
 * @brief `SCardReaderDB` destructor.
 *
 * @param[in,out] database Reference to a VALID `SCardReaderDB` object.
 *
 * @note After this call, `database` should not be used (unless re-initialized).
 */
extern VOID
SCardReaderDB_destroy(
  _Inout_ SCardReaderDB database);

/**
 * @brief Prepares a Smart Card Reader Database (list od states
 * and list of connections) from given reader names.
 *
 * @param[out] database Reference to an UNINITIALIZED `SCardReaderDB` object.
 * @param[in] readerNames The head (pointer to the first element)
 * of a multi-string list. This parameter should NOT be `NULL`.
 * @return `TRUE` on successful load, `FALSE` on memory allocation errors.
 *
 * @note After this call, `database` will hold a VALID (at least initialized)
 * `SCardReaderDB` object. If the function returned `FALSE`,
 * `database` shall be destroyed.
 */
extern BOOL
SCardReaderDB_load(
  _Out_ SCardReaderDB database,
  LPCTSTR readerNames);

/**
 * (...)
 */
extern enum webcard_fetchreaders_enum
SCardReaderDB_fetch(
  _Inout_ SCardReaderDB database,
  _In_ const SCARDCONTEXT context,
  _In_ const BOOL firstFetch);


/**************************************************************/
/* WEBCARD OPERATIONS                                         */
/**************************************************************/

#if defined(_DEBUG)

  /**
   * (...)
   */
  extern LPCSTR
  WebCard_errorLookup(
    _In_ const LONG errorCode);

#endif

/**
 * (...)
 */
extern BOOL
WebCard_init(
  _Out_ SCardReaderDB resultDatabase,
  _Out_ LPSCARDCONTEXT resultContext);

/**
 * (...)
 */
extern VOID
WebCard_close(
  _Inout_ SCardReaderDB database,
  _In_ const SCARDCONTEXT context);

/**
 * (...)
 */
extern VOID
WebCard_run(void);

/**
 * (...)
 */
extern VOID
WebCard_handleRequest(
  _Inout_ JsonByteStream jsonStream,
  _Out_ JsonObject jsonRequest,
  _Out_ JsonObject jsonResponse,
  _In_ ConstSCardReaderDB database,
  _In_ const SCARDCONTEXT context);

/**
 * (...)
 */
extern BOOL
WebCard_pushReaderNameToJsonObject(
  _In_ SCARD_READERSTATE const * reader,
  _Inout_ JsonObject jsonObject,
  _In_ LPCSTR key);

/**
 * (...)
 */
extern BOOL
WebCard_convertReaderStateToJsonObject(
  _In_ SCARD_READERSTATE const * reader,
  _Out_ JsonObject jsonReaderObject);

/**
 * (...)
 */
extern BOOL
WebCard_pushReaderAtrToJsonObject(
  _In_ SCARD_READERSTATE const * reader,
  _Inout_ JsonObject jsonObject,
  _In_ LPCSTR key);

/**
 * (...)
 */
extern BOOL
WebCard_convertReaderStatesToJsonArray(
  _In_ ConstSCardReaderDB database,
  _Out_ JsonArray jsonReadersArray);

/**
 * (...)
 */
extern BOOL
WebCard_pushReadersListToJsonResponse(
  _Inout_ JsonObject jsonResponse,
  _In_ ConstSCardReaderDB database);

/**
 * (...)
 */
extern BOOL
WebCard_tryConnectingToReader(
  _In_ ConstJsonObject jsonRequest,
  _Inout_ JsonObject jsonResponse,
  _In_ ConstSCardReaderDB database,
  _In_ const SCARDCONTEXT context);

/**
 * (...)
 */
extern BOOL
WebCard_tryDisconnectingFromReader(
  _In_ ConstJsonObject jsonRequest,
  _In_ ConstSCardReaderDB database);

/**
 * (...)
 */
extern BOOL
WebCard_transmitAndReceive(
  _In_ ConstJsonObject jsonRequest,
  _Inout_ JsonObject jsonResponse,
  _In_ ConstSCardReaderDB database);

/**
 * (...)
 */
extern VOID
WebCard_sendReaderEvent(
  _In_ SCARD_READERSTATE const * reader,
  _In_ const size_t readerIndex,
  _In_ const enum webcard_readerevent_enum readerEvent,
  _Out_ JsonObject jsonResponse);

/**
 * (...)
 */
extern VOID
WebCard_handleStatusChange(
  _Inout_ SCardReaderDB database,
  _In_ const SCARDCONTEXT context);


/**************************************************************/

#ifdef __cplusplus
  }
#endif

#endif  /* H_WEBCARD__SMART_CARDS */
