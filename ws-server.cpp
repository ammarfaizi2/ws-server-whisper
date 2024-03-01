#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdio>
#include <mutex>
#include <cmath>

#include "wav_writer.h"

typedef websocketpp::server<websocketpp::config::asio> server;
typedef server::message_ptr msg_ptr;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

static wav_writer g_ww;

struct ws_server {
	server ctx;
};

static void on_message(struct ws_server *s, websocketpp::connection_hdl hdl, msg_ptr msg)
{
	const auto &payload = msg->get_payload();
	size_t len = payload.size();

	std::cout << "Client " << hdl.lock().get() << " sent " << len << " bytes" << std::endl;
	g_ww.write(reinterpret_cast<const float *>(payload.data()), len / sizeof(float));
}

static void run_ws_server(const char *addr, uint16_t port)
{
	websocketpp::lib::asio::ip::tcp::endpoint ep(websocketpp::lib::asio::ip::address::from_string(addr), port);
	struct ws_server ws;

	// Set logging settings
	ws.ctx.set_access_channels(websocketpp::log::alevel::all);
	ws.ctx.clear_access_channels(websocketpp::log::alevel::frame_payload);

	// Initialize Asio
	ws.ctx.init_asio();

	// Register our message handler
	ws.ctx.set_message_handler(bind(&on_message, &ws, ::_1, ::_2));
	ws.ctx.listen(ep);
	ws.ctx.start_accept();
	ws.ctx.run();
}

int main(void)
{
	try {
		if (!g_ww.open("output.wav", 16000, 16, 1)) {
			std::cerr << "Failed to open output.wav" << std::endl;
			return 1;
		}

		run_ws_server("0.0.0.0", 9002);
		return 0;
	} catch (const std::exception & e) {
		std::cout << "Error: " << e.what() << std::endl;
		return 1;
	}
}
