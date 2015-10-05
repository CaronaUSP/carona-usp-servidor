#include <curl/curl.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <cstdio>
#include <csignal>
#include <glog/logging.h>
#include <iostream>
#include <rapidjson/filereadstream.h>

#include "ClientThread.hh"
#include "JSON.hh"
#include "Network.hh"

namespace {
	bool running = true;

	void signal_handler(int signo) {
		if (signo == SIGINT)
			running = false;
	}

	void start_server() {
		Network::TCPServer server(14302);
		int fd;

		struct sigaction signal;
		memset((char *) &signal, 0, sizeof(signal));
		signal.sa_handler = signal_handler;
		if (sigaction(SIGINT, &signal, NULL)) {
			PLOG(ERROR) << "sigaction";
		}

		while (running && ((fd = server.accept()) != -1)) {
			Client::ClientThread client(fd);
		}
	}
}

int main(int argc, char **argv) {
	SSL_load_error_strings();
	SSL_library_init();
	FLAGS_log_dir = ".";
	google::InitGoogleLogging(argv[0]);
	google::InstallFailureSignalHandler();

	CURLcode ret;
	ret = curl_global_init(CURL_GLOBAL_ALL);
	if (ret != CURLE_OK) {
		std::cerr << "curl_global_init(): " << ret << '\n';
		return -1;
	}

	try {
		start_server();
	} catch (std::exception& e) {
		std::cerr << e.what() << '\n';
	}
	google::ShutdownGoogleLogging();
	ERR_free_strings();
	return 0;
}
