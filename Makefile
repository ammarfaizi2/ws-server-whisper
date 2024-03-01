
CXX = g++
CXXFLAGS = \
	-O3 \
	-std=c++11 \
	-D_XOPEN_SOURCE=600 \
	-D_GNU_SOURCE \
	-march=native \
	-I./modules/websocketpp \
	-I./modules/whisper.cpp \
	-I./modules/whisper.cpp/examples \
	$$(sdl2-config --cflags)

LIBS = -lboost_system -lboost_thread -lpthread $$(sdl2-config --libs)
HEADERS = whisper_channel.hpp stream.hpp

WHISPER_OBJS = \
	./modules/whisper.cpp/ggml.o \
	./modules/whisper.cpp/ggml-alloc.o \
	./modules/whisper.cpp/ggml-backend.o \
	./modules/whisper.cpp/ggml-quants.o \
	./modules/whisper.cpp/whisper.o

WHISPER_CXX_EXTRA = \
	./modules/whisper.cpp/examples/common.cpp \
	./modules/whisper.cpp/examples/common-ggml.cpp \
	./modules/whisper.cpp/examples/common-sdl.cpp

all: ws-server

ws-server: ws-server.o stream.o $(WHISPER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(WHISPER_CXX_EXTRA) $(LIBS)

ws-server.o: ws-server.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $(<) $(LIBS)

stream.o: stream.cpp modules/whisper.cpp/stream $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $(<) $(LIBS)

$(WHISPER_OBJS) modules/whisper.cpp/stream:
	+$(MAKE) -C modules/whisper.cpp/ stream

clean:
	rm -vf *.o ws-server

.PHONY: clean all
