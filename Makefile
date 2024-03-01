
CXXFLAGS = -I./modules/websocketpp

ws-server: ws-server.cpp
	$(CXX) $(CXXFLAGS) -o ws-server ws-server.cpp -lboost_system -lboost_thread -lpthread
