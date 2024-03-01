#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <unordered_map>
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
#include "whisper_channel.hpp"
#include "stream.hpp"

#define WHISPER_SAMPLE_RATE 88200
#define DEBUG_WRITE_TO_WAV 0

typedef websocketpp::server<websocketpp::config::asio> server;
typedef server::message_ptr msg_ptr;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

struct ws_server {
	server ctx;
};

struct ws_client_slot {
	std::string		fname;
	std::thread		routine;
	struct whisper_channel	wc;

#if DEBUG_WRITE_TO_WAV
	wav_writer		ww;
#endif

};

static int g_argc;
static char **g_argv;
static std::mutex g_ws_clients_mutex;
static std::unordered_map<void *, ws_client_slot *> g_ws_clients;

static ws_client_slot *find_ws_client(websocketpp::connection_hdl hdl)
{
	std::lock_guard<std::mutex> lock(g_ws_clients_mutex);

	auto it = g_ws_clients.find(hdl.lock().get());
	if (it != g_ws_clients.end())
		return it->second;

	return nullptr;
}

static ws_client_slot *add_ws_client(websocketpp::connection_hdl hdl)
{
	std::lock_guard<std::mutex> lock(g_ws_clients_mutex);
	void *key = hdl.lock().get();
	ws_client_slot *cl;

	auto it = g_ws_clients.find(key);
	if (it != g_ws_clients.end())
		return it->second;

	cl = new ws_client_slot;
	g_ws_clients[key] = cl;
	return cl;
}

static void del_ws_client(websocketpp::connection_hdl hdl)
{
	std::lock_guard<std::mutex> lock(g_ws_clients_mutex);
	void *key = hdl.lock().get();
	ws_client_slot *cl;

	auto it = g_ws_clients.find(key);
	if (it != g_ws_clients.end()) {
		cl = it->second;
		g_ws_clients.erase(it);
		delete cl;
	}
}

static void on_accept(struct ws_server *s, websocketpp::connection_hdl hdl)
{
	ws_client_slot *cl = add_ws_client(hdl);

	std::string ep = s->ctx.get_con_from_hdl(hdl)->get_remote_endpoint();
	std::string fname = ep + ".wav";

	// Replace ':' with '_'
	for (size_t i = 0; i < fname.size(); i++) {
		if (fname[i] == ':')
			fname[i] = '_';
	}

#if DEBUG_WRITE_TO_WAV
	std::cout << "Creating file " << fname << std::endl;
	cl->ww.open(fname, WHISPER_SAMPLE_RATE, 16, 1);
	cl->fname = fname;
#endif

	cl->routine = std::thread([=] {
		whisper_entry(g_argc, g_argv, &cl->wc);
	});
}

static void on_message(struct ws_server *s, websocketpp::connection_hdl hdl, msg_ptr msg)
{
	std::string ep = s->ctx.get_con_from_hdl(hdl)->get_remote_endpoint();
	ws_client_slot *cl = find_ws_client(hdl);
	std::vector<float> pcmf32;

	if (!cl)
		return;

	const auto &payload = msg->get_payload();
	size_t len = payload.size();

	// Assuming payload is a binary string of s16 PCM data
	const int16_t* pcm16 = reinterpret_cast<const int16_t*>(payload.data());

	// Number of 16-bit samples
	size_t num_samples = len / sizeof(int16_t);

	// Reserve space for float samples
	pcmf32.reserve(num_samples);

	for(size_t i = 0; i < num_samples; ++i) {
		 // Divide by the max value of int16_t to normalize to -1.0 to 1.0
		float sample = pcm16[i] / 32768.0f;
		pcmf32.push_back(sample);
	}

	printf("Received %zu bytes from %s\n", len, ep.c_str());

	// Send to whisper via the channel.
	cl->wc.produce(pcmf32);

#if DEBUG_WRITE_TO_WAV
	// The rest of your processing
	cl->ww.write(pcmf32.data(), pcmf32.size());
#endif

}

static void on_close(struct ws_server *s, websocketpp::connection_hdl hdl)
{
	std::cout << "Client " << hdl.lock().get() << " disconnected" << std::endl;
	del_ws_client(hdl);
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
	ws.ctx.set_open_handler(bind(&on_accept, &ws, ::_1));
	ws.ctx.set_message_handler(bind(&on_message, &ws, ::_1, ::_2));
	ws.ctx.set_close_handler(bind(&on_close, &ws, ::_1));
	ws.ctx.listen(ep);
	ws.ctx.start_accept();
	ws.ctx.run();
}

int main(int argc, char *argv[])
{
	try {
		g_argc = argc;
		g_argv = argv;
		run_ws_server("0.0.0.0", 9002);
		return 0;
	} catch (const std::exception & e) {
		std::cout << "Error: " << e.what() << std::endl;
		return 1;
	}
}
