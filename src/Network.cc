#include "Network.hh"

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>

#define CERTIFICATE_FILE	"certificate.pem"

namespace Network {
	Socket::Socket(int fd) : fd(fd) {}

	Socket::~Socket() {
		close(fd);
	}

	char Socket::read_byte() {
		/// @todo reading byte by byte isn't fast, but is easy and fits RapidJSON
		/// streams wonderfully
		char byte;
		switch (recv(fd, &byte, 1, 0)) {
		case -1:
			perror("recv");
			throw runtime_error("recv");

		case 0:
			throw runtime_error("recv: Connection closed");

		default:
			return byte;
		}
	}

	void Socket::send(const std::string &data) {
		if (::send(fd, (void *) &data.c_str(), data.size(), 0) == -1) {
			perror("send");
			throw runtime_error("send");
		}
	}

	SecureSocket::SecureSocket(SSL *ssl) : ssl(ssl) {
		if (SSL_accept(ssl) != 1) {
	        ERR_print_errors_fp(stderr);
	        throw runtime_error("SSL_CTX_new");
		}
	}

	SecureSocket::~SecureSocket() {
		int fd = SSL_get_fd(ssl);
		SSL_free(ssl);
		close(fd);
	}

	char SecureSocket::read_byte() {
		/// @todo reading byte by byte isn't fast, but is easy and fits RapidJSON
		/// streams wonderfully
		char byte;
		switch (SSL_read(fd, &byte, 1)) {
		case -1:
			ERR_print_errors_fp(stderr);
			throw runtime_error("SSL_read");

		case 0:
			throw runtime_error("SSL_read: Connection closed");

		default:
			return byte;
		}
	}

	TCPServer::TCPServer(uint16_t port) {
		struct sockaddr_in serv_addr;

		if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("socket");
			throw runtime_error("socket");
		}

#ifdef SOCKET_SO_REUSEADDR
		static const int yes = 1;
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)))
			perror("setsockopt");
#endif

		memset((char *) &serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);

		if (bind(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
			perror("bind");
			throw runtime_error("bind");
		}

		if (listen(socket_fd, LISTEN_LIMIT) == -1) {
			perror("listen");
			throw runtime_error("listen");
		}
	}

	TCPServer::~TCPServer() {
		close(socket_fd);
	}

	int TCPServer::accept() const {
		struct sockaddr_in client_address;
		int connection;
		socklen_t sockaddr_in_length = sizeof(cliAddr);
		connection = accept(socket_fd, (struct sockaddr *) &client_address, &sockaddr_in_length);
		if (connection == -1) {
			perror("accept");
			return -1;
		}
		return connection;
	}

	SecureTCPServer::SecureTCPServer(uint16_t port) : TCPServer(port) {
		ssl_method = SSLv3_server_method();
	    ssl_ctx = SSL_CTX_new(ssl_method);
	    if (ctx == NULL) {
	        ERR_print_errors_fp(stderr);
	        throw runtime_error("SSL_CTX_new");
	    }
		if (SSL_CTX_use_certificate_file(ssl_ctx, CERTIFICATE_FILE, SSL_FILETYPE_PEM) != 1) {
			ERR_print_errors_fp(stderr);
	        throw runtime_error("SSL_CTX_use_certificate_file");
		}
		if (SSL_CTX_use_PrivateKey_file(ssl_ctx, CERTIFICATE_FILE, SSL_FILETYPE_PEM) != 1) {
			ERR_print_errors_fp(stderr);
	        throw runtime_error("SSL_CTX_use_certificate_file");
		}
		if (SSL_CTX_check_private_key(ssl_ctx) != 1) {
			ERR_print_errors_fp(stderr);
	        throw runtime_error("SSL_CTX_check_private_key");
		}
	}

	SecureTCPServer::~SecureTCPServer() {
		SSL_CTX_free(ssl_ctx);
	}

	SecureSocket SecureTCPServer::accept() const {
		int fd = TCPServer::accept();
		ssl = SSL_new(ssl_ctx);
		if (ssl == NULL) {
			ERR_print_errors_fp(stderr);
	        throw runtime_error("SSL_new");
		}
		if (SSL_set_fd(fd) != 1) {
			ERR_print_errors_fp(stderr);
	        throw runtime_error("SSL_set_fd");
		}
		return SecureSocket(ssl);
	}
}
