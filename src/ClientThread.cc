#include "ClientThread.hh"

#include <arpa/inet.h>

#include <cstdint>
#include <glog/logging.h>
#include <iostream>
#include <random>
#include <thread>

#include "JSON.hh"

namespace Client {
	class Client {
	public:
		std::string ip;

		Client(int fd) :fd(fd) {
			save_ip();
			try {
				LOG(INFO) << "New thread " << ip;
				JSON::Socket socket(fd);
				JSON::GlobalConfig config;
				int conexoes_total = config.increment_connections();
				int conexoes_atual = config["conexoes_atual"].GetInt();
				int caronas_total = config["caronas_total"].GetInt();

				rapidjson::Document hello_packet;
				std::string login_message;
				rapidjson::Value login_value(rapidjson::kStringType);
				hello_packet.SetObject();
				login_message = "Carona Comunitária USP\n" +
					std::to_string(conexoes_total) + " clientes já conectados, " +
					std::to_string(conexoes_atual) + " atualmente, " +
					std::to_string(caronas_total) + " caronas dadas";
				login_value = rapidjson::StringRef(login_message);
				hello_packet.AddMember("login", login_value, hello_packet.GetAllocator());
				socket.send(hello_packet);

				if (socket.next()) {
					if (socket["usuario"] == "cadastro") {
						uint64_t code = generate_random_64();
						LOG(INFO) << "New user: code " << code;

					}
				}

				while (socket.next()) {

				}
				LOG(WARNING) << ip << "Invalid JSON";
			} catch (std::exception& e) {
				LOG(ERROR) << ip << "Quit: " << e.what();
			}
		}

	private:
		int fd;
		void save_ip() {
			char ip_buffer[50];
			struct sockaddr_in peer_address;
			socklen_t peer_address_length = sizeof(peer_address);

			if (getpeername(fd, (sockaddr *) &peer_address, &peer_address_length)) {
				ip = "Unknown (" + std::to_string(fd) + "): ";
				PLOG(ERROR) << ip << "getpeername";
			} else {
				if (inet_ntop(AF_INET, &peer_address.sin_addr, ip_buffer, sizeof(ip_buffer) - 1) == NULL) {
					ip = "Unknown (" + std::to_string(fd) + "): ";
					PLOG(ERROR) << ip << "inet_ntop";
				} else {
					ip = std::string(ip_buffer) + ": ";
					return;
				}
			}
		}

		uint64_t generate_random_64() {
			std::random_device dev("/dev/urandom");
			std::uniform_int_distribution<uint64_t> distribution;
			return distribution(dev);
		}
	};

	static void client_thread(int fd) {
		Client client(fd);
	}

	ClientThread::ClientThread(int fd) {
		std::thread thread(client_thread, fd);
		if (thread.joinable())
			thread.detach();
	}
}
