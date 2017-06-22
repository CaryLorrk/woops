# WOOPS_ROOT must be pre-defined
WOOPS_SRC := $(WOOPS_ROOT)/src
WOOPS_LIB := $(WOOPS_ROOT)/lib

WOOPS_CXX := g++
WOOPS_CXXFLAGS := -O0 \
           -std=c++14 \
		   -fPIC \
           -Wall \
		   -Wextra \
		   -pthread \
		   -g \
		   -fsanitize=address \
		   -fsanitize=leak \
		   -fsanitize=undefined 

WOOPS_INCFLAGS := -I$(WOOPS_SRC)
WOOPS_LDFLAGS := $(shell pkg-config --libs grpc++) \
	$(shell pkg-config --libs protobuf) \
	-L$(WOOPS_LIB) -lwoops

WOOPS_PS_LIB := $(WOOPS_LIB)/libwoops.a
