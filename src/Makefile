include ../Rules.make

LIBS = -L$(PROJECT_DIR)/src/common -lcommon \
	-L$(PROJECT_DIR)/src/stream -lstream \
	-L$(PROJECT_DIR)/lib/centos7.2_X64 \
	-L$(PROJECT_DIR)/lib/centos7.2_X64/libevent
	
SRCS_C = $(wildcard *.c) 
SRCS_CPP = $(wildcard *.cpp)

OBJS = $(patsubst %.c,%.o,$(SRCS_C)) $(patsubst %.cpp,%.o,$(SRCS_CPP))

ALL = $(EXEC)
all:	$(ALL)

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<       

.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

$(EXEC): $(OBJS) 
	$(C_COMPILER) $(C_FLAGS) -o $(EXEC) $(OBJS) $(LIBS)  -lcommon -lstream -lstpool -lmsglog -levent -levent_pthreads -losipparser2 -losip2 -lpthread -lm
	$(STRIP) $(EXEC)
	mv $(EXEC) $(PROJECT_DIR)/bin/$(EXEC)
	
clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ include/*~
	
	
	
	 