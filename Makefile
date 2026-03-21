SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
INCLUDE_DIR := include

SRCS := $(wildcard $(SRC_DIR)/*.c)
SRC_MAIN := $(SRC_DIR)/befedit.c
SRC_SRCS := $(filter-out $(SRC_MAIN), $(SRCS))

OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_SRCS))

CFLAGS := -Wextra -Werror -Wall -Wimplicit -pedantic -Wreturn-type -Wformat -Wmissing-prototypes -Wstrict-prototypes -std=c89 -O3 -I$(INCLUDE_DIR)

TARGET := $(BIN_DIR)/befedit

all: $(TARGET)

# build target
$(TARGET): $(OBJS) $(BUILD_DIR)/befedit.o | $(BIN_DIR)
	gcc $^ -o $@ -lm
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@
$(BUILD_DIR)/befedit.o: $(SRC_MAIN) | $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

# create directories if missing
$(BIN_DIR) $(BUILD_DIR) $(TEST_BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

