#ifndef CARONA_USP_NETWORK_H_
#define CARONA_USP_NETWORK_H_

#include <openssl/ssl.h>

#include <cstdint>
#include <string>

namespace Network {
	class Socket {
	public:
		Socket(int fd);
		~Socket();
		char read_byte();
		void send(const std::string &data);

	private:
		int fd;
	};

	class SecureSocket {
	public:
		SecureSocket(SSL *ssl);
		~SecureSocket();
		char read_byte();
		void send(const std::string &data);

	private:
		SSL *ssl;
	};

	class TCPServer {
	public:
		TCPServer(uint16_t port);
		~TCPServer();
		int accept() const;

	private:
		int socket_fd;
	};

	class SecureTCPServer : public TCPServer {
	public:
		SecureTCPServer(uint16_t port);
		~SecureTCPServer();
		SecureSocket accept() const;

	private:
		SSL_METHOD *ssl_method;
    	SSL_CTX *ssl_ctx;
	};
}

#endif	// CARONA_USP_NETWORK_H_
