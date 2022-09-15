/**
 * @file "native/src/json.c"
 * Simplified handling of the JSON data.
 */

#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**************************************************************/

enum json_byte_stream_enum
JsonByteStream_loadFromStandardInput(
  _Out_ JsonByteStream stream)
{
  BOOL test_bool;
  os_specific_stream_t stdin_stream;
  size_t pipe_length;
  uint32_t json_length;

  /* Get Standard Input stream identifier */

  #if defined(_WIN32)
  {
    stdin_stream = GetStdHandle(STD_INPUT_HANDLE);

    if (INVALID_HANDLE_VALUE == stdin_stream)
    {
      return JSON_BYTESTREAM_NOMORE;
    }
  }
  #elif defined(__linux__)
  {
    stdin_stream = STDIN_FILENO;
  }
  #endif

  /* Check if the pipe is not broken */
  /* and if any message is pending to be read */

  test_bool = OSSpecific_peekStream(
    stdin_stream,
    &(pipe_length));

  if (!test_bool)
  {
    return JSON_BYTESTREAM_NOMORE;
  }

  if (0 == pipe_length)
  {
    /* Go back to the program's main loop */
    return JSON_BYTESTREAM_EMPTY;
  }

  /* Read the first four bytes (INT32) */
  /* ("native byte order", no need to check for endiannes) */

  test_bool = OSSpecific_readBytesFromStream(
    stdin_stream,
    &(json_length),
    sizeof(uint32_t));

  if (!test_bool)
  {
    return JSON_BYTESTREAM_NOMORE;
  }

  #if defined(_DEBUG)
    OSSpecific_writeDebugMessage("{JsonByteStream::loadFromStandardInput} 0x%08X/0x%08X bytes on STDIN", json_length, pipe_length);
  #endif

  /* Validate given text length */

  if ((0 == json_length) ||
    (UINT32_MAX == json_length) ||
    ((pipe_length - sizeof(uint32_t)) != json_length))
  {
    #if defined(_DEBUG)
      OSSpecific_writeDebugMessage("{JsonByteStream::loadFromStandardInput} invalid stream length!");
    #endif

    return JSON_BYTESTREAM_NOMORE;
  }

  /* Initialize "JsonByteStream" object */

  stream->head = malloc(sizeof(BYTE) * json_length);
  if (NULL == (stream->head))
  {
    #if defined(_DEBUG)
      OSSpecific_writeDebugMessage("{JsonByteStream::loadFromStandardInput} memory allocation failed!");
    #endif

    return JSON_BYTESTREAM_NOMORE;
  }

  stream->head_length = json_length;

  /* Read the rest of the STDIN stream (UTF-8 text) */

  test_bool = OSSpecific_readBytesFromStream(
    stdin_stream,
    &(stream->head[0]),
    json_length);

  if (!test_bool)
  {
    free(stream->head);
    return JSON_BYTESTREAM_NOMORE;
  }

  /* "JsonByteStream" object is now ready to be parsed */

  stream->tail = stream->head;
  stream->tail_length = json_length;

  return JSON_BYTESTREAM_VALID;
}

/**************************************************************/

VOID
JsonByteStream_destroy(
  _Inout_ JsonByteStream stream)
{
  if (NULL != stream->head)
  {
    free(stream->head);
    stream->head = NULL;
  }
}

/**************************************************************/

BOOL
JsonByteStream_peek(
  _In_ ConstJsonByteStream stream,
  _Out_ LPBYTE byte)
{
  if (stream->tail_length > 0)
  {
    byte[0] = stream->tail[0];
    return TRUE;
  }

  #if defined(_DEBUG)
  OSSpecific_writeDebugMessage(
    "JSON byte stream, peek failed: no more bytes");
  #endif

  return FALSE;
}

/**************************************************************/

VOID
JsonByteStream_skip(
  _Inout_ JsonByteStream stream,
  _In_ const size_t count)
{
  stream->tail += count;
  stream->tail_length -= count;
}

/**************************************************************/

BOOL
JsonByteStream_read(
  _Inout_ JsonByteStream stream,
  _Out_ LPBYTE output,
  _In_ const size_t count)
{
  if (stream->tail_length < count)
  {
    #if defined(_DEBUG)
    OSSpecific_writeDebugMessage(
      "JSON byte stream, read failed: no more bytes");
    #endif

    return FALSE;
  }

  memcpy(output, stream->tail, count);
  JsonByteStream_skip(stream, count);
  return TRUE;
}

/**************************************************************/

