CXX = g++
CXXFLAGS = $(shell pkg-config --cflags libavcodec libavformat libavutil)
LDFLAGS = $(shell pkg-config --libs libavcodec libavformat libavutil)

TARGET = nvenc_streamer
SRC = nvenc_test.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(SRC) -o $(TARGET) $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
