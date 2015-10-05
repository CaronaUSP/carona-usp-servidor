#include <exception>
#include <iostream>

#include "JSON.hh"
#include "Network.hh"

int main() {
	try {
		Network::TCPServer server(14302);
		int fd;

		while ((fd = server.accept()) != -1) {
			std::cout << "Client OK\n";
			JSON::Socket socket(fd);
			std::cout << "Socket OK\n";

			while (socket.next())
				std::cout << "Parse OK\n";
			std::cout << "Parse ERR\n";
		}
	} catch (std::exception& e) {
		std::cerr << e.what() << '\n';
	}

	return 0;
}
