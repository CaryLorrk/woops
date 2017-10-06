WOOPS_ROOT := $(shell pwd)

include $(WOOPS_ROOT)/def.mk

SRC := $(WOOPS_SRC)
LIB := $(WOOPS_LIB)

NEED_DIR = $(LIB)

CXX := $(WOOPS_CXX)
CXXFLAGS := $(WOOPS_CXXFLAGS)
INCFLAGS := $(WOOPS_INCFLAGS) -I$(WOOPS_SRC)
LDFLAGS := $(WOOPS_LDFLAGS)


PROTOC := protoc
CXXFLAGS += -DPROTOBUF_INLINE_NOT_IN_HEADERS=0
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PS_LIB := $(WOOPS_PS_LIB)

RM := rm -rf
AR := ar csr
MKDIR := mkdir -p

all: build

clean:
	$(RM) $(LIB) $(PS_OBJ) $(PS_DEP) $(PS_PB_SRC) $(PS_PB_HEADER) $(PS_PB_OBJ)

.PHONY: all clean allclean build test

include $(SRC)/woops.mk
