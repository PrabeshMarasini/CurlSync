CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lcurl -lssl -lcrypto -lpthread

SRC_DIR = src
BUILD_DIR = build
TARGET = curlsync

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean install uninstall test

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
		mkdir -p $(BUILD_DIR)

clean:
		rm -rf $(BUILD_DIR) $(TARGET)

install: $(TARGET)
		install -m 0755 $(TARGET) /usr/local/bin/
		mkdir -p /etc/curlsync
		cp -n config/urls.config /etc/curlsync/urls.config.example || true

uninstall:
		rm -f /usr/local/bin/$(TARGET)

test: $(TARGET)
		@echo "Running tests..."
		@echo "Test suite not yet implemented"

help:
		@echo "CurlSync Makefile"
		@echo "================="
		@echo "Targets:"
		@echo " all 		- Build the project (default)"
		@echo " clean 		- Remove build files"
		@echo " install		- Install to /usr/local/bin"
		@echo " uninstall	- Remove from /usr/local/bin"
		@echo " test		- Run tests"
		@echo " help		- Show this help message"