BOOL
JsonByteStream_skipWhitespace(
  _Inout_ JsonByteStream stream)
{
  BYTE test_byte;

  while (JsonByteStream_peek(stream, &(test_byte)))
  {
    switch (test_byte)
    {
      case ' ':  /* space */
      case '\n': /* line feed */
      case '\r': /* carriage return */
      case '\t': /* horizontal tab */
      {
        break;
      }

      default:
      {
        return TRUE;
      }
    }

    JsonByteStream_skip(stream, 1);
  }

  return FALSE;
}

/**************************************************************/

BOOL
JsonString_parse(
  _Outptr_result_maybenull_ UTF8String * const result,
  _In_ const BOOL allocate,
  _Inout_ JsonByteStream stream)
{
  BYTE test_byte;
  BOOL test_bool;

  if (allocate)
  {
    result[0] = malloc(sizeof(struct utf8_string_t));
    if (NULL == result[0]) { return FALSE; }
  }
  UTF8String_init(result[0]);

  /* String starts with '"' */

  if (!JsonByteStream_read(stream, &(test_byte), 1) || ('"' != test_byte))
  {
    #if defined(_DEBUG)
    OSSpecific_writeDebugMessage(
      "JSON string, parsing failed: expected an opening quote");
    #endif

    return FALSE;
  }

  while (JsonByteStream_read(stream, &(test_byte), 1))
  {
    if (test_byte < ' ')
    {
      #if defined(_DEBUG)
      OSSpecific_writeDebugMessage(
        "JSON string, parsing failed: unexpected character 0x%02X",
        test_byte);
      #endif

      return FALSE;
    }
    else if (0x80 & test_byte)
    {
      /* Multibyte codepoint */

      size_t remaining_bytes = stream->tail_length;

      test_bool = UTF8_validateTransformation(
        &(stream->tail[-1]),
        &(remaining_bytes),
        NULL);

      if (!test_bool)
      {
        #if defined(_DEBUG)
        OSSpecific_writeDebugMessage(
          "JsonString::parse(): not a valid UTF-8 representation!");
        #endif

        return FALSE;
      }

      test_bool = UTF8String_pushText(
        result[0],
        (LPCSTR) &(stream->tail[-1]),
        remaining_bytes);

      JsonByteStream_skip(stream, (remaining_bytes - 1));

      if (!test_bool) { return FALSE; }
    }
    else if ('"' == test_byte)
    {
      /* String ends with '"' */
      return TRUE;
    }
    else
    {
      if ('\\' == test_byte)
      {
        if (!JsonByteStream_read(stream, &(test_byte), 1))
        {
          return FALSE;
        }

        switch (test_byte)
        {
          case '"':
          case '\\':
          case '/':
            break;
          case 'b':
            test_byte = '\b';
            break;
          case 'f':
            test_byte = '\f';
            break;
          case 'n':
            test_byte = '\n';
            break;
          case 'r':
            test_byte = '\r';
            break;
          case 't':
            test_byte = '\t';
            break;
          default:
            #if defined(_DEBUG)
            OSSpecific_writeDebugMessage(
              "JSON stream, parsing failed: unknown escape sequence 0x%02X",
              test_byte);
            #endif
            return FALSE;
        }
      }

      if (!UTF8String_pushByte(result[0], test_byte))
      {
        return FALSE;
      }
    }
  }

  return FALSE;
}

/**************************************************************/

