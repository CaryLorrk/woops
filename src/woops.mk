PS_SRC = $(shell find $(SRC) -type f -name "*.cc")
PS_OBJ = $(PS_SRC:.cc=.o)
PS_DEP = $(PS_OBJ:.o=.d)

PS_PB_PROTO := $(shell find $(SRC) -type f -name "*.proto")
PS_PB_SRC := $(PS_PB_PROTO:.proto=.pb.cc)
PS_PB_HEADER := $(PS_PB_PROTO:.proto=.pb.h)
PS_PB_OBJ := $(PS_PB_PROTO:.proto=.pb.o)

DEPFLAGS = -MF"$@" -MM -MP -MT"$@" -MT"$(<:.cc=.o)"



.PRECIOUS: $(PS_PB_HEADER) $(PS_PB_SRC) $(PS_DEP)

build: $(PS_LIB)

$(PS_LIB): $(PS_OBJ)
	$(MKDIR) $(NEED_DIR)
	$(AR) $@ $(PS_OBJ)

$(PS_OBJ): $(PS_DEP)
$(PS_DEP): $(PS_PB_SRC)

%.d: %.cc
	$(CXX) $(DEPFLAGS) $(CXXFLAGS) $(INCFLAGS) -c $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

%.pb.cc: %.proto
	$(PROTOC) -I $(shell dirname $@) --cpp_out=$(shell dirname $@) $<


ifneq ($(MAKECMDGOALS),clean)
-include $(PS_DEP)
endif
