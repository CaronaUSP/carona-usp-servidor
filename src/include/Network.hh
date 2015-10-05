#ifndef CARONA_USP_NETWORK_H_
#define CARONA_USP_NETWORK_H_

#include <curl/curl.h>
#include <openssl/ssl.h>

#include <cstdint>
#include <string>

/// @todo Secure versions should be used. Needs some testing and certificates
/// set up. http://nelenkov.blogspot.com.br/2011/12/using-custom-certificate-trust-store-on.html
/// should help.
namespace Network {
	class Socket {
	public:
		Socket(int fd);
		~Socket();
		char read_byte();
		char peek_byte();
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
		const SSL_METHOD *ssl_method;
    	SSL_CTX *ssl_ctx;
	};

	class SendMail {
		// friend class SendMail;
	public:
		SendMail(const std::string& to);
		~SendMail();
		void send(std::string text);

	private:
		CURL *curl;
		struct curl_slist *recipient = NULL;
		std::string mail;
		std::string recipient_string;
		size_t processed = 0;

		static size_t read_callback(char *buffer, size_t size, size_t nitems, void *instream);
	};
}

#endif	// CARONA_USP_NETWORK_H_