BOOL
JsonString_toString(
  _In_ ConstUTF8String string,
  _Inout_ UTF8String output)
{
  BYTE test_bytes[2] = { 0x00 };
  BOOL test_bool;

  if (!UTF8String_pushByte(output, '"'))
  {
    return FALSE;
  }

  for (size_t i = 0; i < string->length;  i++)
  {
    test_bytes[1] = string->text[i];

    test_bool = TRUE;

    switch (test_bytes[1])
    {
      case '"':
      case '\\':
      {
        break;
      }
      case '\b':
      {
        test_bytes[1] = 'b';
        break;
      }
      case '\f':
      {
        test_bytes[1] = 'f';
        break;
      }
      case '\n':
      {
        test_bytes[1] = 'n';
        break;
      }
      case '\r':
      {
        test_bytes[1] = 'r';
        break;
      }
      case '\t':
      {
        test_bytes[1] = 't';
        break;
      }
      default:
      {
        test_bool = FALSE;
      }
    }

    if (test_bool)
    {
      /* Character escaped in a short form */

      if (!UTF8String_pushByte(output, '\\'))
      {
        return FALSE;
      }

      if (!UTF8String_pushByte(output, test_bytes[1]))
      {
        return FALSE;
      }
    }
    else if (0x80 & test_bytes[1])
    {
      /* Multibyte codepoint */

      size_t remaining_bytes = (string->length - i);

      test_bool = UTF8_validateTransformation(
        &(string->text[i]),
        &(remaining_bytes),
        NULL);

      if (!test_bool)
      {
        #if defined(_DEBUG)
        OSSpecific_writeDebugMessage(
          "JsonString::toString(): not a valid UTF-8 representation!");
        #endif

        return FALSE;
      }

      test_bool = UTF8String_pushText(
        output,
        (LPCSTR) &(string->text[i]),
        remaining_bytes);

      if (!test_bool) { return FALSE; }
    }
    else if (test_bytes[1] < ' ')
    {
      /* Control character escaped in a long form */

      if (!UTF8String_pushText(output, "\\u", 2))
      {
        return FALSE;
      }

      if (!UTF8String_pushBytesAsHex(output, 2, test_bytes))
      {
        return FALSE;
      }
    }
    else
    {
      /* Regular ASCII printable character */

      if (!UTF8String_pushByte(output, test_bytes[1]))
      {
        return FALSE;
      }
    }
  }

  return UTF8String_pushByte(output, '"');
}

/**************************************************************/

VOID
JsonValue_init(
  _Out_ JsonValue value)
{
  value->type  = JSON_VALUETYPE_NULL;
  value->value = NULL;
}

/**************************************************************/

VOID
JsonValue_destroy(
  _Inout_ JsonValue value)
{
  if (NULL != value->value)
  {
    switch (value->type)
    {
      case JSON_VALUETYPE_TRUE:
      case JSON_VALUETYPE_FALSE:
      case JSON_VALUETYPE_NULL:
      {
        return;
      }
      default:
      {
        /* Continue with dynamically allocated types */
      }
    }

    switch (value->type)
    {
      case JSON_VALUETYPE_STRING:
      {
        UTF8String_destroy(value->value);
        break;
      }
      case JSON_VALUETYPE_OBJECT:
      {
        JsonObject_destroy(value->value);
        break;
      }
      case JSON_VALUETYPE_ARRAY:
      {
        JsonArray_destroy(value->value);
        break;
      }
      default:
      {
        /* `JSON_VALUETYPE_NUMBER` is just a pointer to one `FLOAT` */
      }
    }

    free(value->value);
  }
}

/**************************************************************/

BOOL
JsonValue_copy(
  _Out_ JsonValue destination,
  _In_ ConstJsonValue source)
{
  size_t bytesize;

  /* Copy the JsonValue type field */

  destination->type = source->type;

  /* Check if the type points to a dynamic object */

  switch (source->type)
  {
    case JSON_VALUETYPE_STRING:
    {
      bytesize = sizeof(struct utf8_string_t);
      break;
    }
    case JSON_VALUETYPE_NUMBER:
    {
      bytesize = sizeof(FLOAT);
      break;
    }
    case JSON_VALUETYPE_OBJECT:
    {
      bytesize = sizeof(struct json_object_t);
      break;
    }
    case JSON_VALUETYPE_ARRAY:
    {
      bytesize = sizeof(struct json_array_t);
      break;
    }
    default:
    {
      destination->value = source->value;
      return TRUE;
    }
  }

  /* Allocate memory for an object to be cloned */

  destination->value = malloc(bytesize);
  if (NULL == destination->value) { return FALSE; }

  /* `destination->value` points to an UNINITIALIZED object of given type */

  switch (source->type)
  {
    case JSON_VALUETYPE_STRING:
    {
      return UTF8String_copy(destination->value, source->value);
    }
    case JSON_VALUETYPE_NUMBER:
    {
      ((FLOAT *) destination->value)[0] = ((FLOAT *) source->value)[0];
      return TRUE;
    }
    case JSON_VALUETYPE_OBJECT:
    {
      return JsonObject_copy(destination->value, source->value);
    }
    case JSON_VALUETYPE_ARRAY:
    {
      return JsonArray_copy(destination->value, source->value);
    }
    default:
    {
      return FALSE;
    }
  }
}

/**************************************************************/

