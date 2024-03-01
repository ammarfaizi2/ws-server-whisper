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
	wav_writer		ww;
	std::vector<float>	pcmf32;
};

#define WHISPER_SAMPLE_RATE 44100

struct whisper_params {
	int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
	int32_t step_ms    = 3000;
	int32_t length_ms  = 10000;
	int32_t keep_ms    = 200;
	int32_t capture_id = -1;
	int32_t max_tokens = 32;
	int32_t audio_ctx  = 0;

	float vad_thold    = 0.6f;
	float freq_thold   = 100.0f;

	bool speed_up      = false;
	bool translate     = false;
	bool no_fallback   = false;
	bool print_special = false;
	bool no_context    = true;
	bool no_timestamps = false;
	bool tinydiarize   = false;
	bool save_audio    = false; // save audio to wav file
	bool use_gpu       = true;

	std::string language  = "en";
	std::string model     = "models/ggml-base.en.bin";
	std::string fname_out;
};

static whisper_params params;
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
	const int n_samples_step = (1e-3*params.step_ms  )*WHISPER_SAMPLE_RATE;
	const int n_samples_len  = (1e-3*params.length_ms)*WHISPER_SAMPLE_RATE;
	const int n_samples_keep = (1e-3*params.keep_ms  )*WHISPER_SAMPLE_RATE;
	const int n_samples_30s  = (1e-3*30000.0         )*WHISPER_SAMPLE_RATE;

	ws_client_slot *cl = add_ws_client(hdl);

	std::string ep = s->ctx.get_con_from_hdl(hdl)->get_remote_endpoint();
	std::string fname = ep + ".wav";

	// Replace ':' with '_'
	for (size_t i = 0; i < fname.size(); i++) {
		if (fname[i] == ':')
			fname[i] = '_';
	}

	std::cout << "Creating file " << fname << std::endl;
	cl->ww.open(fname, WHISPER_SAMPLE_RATE, 16, 1);
	cl->fname = fname;
	cl->pcmf32 = std::vector<float>(n_samples_30s, 0.0f);
	cl->ww.write(cl->pcmf32.data(), cl->pcmf32.size());
}

static void on_message(struct ws_server *s, websocketpp::connection_hdl hdl, msg_ptr msg)
{
	std::string ep = s->ctx.get_con_from_hdl(hdl)->get_remote_endpoint();
	ws_client_slot *cl = find_ws_client(hdl);

	if (!cl)
		return;

	const auto &payload = msg->get_payload();
	size_t len = payload.size();

	cl->pcmf32.clear();
	cl->pcmf32.resize(len/sizeof(float));
	memcpy(cl->pcmf32.data(), payload.data(), len);
	cl->ww.write(cl->pcmf32.data(), cl->pcmf32.size());
	std::cout << "Received " << len << " bytes from " << ep << std::endl;

	// for (size_t i = 0; i < len/sizeof(float); i++)
	// 	std::cout << cl->pcmf32[i] << " ";

	// std::cout << std::endl;
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

int main(void)
{
	try {
		run_ws_server("0.0.0.0", 9002);
		return 0;
	} catch (const std::exception & e) {
		std::cout << "Error: " << e.what() << std::endl;
		return 1;
	}
}
