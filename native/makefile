################################################################
# "Webcard" cross-platform project.
# List of required source files.

WEBCARD_HEADERS = \
  src/json/json.h \
  src/os_specific/os_specific.h \
  src/misc/misc.h \
  src/os_specific/wtypes_for_unix.h \
  src/smart_cards/smart_cards.h \
  src/utf/utf.h

WEBCARD_SOURCES = \
  src/webcard_main.c \
  src/json/json_array.c \
  src/json/json_bytestream.c \
  src/json/json_object.c \
  src/json/json_pair.c \
  src/json/json_string.c \
  src/json/json_value.c \
  src/misc/misc.c \
  src/os_specific/os_specific.c \
  src/smart_cards/sc_conn.c \
  src/smart_cards/sc_db.c \
  src/smart_cards/sc_webcard.c \
  src/utf/utf.c

################################################################
# Detecting Target Operating System and Processor Architecture.
# Selecting: "C Compiler", "Output directory",
#  "Output executable name", "C Compiler libraries".

MSG_OS_OK        = * Target OS detected as
MSG_OS_MISMATCH  = * Unsupported Operating System
MSG_CPU_OK       = * Processor Architecture detected as
MSG_CPU_MISMATCH = * Unsupported Processor Architecture

ifeq ($(OS),Windows_NT)
  $(info $(MSG_OS_OK) "Windows")
  CPU = $(PROCESSOR_ARCHITECTURE)

  ifeq ($(CPU),x86)
    $(info $(MSG_CPU_OK) 32-bit ($(CPU)))
    C_COMPILER = mingw32-gcc.exe
    BINDIR = out/win32
    EXEC_WEBCARD = $(BINDIR)/webcard.exe

  else ifeq ($(CPU),AMD64)
    $(info $(MSG_CPU_OK) 64-bit ($(CPU)))
    C_COMPILER = x86_64-w64-mingw32-gcc.exe
    BINDIR = out/win64
    EXEC_WEBCARD = $(BINDIR)/webcard.exe

  else
    $(error $(MSG_CPU_MISMATCH)! ($(CPU)))
  endif

  RES_WEBCARD = res\out\webcard.res
  C_LIBS =  -lwinscard

else
  OS = $(shell uname -s)

  ifeq ($(OS),Linux)
    $(info $(MSG_OS_OK) "Linux")
    CPU = $(shell uname -m)

    ifeq ($(CPU),i686)
      $(info $(MSG_CPU_OK) 32-bit ($(CPU)))
      C_COMPILER = i686-linux-gnu-gcc
      BINDIR = out/linux32
      EXEC_WEBCARD = $(BINDIR)/webcard

    else ifeq ($(CPU),x86_64)
      $(info $(MSG_CPU_OK) 64-bit ($(CPU)))
      C_COMPILER = x86_64-linux-gnu-gcc
      BINDIR = out/linux64
      EXEC_WEBCARD = $(BINDIR)/webcard

    else
      $(error $(MSG_CPU_MISMATCH)! ($(CPU)))
    endif

    C_LIBS = -lpcsclite

  else ifeq ($(OS),Darwin)
    $(info $(MSG_OS_OK) "macOS")
    CPU = $(shell uname -m)

    ifeq ($(CPU),i686)
      $(info $(MSG_CPU_OK) 32-bit ($(CPU)))

    else ifeq ($(CPU),x86_64)
      $(info $(MSG_CPU_OK) 64-bit ($(CPU)))

    else
      $(error $(MSG_CPU_MISMATCH)! ($(CPU)))
    endif

    C_COMPILER = clang
    BINDIR = out/macOS
    EXEC_WEBCARD = $(BINDIR)/webcard

    C_LIBS = -framework PCSC

  else
    $(error $(MSG_OS_MISMATCH)! ($(OS)))
  endif
endif

################################################################
# Detecting current Shell ("CMD" or "SH").
# Selecting shell-specific commands.

ifeq ($(COMSPEC),)
  COMSPEC = $(ComSpec)
endif
ifneq ($(COMSPEC),)
  SHELL = $(COMSPEC)
  .SHELLFLAGS = /C
  DOS_BINDIR = $(subst /,\,$(BINDIR))
  SHELL_BINDIR_CHECK_01 = IF NOT EXIST out\ MD out
  SHELL_BINDIR_CHECK_02 = IF NOT EXIST $(DOS_BINDIR)\ MD $(DOS_BINDIR)
  SHELL_RESDIR_CHECK = IF NOT EXIST res\out\ MD res\out
  SHELL_WHERE = where
  SHELL_DEVNUL = NUL
else
  SHELL_BINDIR_CHECK_01 = mkdir -p out
  SHELL_BINDIR_CHECK_02 = mkdir -p $(BINDIR)
  SHELL_RESDIR_CHECK = mkdir -p res/out
  SHELL_WHERE = which
  SHELL_DEVNUL = /dev/null
endif

$(info * SHELL is `$(SHELL)`)

################################################################
# Check if all the required build tools are installed.

ifdef RES_WEBCARD
  WINDRES = windres
endif

BUILD_TOOLS = $(WINDRES) $(C_COMPILER)

$(foreach tool,$(BUILD_TOOLS),\
  $(if $(shell $(SHELL_WHERE) $(tool) 2>$(SHELL_DEVNUL)),\
    $(info * "$(tool)" found.),\
    $(error * "$(tool)" not in PATH! Please refer to "README.MD")))

################################################################
# Header files are scanned starting from the "./src" directory

C_FLAGS = -I./src

################################################################
# Recipes for specific targets.
# Selecting "C Compiler flags" depending on the target:
#  "release" (default) or "debug".

.PHONY: release debug

release: C_FLAGS += -O3 -s -Wall -pedantic-errors
release: $(EXEC_WEBCARD)

debug: C_FLAGS += -g -Wall -pedantic-errors -D_DEBUG
debug: $(EXEC_WEBCARD)

$(EXEC_WEBCARD): $(WEBCARD_HEADERS) $(WEBCARD_SOURCES) $(RES_WEBCARD)
	$(info )
	$(SHELL_BINDIR_CHECK_01)
	$(SHELL_BINDIR_CHECK_02)
	$(C_COMPILER) $(C_FLAGS) -o $@ $(WEBCARD_SOURCES) $(RES_WEBCARD) $(C_LIBS)

$(RES_WEBCARD): res/webcard.rc res/webcard.ico
	$(info )
	$(SHELL_RESDIR_CHECK)
	$(WINDRES) res/webcard.rc -O coff $(RES_WEBCARD)

################################################################
