CXX = g++

CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++23 -g -O0 \
           -fsanitize=address,undefined \
           -fno-omit-frame-pointer

CPPFLAGS = -Iinclude

LDFLAGS = -fsanitize=address,undefined
LDLIBS = -lX11

TARGET = build/beanwm
BUILD_DIR = build

SRC = $(wildcard src/*.cpp)

$(TARGET): $(SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

test: $(TARGET)
	DISPLAY=:2 ./$(TARGET)