BOOL
JsonValue_parseNumber(
  _Inout_ JsonValue value,
  _Inout_ JsonByteStream stream)
{
  char buf[256];
  char * buf_end = buf;
  char * buf_end_dummy;
  FLOAT number;

  /* 'A': expecting a minus ('-') or a digit ('0'-'9') */
  /* 'B': expecting a digit ('0'-'9') */
  /* 'C': started with zero, expecting a dot ('.') or stream end */
  /* 'D': expecting digits, dot ('.'), 'e', 'E' or stream end */
  /* 'E': started fraction, expecting a digit */
  /* 'F': fraction, expecting digits, 'e', 'E' or stream end */
  /* 'G': started exp, expecting digits, '-' or '+' */
  /* 'H': started exp, expecting expecting a digit */
  /* 'I': exp, expecing digitsor stream end */
  /* 'J': parsing success (whitespace, comma or closing brackets) */
  /* any other state: parsing error (unexpected character) */
  int parser_state = 'A';
  BYTE test_byte;

  /* In case of parsing failure, dynamic value stays NULL */

  value->value = NULL;

  while ((parser_state >= 'A') && (parser_state <= 'I'))
  {
    if ('A' != parser_state)
    {
      if (!Misc_pushToLocalBuffer(buf, &(buf_end), sizeof(buf), test_byte))
      {
        #if defined(_DEBUG)
        OSSpecific_writeDebugMessage(
          "JSON number parsing failed: buffer overflow");
        #endif

        return FALSE;
      }

      JsonByteStream_skip(stream, 1);
    }

    if (!JsonByteStream_peek(stream, &(test_byte)))
    {
      return FALSE;
    }

    switch (parser_state)
    {
      case 'A':
      {
        switch (test_byte)
        {
          case '0':
          {
            parser_state = 'C';
            break;
          }
          case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
          {
            parser_state = 'D';
            break;
          }
          case '-':
          {
            parser_state = 'B';
            break;
          }
          default:
          {
            parser_state = 0;
          }
        }

        break;
      }
      case 'B':
      {
        switch (test_byte)
        {
          case '0':
          {
            parser_state = 'C';
            break;
          }
          case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
          {
            parser_state = 'D';
            break;
          }
          default:
          {
            parser_state = 0;
          }
        }

        break;
      }
      case 'C':
      {
        switch (test_byte)
        {
          case '.':
          {
            parser_state = 'E';
            break;
          }
          case ' ': case '\r': case '\n': case '\t':
          case ',': case ']': case '}':
          {
            parser_state = 'J';
            break;
          }
          default:
          {
            parser_state = 0;
          }
        }

        break;
      }
      case 'D':
      {
        switch (test_byte)
        {
          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
          {
            break;
          }
          case '.':
          {
            parser_state = 'E';
            break;
          }
          case 'E': case 'e':
          {
            parser_state = 'G';
            break;
          }
          case ' ': case '\r': case '\n': case '\t':
          case ',': case ']': case '}':
          {
            parser_state = 'J';
            break;
          }
          default:
          {
            parser_state = 0;
          }
        }

        break;
      }
      case 'E':
      {
        switch (test_byte)
        {
          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
          {
            parser_state = 'F';
            break;
          }
          default:
          {
            parser_state = 0;
          }
        }

        break;
      }
      case 'F':
      {
        switch (test_byte)
        {
          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
          {
            break;
          }
          case 'E': case 'e':
          {
            parser_state = 'G';
            break;
          }
          case ' ': case '\r': case '\n': case '\t':
          case ',': case ']': case '}':
          {
            parser_state = 'J';
            break;
          }
          default:
          {
            parser_state = 0;
          }
        }

        break;
      }
      case 'G':
      {
        switch (test_byte)
        {
          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
          {
            parser_state = 'I';
            break;
          }
          case '-': case '+':
          {
            parser_state = 'H';
            break;
          }
          default:
          {
            parser_state = 0;
          }
        }

        break;
      }
      case 'H':
      {
        switch (test_byte)
        {
          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
          {
            parser_state = 'I';
            break;
          }
          default:
          {
            parser_state = 0;
          }
        }

        break;
      }
      case 'I':
      {
        switch (test_byte)
        {
          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
          {
            break;
          }
          case ' ': case '\r': case '\n': case '\t':
          case ',': case ']': case '}':
          {
            parser_state = 'J';
            break;
          }
          default:
          {
            parser_state = 0;
          }
        }

        break;
      }
    }
  }

  /* Parsing completed, check if any errors occurred */

  if ('J' != parser_state)
  {
    #if defined(_DEBUG)
    OSSpecific_writeDebugMessage(
      "JSON number parsing failed: unexpected character 0x%02X",
      test_byte);
    #endif

    return FALSE;
  }

  /* Otherwise, try to parse a FLOAT */

  buf_end[0] = 0;

  errno = 0;
  number = strtof(buf, &(buf_end_dummy));
  if ((0 != errno) || (buf_end_dummy != buf_end))
  {
    #if defined(_DEBUG)
    OSSpecific_writeDebugMessage(
      "{strtof} failed: 0x%08X",
      errno);
    #endif

    return FALSE;
  }

  value->value = malloc(sizeof(FLOAT));
  if (NULL == value->value)
  {
    return FALSE;
  }

  ((FLOAT *) value->value)[0] = number;
  return TRUE;
}

