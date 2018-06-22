PROJECT_DIR :=/root/eclipse-workspace/gb28181_stream_server
DATE = $(shell date '+%Y_%m_%d')

PLATFORM?=X64
BUILD?=RELEASE

CROSS_COMPILE :=
EXEC = gb_stream_server_$(PLATFORM)_$(DATE)

##### Change the following for your environment:
INCLUDES =  -I. -I$(PROJECT_DIR)/inc -I$(PROJECT_DIR)/inc/libevent

COMPILE_OPTS = $(INCLUDES) -O2 -g

C = c
C_COMPILER = $(CROSS_COMPILE)gcc
C_FLAGS = $(COMPILE_OPTS)
CPP = cpp
CPLUSPLUS_COMPILER = $(CROSS_COMPILE)g++
CPLUSPLUS_FLAGS = $(COMPILE_OPTS)
STRIP = $(CROSS_COMPILE)strip
OBJ = o
LINK = $(CROSS_COMPILE)g++ -o
LINK_OPTS = -L.
CONSOLE_LINK_OPTS =	$(LINK_OPTS)
LIBRARY_LINK = $(CROSS_COMPILE)ar cr 
LIBRARY_LINK_OPTS =	
LIB_SUFFIX = a
LIBS_FOR_CONSOLE_APPLICATION =
LIBS_FOR_GUI_APPLICATION =
EXE =
##### End of variables to change


