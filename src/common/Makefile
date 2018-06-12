include ../../Rules.make

SRCS_C = $(wildcard *.c) 
SRCS_CPP = $(wildcard *.cpp)

OBJS = $(patsubst %.c,%.o,$(SRCS_C)) $(patsubst %.cpp,%.o,$(SRCS_CPP))

COMMON_LIB = libcommon.$(LIB_SUFFIX)
ALL = $(COMMON_LIB)
all:	$(ALL)

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<       

.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

$(COMMON_LIB): $(OBJS) 
	$(LIBRARY_LINK)$@ $(LIBRARY_LINK_OPTS) $(OBJS)
	
clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ include/*~ 