/**************************************************************/

BOOL
JsonValue_parse(
  _Outptr_result_maybenull_ JsonValue * const result,
  _In_ BOOL allocate,
  _Inout_ JsonByteStream stream)
{
  BYTE test_bytes[2][4];

  if (allocate)
  {
    result[0] = malloc(sizeof(struct json_value_t));
    if (NULL == result[0]) { return FALSE; }
  }
  JsonValue_init(result[0]);

  if (!JsonByteStream_skipWhitespace(stream))
  {
    return FALSE;
  }

  if (!JsonByteStream_peek(stream, &(test_bytes[0][0])))
  {
    return FALSE;
  }

  switch (test_bytes[0][0])
  {
    case '"':
    {
      /* string value */
      result[0]->type = JSON_VALUETYPE_STRING;

      if (!JsonString_parse((UTF8String *) &(result[0]->value), TRUE, stream))
      {
        return FALSE;
      }

      break;
    }
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      /* number value */
      result[0]->type = JSON_VALUETYPE_NUMBER;

      if (!JsonValue_parseNumber(result[0], stream))
      {
        return FALSE;
      }

      break;
    }
    case '{':
    {
      /* object value */
      result[0]->type = JSON_VALUETYPE_OBJECT;

      if (!JsonObject_parse((JsonObject *) &(result[0]->value), TRUE, stream))
      {
        return FALSE;
      }

      break;
    }
    case '[':
    {
      /* array value */
      result[0]->type = JSON_VALUETYPE_ARRAY;

      if (!JsonArray_parse((JsonArray *) &(result[0]->value), stream))
      {
        return FALSE;
      }

      break;
    }
    case 't':
    {
      /* true value */
      result[0]->type = JSON_VALUETYPE_TRUE;

      if (!JsonByteStream_read(stream, test_bytes[0], 3))
      {
        return FALSE;
      }

      test_bytes[1][0] = 'r';
      test_bytes[1][1] = 'u';
      test_bytes[1][2] = 'e';

      if (0 != memcmp(test_bytes[0], test_bytes[1], 3))
      {
        #if defined(_DEBUG)
        OSSpecific_writeDebugMessage(
          "JSON value, parsing failed: expected literal 'true'");
        #endif

        return FALSE;
      }

      break;
    }
    case 'f':
    {
      /* false value */
      result[0]->type = JSON_VALUETYPE_FALSE;

      if (!JsonByteStream_read(stream, test_bytes[0], 4))
      {
        return FALSE;
      }

      test_bytes[1][0] = 'a';
      test_bytes[1][1] = 'l';
      test_bytes[1][2] = 's';
      test_bytes[1][3] = 'e';

      if (0 != memcmp(test_bytes[0], test_bytes[1], 4))
      {
        #if defined(_DEBUG)
        OSSpecific_writeDebugMessage(
          "JSON value, parsing failed: expected literal 'false'");
        #endif

        return FALSE;
      }

      break;
    }
    case 'n':
    {
      /* null value */
      result[0]->type = JSON_VALUETYPE_NULL;

      if (!JsonByteStream_read(stream, test_bytes[0], 3))
      {
        return FALSE;
      }

      test_bytes[1][0] = 'u';
      test_bytes[1][1] = 'l';
      test_bytes[1][2] = 'l';

      if (0 != memcmp(test_bytes[0], test_bytes[1], 3))
      {
        #if defined(_DEBUG)
        OSSpecific_writeDebugMessage(
          "JSON value, parsing failed: expected literal 'null'");
        #endif

        return FALSE;
      }

      break;
    }
    default:
    {
      return FALSE;
    }
  }

  if (!JsonByteStream_skipWhitespace(stream))
  {
    return FALSE;
  }

  if (!JsonByteStream_peek(stream, &(test_bytes[0][0])))
  {
    return FALSE;
  }

  switch (test_bytes[0][0])
  {
    case ' ': case '\r': case '\n': case '\t':
    case ',': case ']': case '}':
    {
      return TRUE;
    }
    default:
    {
      #if defined(_DEBUG)
      OSSpecific_writeDebugMessage(
        "JSON value, parsing failed: unexpected character 0x%02X",
        test_bytes[0][0]);
      #endif

      return FALSE;
    }
  }
}

