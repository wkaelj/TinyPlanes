
BUILD:=build
CLIENT_DIR:=client
SERVER_DIR:=server
SHARED_DIR:=shared
LIBS_DIR:=libs

STD:=gnu2x
CC:=cc

CFLAGS := -Wall -Wextra -pedantic -std=$(STD) -Og -g -I./$(SHARED_DIR) -I./$(LIBS_DIR)
# CFLAGS += -fsanitize=address
CFLAGS += -fopenmp
LDFLAGS:= -lSDL2 -lSDL2_image -lSDL2_ttf -lm

CLIENT_SRC:=$(wildcard $(CLIENT_DIR)/*.c) $(wildcard $(CLIENT_DIR)/**/*.c)
CLIENT_OBJ:=$(CLIENT_SRC:%.c=$(BUILD)/%.o)

SERVER_SRC:=$(wildcard $(SERVER_DIR)/*.c)
SERVER_OBJ:=$(SERVER_SRC:%.c=$(BUILD)/%.o)

SHARED_SRC:=$(wildcard $(SHARED_DIR)/*.c)
SHARED_OBJ:=$(SHARED_SRC:%.c=$(BUILD)/%.o)

NOISE_SRC:=$(wildcard $(LIBS_DIR)/noise1234/*.c)
LIBS_OBJ:=$(LIBS_SRC:%.c=$(BUILD)/%.o)


TEST_SRC:=$(filter-out $(wildcard build/**/main.o),$(CLIENT_OBJ) $(SERVER_OBJ) $(SHARED_OBJ) $(LIBS_SRC))

# used to easily create dirs
CREATE_DIRS := mkdir -p $(BUILD)/$(CLIENT_DIR)/render $(BUILD)/$(SERVER_DIR) 
CREATE_DIRS += $(BUILD)/$(SHARED_DIR) $(BUILD)/$(LIBS_DIR)/noise1234

ifndef VERBOSE
MAKEFLAGS += --silent
endif

.PHONY: run clean

.DEFAULT_GOAL:= run

run: client.out
	./$<

memcheck: client.out
	valgrind --track-origins=yes ./$<

clean:
	rm -r build/*
	$(CREATE_DIRS)

dirs:
	$(CREATE_DIRS) 
 
test: $(TEST_SRC) $(wildcard tests/*.c)
	echo $^
	$(CC) -o tests/test.out -g -O0 $^ $(LDFLAGS) -I./shared -I./client -I./server
	./tests/test.out

client.out: $(CLIENT_OBJ) $(SHARED_OBJ) $(NOISE_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

server.out: $(SERVER_OBJ) $(SHARED_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ -lm 

$(BUILD)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
