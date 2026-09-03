CXX = g++

CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++23 -g -O0 \
           -fsanitize=address,undefined \
           -fno-omit-frame-pointer

CPPFLAGS = -Iinclude -Iinclude/managers -Iinclude/helpers

LDFLAGS = -fsanitize=address,undefined
LDLIBS = -lX11

TARGET = build/beanwm
BUILD_DIR = build

SRC = src/main.cpp $(wildcard src/managers/*.cpp) $(wildcard src/helpers/*.cpp)

.DEFAULT_GOAL := $(TARGET)

include/helpers/config.h: include/helpers/config.def.h
	@echo "Creating $@ from $< — edit $@ to customize"
	@cp $< $@

$(TARGET): $(SRC) include/helpers/config.h
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

test: $(TARGET)
	DISPLAY=:2 ./$(TARGET)

config:
	cp include/helpers/config.def.h include/helpers/config.h
	@echo "Reset configs from include/helpers/config.def.h"