/**************************************************************/

BOOL
JsonValue_toString(
  _In_ ConstJsonValue value,
  _Inout_ UTF8String output)
{
  char number_buffer[64];
  FLOAT value_number;

  switch (value->type)
  {
    case JSON_VALUETYPE_STRING:
    {
      return JsonString_toString(value->value, output);
    }
    case JSON_VALUETYPE_NUMBER:
    {
      value_number = ((FLOAT *) value->value)[0];
      snprintf(number_buffer, 64, "%.f", value_number);
      return UTF8String_pushText(output, number_buffer, 0);
    }
    case JSON_VALUETYPE_OBJECT:
    {
      return JsonObject_toString(value->value, output);
    }
    case JSON_VALUETYPE_ARRAY:
    {
      return JsonArray_toString(value->value, output);
    }
    case JSON_VALUETYPE_TRUE:
    {
      return UTF8String_pushText(output, "true", 4);
    }
    case JSON_VALUETYPE_FALSE:
    {
      return UTF8String_pushText(output, "false", 5);
    }
    default:
    {
      return UTF8String_pushText(output, "null", 4);
    }
  }
}

/**************************************************************/

VOID
JsonArray_init(
  _Out_ JsonArray array)
{
  array->count    = 0;
  array->capacity = 0;
  array->values   = NULL;
}

/**************************************************************/

VOID
JsonArray_destroy(
  _Inout_ JsonArray array)
{
  if (NULL != array->values)
  {
    for (size_t i = 0; i < array->count; i++)
    {
      JsonValue_destroy(&(array->values[i]));
    }

    free(array->values);
  }
}

/**************************************************************/

BOOL
JsonArray_copy(
  _Out_ JsonArray destination,
  _In_ ConstJsonArray source)
{
  if (0 == source->count)
  {
    JsonArray_init(destination);
    return TRUE;
  }

  const size_t capacity = Misc_nextPowerOfTwo(source->count - 1);
  const size_t bytesize = sizeof(struct json_value_t) * capacity;

  destination->count = 0;
  destination->capacity = capacity;

  destination->values = malloc(bytesize);
  if (NULL == destination->values) { return FALSE; }

  for (size_t i = 0; i < source->count; i++)
  {
    BOOL test_bool = JsonValue_copy(&(destination->values[i]), &(source->values[i]));
    destination->count += 1;
    if (!test_bool) { return FALSE; }
  }

  return TRUE;
}

/**************************************************************/

BOOL
JsonArray_append(
  _Inout_ JsonArray array,
  _In_ ConstJsonValue value)
{
  size_t new_capacity = (array->count + 1);

  if (new_capacity > array->capacity)
  {
    new_capacity = Misc_nextPowerOfTwo(new_capacity - 1);

    const size_t new_bytesize = sizeof(struct json_value_t) * new_capacity;
    JsonValue new_values = realloc(array->values, new_bytesize);
    if (NULL == new_values) { return FALSE; }

    array->values = new_values;
    array->capacity = new_capacity;
  }

  BOOL test_bool = JsonValue_copy(
    &(array->values[array->count]),
    value);

  array->count += 1;
  return test_bool;
}

/**************************************************************/

BOOL
JsonArray_parse(
  _Outptr_result_maybenull_ JsonArray * const result,
  _Inout_ JsonByteStream stream)
{
  BOOL test_bool;
  BYTE test_byte;
  struct json_value_t json_value;
  JsonValue json_value_ptr = &(json_value);

  result[0] = malloc(sizeof(struct json_array_t));
  if (NULL == result[0]) { return FALSE; }
  JsonArray_init(result[0]);

  /* Array starts with '[' */

  if (!JsonByteStream_read(stream, &(test_byte), 1) || ('[' != test_byte))
  {
    #if defined(_DEBUG)
    OSSpecific_writeDebugMessage(
      "JSON array, parsing failed: expected an opening square bracket");
    #endif

    return FALSE;
  }

  if (!JsonByteStream_skipWhitespace(stream))
  {
    return FALSE;
  }

  while (JsonByteStream_peek(stream, &(test_byte)))
  {
    if (']' == test_byte)  /* Array ends with ']' */
    {
      JsonByteStream_skip(stream, 1);
      return TRUE;
    }

    if (',' == test_byte)
    {
      if (0 == result[0]->count)
      {
        #if defined(_DEBUG)
        OSSpecific_writeDebugMessage(
          "JSON array, parsing failed: unexpected comma");
        #endif

        return FALSE;
      }

      JsonByteStream_skip(stream, 1);
    }
    else
    {
      if (0 != result[0]->count)
      {
        #if defined(_DEBUG)
        OSSpecific_writeDebugMessage(
          "JSON value, parsing failed: expected a comma");
        #endif

        return FALSE;
      }
    }

    test_bool = JsonValue_parse(&(json_value_ptr), FALSE, stream);

    if (test_bool)
    {
      test_bool = JsonArray_append(result[0], json_value_ptr);
    }

    JsonValue_destroy(json_value_ptr);

    if (!test_bool)
    {
      return FALSE;
    }
  }

  return FALSE;
}

