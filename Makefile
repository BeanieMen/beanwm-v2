CXX = g++

CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++23 -O3
LDLIBS = -lX11

TARGET = build/beanwm
BUILD_DIR = build

SRC = $(wildcard src/*.cpp)

$(TARGET): $(SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

test: $(TARGET)
	DISPLAY=:2 ./$(TARGET) 