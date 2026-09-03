CXX      = g++
PREFIX   ?= /usr
SYSCONFDIR ?= /etc

# Include paths
CPPFLAGS = -Iinclude -Iinclude/managers -Iinclude/helpers

# Flags shared between profiles
CXXFLAGS_COMMON = -Wall -Wextra -Wpedantic -std=c++23

# Release profile: optimised, no sanitizers
CXXFLAGS_RELEASE = $(CXXFLAGS_COMMON) -O2 -DNDEBUG
LDFLAGS_RELEASE  =

# Debug profile: sanitizers, no optimisation
CXXFLAGS_DEBUG = $(CXXFLAGS_COMMON) -O0 -g \
                 -fsanitize=address,undefined \
                 -fno-omit-frame-pointer
LDFLAGS_DEBUG  = -fsanitize=address,undefined

LDLIBS  = -lX11

TARGET    = build/beanwm
BUILD_DIR = build
OBJ_DIR   = build/obj

SRC = src/main.cpp \
      $(wildcard src/managers/*.cpp) \
      $(wildcard src/helpers/*.cpp)

# Map src/**/*.cpp → build/obj/**/*o preserving subdirectory structure
OBJ = $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(SRC))


.DEFAULT_GOAL := release

release: CXXFLAGS := $(CXXFLAGS_RELEASE)
release: LDFLAGS  := $(LDFLAGS_RELEASE)
release: $(TARGET)

debug: CXXFLAGS := $(CXXFLAGS_DEBUG)
debug: LDFLAGS  := $(LDFLAGS_DEBUG)
debug: $(TARGET)

$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

install: release
	install -Dm755 $(TARGET)             $(DESTDIR)$(PREFIX)/bin/beanwm
	install -Dm644 config/config.default $(DESTDIR)$(SYSCONFDIR)/beanwm/config

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/beanwm
	rm -f $(DESTDIR)$(SYSCONFDIR)/beanwm/config


clean:
	rm -rf $(BUILD_DIR)

run: release
	./$(TARGET)

test: release
	DISPLAY=:2 ./$(TARGET)

.PHONY: release debug install uninstall clean run test