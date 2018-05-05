HELLOWORLD_ROOT := $(shell pwd)
WOOPS_ROOT := $(HELLOWORLD_ROOT)/../..

include $(WOOPS_ROOT)/def.mk

SRC = $(HELLOWORLD_ROOT)/src
CXX = $(WOOPS_CXX)
CXXFLAGS = $(WOOPS_CXXFLAGS)
INCFLAGS = $(WOOPS_INCFLAGS) -I$(WOOPS_SRC) -I$(SRC)
LDFLAGS = $(WOOPS_LDFLAGS)

WOOPS_HEADER = $(shell find $(WOOPS_SRC) -type f -name "*.h")

HELLOWORLD_SRC = $(shell find $(SRC) -type f -name "*.cc")
HELLOWORLD_HEADER = $(shell find $(SRC) -type f -name "*.h")
HELLOWORLD_OBJ = $(HELLOWORLD_SRC:.cc=.o)
HELLOWORLD_BIN = $(HELLOWORLD_ROOT)/bin
HELLOWORLD_MAIN = $(HELLOWORLD_BIN)/helloworld_main

all: $(HELLOWORLD_MAIN)

$(HELLOWORLD_MAIN): $(HELLOWORLD_OBJ) $(HELLOWORLD_HEADER) $(WOOPS_HEADER) $(WOOPS_ROOT)/lib/libwoops.a
	mkdir -p $(HELLOWORLD_BIN)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $^ -o $@  $(LDFLAGS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

clean:
	rm -rf $(HELLOWORLD_OBJ) $(HELLOWORLD_BIN)

.PHONY: clean