/**************************************************************/

BOOL
JsonArray_toString(
  _In_ ConstJsonArray array,
  _Inout_ UTF8String output)
{
  if (!UTF8String_pushByte(output, '['))
  {
    return FALSE;
  }

  for (size_t i = 0; i < array->count; i++)
  {
    if (!JsonValue_toString(&(array->values[i]), output))
    {
      return FALSE;
    }

    if (i < (array->count - 1))
    {
      if (!UTF8String_pushByte(output, ','))
      {
        return FALSE;
      }
    }
  }

  return UTF8String_pushByte(output, ']');
}

/**************************************************************/

VOID
JsonPair_init(
  _Out_ JsonPair pair)
{
  UTF8String_init(&(pair->key));
  JsonValue_init(&(pair->value));
}

/**************************************************************/

VOID
JsonPair_destroy(
  _Inout_ JsonPair pair)
{
  UTF8String_destroy(&(pair->key));
  JsonValue_destroy(&(pair->value));
}

/**************************************************************/

BOOL
JsonPair_copy(
  _Out_ JsonPair destination,
  _In_ ConstJsonPair source)
{
  JsonPair_init(destination);

  if (!UTF8String_copy(&(destination->key), &(source->key)))
  {
    return FALSE;
  }

  return JsonValue_copy(&(destination->value), &(source->value));
}

/**************************************************************/

BOOL
JsonPair_parse(
  _Outptr_result_maybenull_ JsonPair * const result,
  _In_ BOOL allocate,
  _Inout_ JsonByteStream stream)
{
  BYTE test_byte;

  if (allocate)
  {
    result[0] = malloc(sizeof(struct json_pair_t));
    if (NULL == result[0]) { return FALSE; }
  }
  JsonPair_init(result[0]);

  if (!JsonByteStream_skipWhitespace(stream))
  {
    return FALSE;
  }

  UTF8String key_pointer = &(result[0]->key);
  if (!JsonString_parse(&(key_pointer), FALSE, stream))
  {
    return FALSE;
  }

  if (!JsonByteStream_skipWhitespace(stream))
  {
    return FALSE;
  }

  if (!JsonByteStream_read(stream, &(test_byte), 1) || (':' != test_byte))
  {
    return FALSE;
  }

  JsonValue value_pointer = &(result[0]->value);
  return JsonValue_parse(&(value_pointer), FALSE, stream);
}

/**************************************************************/

BOOL
JsonPair_toString(
  _In_ ConstJsonPair pair,
  _Inout_ UTF8String output)
{
  if (!JsonString_toString(&(pair->key), output))
  {
    return FALSE;
  }

  if (!UTF8String_pushByte(output, ':'))
  {
    return FALSE;
  }

  return JsonValue_toString(&(pair->value), output);
}

/**************************************************************/

VOID
JsonObject_init(
  _Out_ JsonObject object)
{
  object->count    = 0;
  object->capacity = 0;
  object->pairs    = NULL;
}

/**************************************************************/

VOID
JsonObject_destroy(
  _Inout_ JsonObject object)
{
  if (NULL != object->pairs)
  {
    for (size_t i = 0; i < object->count; i++)
    {
      JsonPair_destroy(&(object->pairs[i]));
    }

    free(object->pairs);
  }
}

/**************************************************************/

BOOL
JsonObject_copy(
  _Out_ JsonObject destination,
  _In_ ConstJsonObject source)
{
  if (0 == source->count)
  {
    JsonObject_init(destination);
    return TRUE;
  }

  const size_t capacity = Misc_nextPowerOfTwo(source->count - 1);
  const size_t bytesize = sizeof(struct json_pair_t) * capacity;

  destination->count = 0;
  destination->capacity = capacity;

  destination->pairs = malloc(bytesize);
  if (NULL == destination->pairs) { return FALSE; }

  for (size_t i = 0; i < source->count; i++)
  {
    BOOL test_bool = JsonPair_copy(&(destination->pairs[i]), &(source->pairs[i]));
    destination->count += 1;
    if (!test_bool) { return FALSE; }
  }

  return TRUE;
}

