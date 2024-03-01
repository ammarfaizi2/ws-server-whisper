
CXX = g++
CXXFLAGS = -I./modules/websocketpp -std=c++11 -Wall -Wextra -O2 -ggdb3
LIBS = -lboost_system -lboost_thread -lpthread

HEADER_FILES = ws_server.h stream_worker.h

all: ws_server

ws_server: main.o ws_server.o stream_worker.o
	$(CXX) $(CXXFLAGS) -o ws_server $^ $(LIBS)

main.o: main.cpp $(HEADER_FILES)
	$(CXX) $(CXXFLAGS) -o $(@) -c $(<)

ws_server.o: ws_server.cpp $(HEADER_FILES)
	$(CXX) $(CXXFLAGS) -o $(@) -c $(<)

stream_worker.o: stream_worker.cpp $(HEADER_FILES)
	$(CXX) $(CXXFLAGS) -o $(@) -c $(<)

clean:
	rm -vf *.o ws_server

.PHONY: all clean
