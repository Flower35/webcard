################################################################
# "Webcard" cross-platform project.
# List of required source files.

WEBCARD_HEADERS = \
  src/wtypes_for_unix.h \
  src/os_specific.h \
  src/misc.h \
  src/utf.h \
  src/json.h \
  src/smart_cards.h

WEBCARD_SOURCES = \
  src/os_specific.c \
  src/misc.c \
  src/utf.c \
  src/json.c \
  src/smart_cards.c \
  src/webcard.c

################################################################
# Detecting Target Operating System and Processor Architecture.
# Selecting: "C Compiler", "Output directory checking command",
#  "Output executable name", "GCC libraries", "GCC flags"

MSG_OS_OK        = Target OS detected as
MSG_OS_MISMATCH  = Unsupported Operating System
MSG_CPU_OK       = Processor Archivecture detected as
MSG_CPU_MISMATCH = Unsupported Processor Architecture

ifeq ($(OS),Windows_NT)
  $(info $(MSG_OS_OK) "Windows")
  CPU = $(PROCESSOR_ARCHITECTURE)

  ifeq ($(CPU),x86)
    $(info $(MSG_CPU_OK) 32-bit ($(CPU)))
    C_COMPILER = mingw32-gcc.exe
    SHELL_DIR_CHECK = IF NOT EXIST x86\ MD x86
    EXEC_WEBCARD = x86/webcard.exe

  else ifeq ($(CPU),AMD64)
    $(info $(MSG_CPU_OK) 64-bit ($(CPU)))
    C_COMPILER = x86_64-w64-mingw32-gcc.exe
    SHELL_DIR_CHECK = IF NOT EXIST x64\ MD x64
    EXEC_WEBCARD = x64/webcard.exe

  else
    $(error $(MSG_CPU_MISMATCH)! ($(CPU)))
  endif

  SHELL_RES_DIR_CHECK = IF NOT EXIST res\out\ MD res\out
  RES_WEBCARD = res\out\webcard.res

  GCC_LIBS =  -lwinscard

else
  OS = $(shell uname -s)

  ifeq ($(OS),Linux)
    $(info $(MSG_OS_OK) "Linux")
    CPU = $(shell uname -m)

    ifeq ($(CPU),i686)
      $(info $(MSG_CPU_OK) 32-bit ($(CPU)))
      C_COMPILER = i686-linux-gnu-gcc
      SHELL_DIR_CHECK = mkdir -p x86
      EXEC_WEBCARD = x86/webcard
      EXEC_WEBCARD_PARENT = x86/webcard_parent

    else ifeq ($(CPU),x86_64)
      $(info $(MSG_CPU_OK) 64-bit ($(CPU)))
      C_COMPILER = x86_64-linux-gnu-gcc
      SHELL_DIR_CHECK = mkdir -p x64
      EXEC_WEBCARD = x64/webcard
      EXEC_WEBCARD_PARENT = x64/webcard_parent

    else
      $(error $(MSG_CPU_MISMATCH)! ($(CPU)))
    endif

    GCC_LIBS = -lpcsclite

  else
    $(error $(MSG_OS_MISMATCH)! ($(OS)))
  endif
endif

GCC_RELEASE_FLAGS = -O3 -s -Wall -pedantic-errors
GCC_DEBUG_FLAGS = -g -Wall -pedantic-errors -D_DEBUG

GCC_FLAGS = $(GCC_RELEASE_FLAGS)

################################################################
# Recipes for specific targets

.PHONY: all

all: $(EXEC_WEBCARD) $(EXEC_WEBCARD_PARENT)

$(EXEC_WEBCARD): $(WEBCARD_HEADERS) $(WEBCARD_SOURCES) $(RES_WEBCARD)
	$(SHELL_DIR_CHECK)
	$(C_COMPILER) $(GCC_FLAGS) -o $@ $(WEBCARD_SOURCES) $(RES_WEBCARD) $(GCC_LIBS)

$(RES_WEBCARD): res/webcard.rc res/webcard.ico
	$(SHELL_RES_DIR_CHECK)
	windres res/webcard.rc -O coff $(RES_WEBCARD)

$(EXEC_WEBCARD_PARENT): test/webcard_parent.c
	$(C_COMPILER) $(GCC_FLAGS) -o $@ test/webcard_parent.c

################################################################