/**************************************************************/

BOOL
JsonObject_appendPair(
  _Inout_ JsonObject object,
  _In_ ConstJsonPair pair)
{
  size_t new_capacity = (object->count + 1);

  if (new_capacity > object->capacity)
  {
    new_capacity = Misc_nextPowerOfTwo(new_capacity - 1);

    const size_t new_bytesize = sizeof(struct json_pair_t) * new_capacity;
    JsonPair new_pairs = realloc(object->pairs, new_bytesize);
    if (NULL == new_pairs) { return FALSE; }

    object->pairs = new_pairs;
    object->capacity = new_capacity;
  }

  BOOL test_bool = JsonPair_copy(
    &(object->pairs[object->count]),
    pair);

  object->count += 1;
  return test_bool;
}

/**************************************************************/

BOOL
JsonObject_appendKeyValue(
  _Inout_ JsonObject object,
  _In_ LPCSTR key,
  _In_ ConstJsonValue value)
{
  struct json_pair_t json_pair;

  UTF8String_makeTemporary(&(json_pair.key), key);

  json_pair.value = value[0];

  return JsonObject_appendPair(object, &(json_pair));
}

/**************************************************************/

BOOL
JsonObject_parse(
  _Outptr_result_maybenull_ JsonObject * const result,
  _In_ BOOL allocate,
  _Inout_ JsonByteStream stream)
{
  BOOL test_bool;
  BYTE test_byte;
  struct json_pair_t json_pair;
  JsonPair json_pair_ptr = &(json_pair);

  if (allocate)
  {
    result[0] = malloc(sizeof(struct json_object_t));
    if (NULL == result[0]) { return FALSE; }
  }
  JsonObject_init(result[0]);

  /* Object starts with '{' */

  if (!JsonByteStream_read(stream, &(test_byte), 1) || ('{' != test_byte))
  {
    #if defined(_DEBUG)
    OSSpecific_writeDebugMessage(
      "JSON object, parsing failed: expected an opening curly bracket");
    #endif

    return FALSE;
  }

  if (!JsonByteStream_skipWhitespace(stream))
  {
    return FALSE;
  }

  while (JsonByteStream_peek(stream, &(test_byte)))
  {
    if ('}' == test_byte)  /* Object ends with '}' */
    {
      JsonByteStream_skip(stream, 1);
      return TRUE;
    }

    if (',' == test_byte)
    {
      if (0 == result[0]->count)
      {
        #if defined(_DEBUG)
        OSSpecific_writeDebugMessage(
          "JSON object, parsing failed: unexpected comma");
        #endif

        return FALSE;
      }

      JsonByteStream_skip(stream, 1);
    }
    else
    {
      if (0 != result[0]->count)
      {
        #if defined(_DEBUG)
        OSSpecific_writeDebugMessage(
          "JSON object, parsing failed: expected a comma");
        #endif

        return FALSE;
      }
    }

    test_bool = JsonPair_parse(&(json_pair_ptr), FALSE, stream);

    if (test_bool)
    {
      test_bool = JsonObject_appendPair(result[0], json_pair_ptr);
    }

    JsonPair_destroy(json_pair_ptr);

    if (!test_bool)
    {
      return FALSE;
    }
  }

  return FALSE;
}

/**************************************************************/

BOOL
JsonObject_toString(
  _In_ ConstJsonObject object,
  _Inout_ UTF8String output)
{
  if (!UTF8String_pushByte(output, '{'))
  {
    return FALSE;
  }

  for (size_t i = 0; i < object->count; i++)
  {
    if (!JsonPair_toString(&(object->pairs[i]), output))
    {
      return FALSE;
    }

    if (i < (object->count - 1))
    {
      if (!UTF8String_pushByte(output, ','))
      {
        return FALSE;
      }
    }
  }

  return UTF8String_pushByte(output, '}');
}

/**************************************************************/

BOOL
JsonObject_getValue(
  _In_ ConstJsonObject object,
  _Out_ JsonValue result,
  _In_ LPCSTR key)
{
  for (size_t i = 0; i < object->count; i++)
  {
    if (UTF8String_matches(&(object->pairs[i].key), key))
    {
      result[0] = object->pairs[i].value;
      return TRUE;
    }
  }

  return FALSE;
}

/**************************************************************/
