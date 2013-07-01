include $(TOP_DIR)/prefix.make
LIB_BASE := ../../librorc
LIB := $(LIB_BASE)/lib/$(BUILD_ID)

INCLUDES := -I$(LIB_BASE)/src -I$(LIB_BASE)/include -I../$(FIRMWARE_BASE)
ifdef INSTALL_HOME
	INCLUDES += -I$(INSTALL_HOME)/include
endif

CFLAGS += $(INCLUDES) -L$(LIB) -L`pda-config --ldlibrarypath`
OBJ := $(patsubst %.cpp, $(TOP_DIR)/build/$(BUILD_ID)/%.o, $(wildcard *.cpp))
#OBJ := $(TOP_DIR)/build/$(BUILD_ID)/rorc_status.o 
#OBJ += $(TOP_DIR)/build/$(BUILD_ID)/pcie128pg_nch.o
#OBJ += $(TOP_DIR)/build/$(BUILD_ID)/flashload_htg.o
OBJ += $(patsubst %.c, $(TOP_DIR)/build/$(BUILD_ID)/%.o, $(wildcard *.c))
INCLUDE_TARGETS = $(wildcard $(LIB_BASE)/src/*.hh)
LIBRARY = $(LIB)/librorc.so
#INCLUDE_TARGETS +=../$(FIRMWARE_BASE)/rorc_registers.h 
LOCAL_INCLUDES = $(wildcard *.h)

all: $(LIB) $(OBJ)

$(LIB) : $(INCLUDE_TARGETS)
	make -C $(LIB_BASE) $(BUILD_ID)

$(TOP_DIR)/build/$(BUILD_ID)/%.o: %.cpp $(INCLUDE_TARGETS) $(LOCAL_INCLUDES) $(LIBRARY)
	g++ $(CFLAGS) $< -o $@ -lrorc -lpthread -lpda

$(TOP_DIR)/build/$(BUILD_ID)/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
