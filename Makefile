C++ := g++

APP = yatou

ifndef os
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
		os = LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
		os = UNIX
    endif
endif

################################################################################
# Paths
################################################################################
# The name of the library
  SO_NAME         = yatou.so
# The full instal path of the library
  SO_PATH         = $(abspath $(dir mkfile_path))/$(SO_NAME)
# The name of the binary
  BIN_NAME         = yatou
# Include directories
  INC_DIRS        = include/

################################################################################
# Compiler flags
################################################################################
# Construct include flags from INC_DIRS and dependencies
  INCLUDES        = $(patsubst %,-I%,$(INC_DIRS))
# Optimization flags
  OPTFLAGS        = -finline-functions -O3
# Error flags
  ERRFLAGS        = -Wall
# File system flags
  FSFLAGS         = -D_LARGE_FILE_SOURCE=1
# Misc flags
  MISCFLAGS       = -g -fPIC -static
# Construct compiler flags
  CCFLAGS         = $(INCLUDES) $(OPTFLAGS) $(FSFLAGS) $(ERRFLAGS) $(MISCFLAGS) -D$(os)
ifeq ($(os), LINUX)
# LD linking flags
  LDFLAGS         = -lstdc++ -lpthread -lm $(patsubst %,-L%,$(INC_DIRS))
# Other linking flags
  LINK_FLAGS      = -shared -Wl,-soname,lyatou.so
endif
ifeq ($(os), UNIX)
# LD linking flags
  LDFLAGS         = -lstdc++ $(patsubst %,-L%,$(INC_DIRS))
# Other linking flags
  LINK_FLAGS      = -shared -Wl,-install_name,lyaout.so
endif

OBJECTS = src/yatou.o

ifndef arch
   arch = IA32
endif

ifeq ($(arch), IA32)
    CCFLAGS += -DIA32
endif

ifeq ($(arch), POWERPC)
   CCFLAGS += -mcpu=powerpc
endif

ifeq ($(arch), IA64)
   CCFLAGS += -DIA64
endif

ifeq ($(arch), SPARC)
   CCFLAGS += -DSPARC
endif

ifeq ($(os), SUNOS)
   LDFLAGS += -lrt -lsocket
endif


all: $(APP)

%.o: %.cc
	$(C++) $(CCFLAGS) $(LDFLAGS) $< -o $@ -c


yatou.so: $(OBJECTS) $(DEP_OBJS)
	$(C++) $(LINK_FLAGS) -o $(SO_PATH) $(DEP_OBJS) $(OBJECTS) $(LIBS)


yatou: $(OBJECTS) $(DEP_OBJS)
	$(C++) -o $(BIN_NAME) $(DEP_OBJS) $(OBJECTS) $(LIBS)


clean:
	rm -f $(OBJECTS) $(DEP_OBJS) $(SO_NAME) $(BIN_NAME)
