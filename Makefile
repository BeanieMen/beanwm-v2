CXX = g++

CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++23 -g -O0 \
           -fsanitize=address,undefined \
           -fno-omit-frame-pointer

CPPFLAGS = -Iinclude

LDFLAGS = -fsanitize=address,undefined
LDLIBS = -lX11

TARGET = build/beanwm
BUILD_DIR = build

SRC = src/main.cpp \
      src/window_manager.cpp \
      src/keybindings.cpp \
      src/management.cpp

.DEFAULT_GOAL := $(TARGET)

include/config.h: include/config.def.h
	@echo "Creating $@ from $< — edit $@ to customize"
	@cp $< $@

$(TARGET): $(SRC) include/config.h
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

test: $(TARGET)
	DISPLAY=:2 ./$(TARGET)

config:
	cp include/config.def.h include/config.h
	@echo "Reset configs from include/config.def.h"