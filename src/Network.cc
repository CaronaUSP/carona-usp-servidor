#include "Network.hh"

#include <netdb.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <glog/logging.h>
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
			PLOG(ERROR) << "recv";
			throw std::runtime_error("recv");

		case 0:
			throw std::runtime_error("recv: Connection closed");

		default:
			return byte;
		}
	}

	char Socket::peek_byte() {
		/// @todo reading byte by byte isn't fast, but is easy and fits RapidJSON
		/// streams wonderfully
		char byte;
		switch (recv(fd, &byte, 1, MSG_PEEK)) {
		case -1:
			PLOG(ERROR) << "recv";
			throw std::runtime_error("recv");

		case 0:
			throw std::runtime_error("recv: Connection closed");

		default:
			return byte;
		}
	}

	void Socket::send(const std::string &data) {
		if (::send(fd, (void *) data.c_str(), data.size(), 0) == -1) {
			PLOG(ERROR) << "send";
			throw std::runtime_error("send");
		}
	}

	SecureSocket::SecureSocket(SSL *ssl) : ssl(ssl) {
		if (SSL_accept(ssl) != 1) {
	        ERR_print_errors_fp(stderr);
	        throw std::runtime_error("SSL_CTX_new");
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
		switch (SSL_read(ssl, &byte, 1)) {
		case -1:
			ERR_print_errors_fp(stderr);
			throw std::runtime_error("SSL_read");

		case 0:
			throw std::runtime_error("SSL_read: Connection closed");

		default:
			return byte;
		}
	}

	TCPServer::TCPServer(uint16_t port) {
		struct sockaddr_in serv_addr;

		if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			PLOG(ERROR) << "socket";
			throw std::runtime_error("socket");
		}

#ifdef SOCKET_SO_REUSEADDR
		static const int yes = 1;
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)))
			PLOG(ERROR) << "setsockopt";
#endif

		memset((char *) &serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);

		if (bind(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
			PLOG(ERROR) << "bind";
			throw std::runtime_error("bind");
		}

		if (listen(socket_fd, LISTEN_LIMIT) == -1) {
			PLOG(ERROR) << "listen";
			throw std::runtime_error("listen");
		}
	}

	TCPServer::~TCPServer() {
		close(socket_fd);
	}

	int TCPServer::accept() const {
		struct sockaddr_in client_address;
		int connection;
		socklen_t sockaddr_in_length = sizeof(client_address);
		connection = ::accept(socket_fd, (struct sockaddr *) &client_address, &sockaddr_in_length);
		if (connection == -1) {
			PLOG(ERROR) << "accept";
			return -1;
		}
		return connection;
	}

	SecureTCPServer::SecureTCPServer(uint16_t port) : TCPServer(port) {
		ssl_method = SSLv3_server_method();
	    ssl_ctx = SSL_CTX_new(ssl_method);
	    if (ssl_ctx == NULL) {
	        ERR_print_errors_fp(stderr);
	        throw std::runtime_error("SSL_CTX_new");
	    }
		if (SSL_CTX_use_certificate_file(ssl_ctx, CERTIFICATE_FILE, SSL_FILETYPE_PEM) != 1) {
			ERR_print_errors_fp(stderr);
	        throw std::runtime_error("SSL_CTX_use_certificate_file");
		}
		if (SSL_CTX_use_PrivateKey_file(ssl_ctx, CERTIFICATE_FILE, SSL_FILETYPE_PEM) != 1) {
			ERR_print_errors_fp(stderr);
	        throw std::runtime_error("SSL_CTX_use_certificate_file");
		}
		if (SSL_CTX_check_private_key(ssl_ctx) != 1) {
			ERR_print_errors_fp(stderr);
	        throw std::runtime_error("SSL_CTX_check_private_key");
		}
	}

	SecureTCPServer::~SecureTCPServer() {
		SSL_CTX_free(ssl_ctx);
	}

	SecureSocket SecureTCPServer::accept() const {
		int fd = TCPServer::accept();
		SSL *ssl;
		ssl = SSL_new(ssl_ctx);
		if (ssl == NULL) {
			ERR_print_errors_fp(stderr);
	        throw std::runtime_error("SSL_new");
		}
		if (SSL_set_fd(ssl, fd) != 1) {
			ERR_print_errors_fp(stderr);
	        throw std::runtime_error("SSL_set_fd");
		}
		return SecureSocket(ssl);
	}

// Writing a function for this is a bit hacky due to cURL definition of
// curl_easy_setopt (a macro with type checking depending on option), so I use a
// macro as well
#define CURL_SETOPT(option, parameter)	\
	do {					\
		if ((code = curl_easy_setopt(curl, option, parameter)) != CURLE_OK) {		\
			LOG(ERROR) << "curl_setopt(" << option << ", " << parameter << "): "	\
				<< code << " - " << curl_easy_strerror(code);						\
			throw std::runtime_error("curl_setopt failed");							\
		}								\
	} while (0)

	SendMail::SendMail(const std::string& to) {
		CURLcode code;
		if ((curl = curl_easy_init()) == NULL) {
			LOG(ERROR) << "curl_easy_init failed";
			throw std::runtime_error("curl_easy_init failed");
		}
		recipient_string = "<" + to + ">";
		CURL_SETOPT(CURLOPT_USERNAME, SMTP_USER);
		CURL_SETOPT(CURLOPT_PASSWORD, SMTP_PASSWORD);
		CURL_SETOPT(CURLOPT_URL, SMTP_ADDRESS);
		CURL_SETOPT(CURLOPT_READFUNCTION, read_callback);
		CURL_SETOPT(CURLOPT_READDATA, (void *) this);
		recipient = curl_slist_append(recipient, to.c_str());
		CURL_SETOPT(CURLOPT_MAIL_RCPT, recipient);
		CURL_SETOPT(CURLOPT_UPLOAD, 1L);
		CURL_SETOPT(CURLOPT_VERBOSE, 1L);
	}

	SendMail::~SendMail() {
		curl_slist_free_all(recipient);
		curl_easy_cleanup(curl);
	}

	size_t SendMail::read_callback(char *buffer, size_t size, size_t nitems, void *instream) {
		size_t next_block_size = size * nitems;
		SendMail *sendmail = (SendMail *) instream;

		if (next_block_size > sendmail->mail.size() - sendmail->processed)
			next_block_size = sendmail->mail.size() - sendmail->processed;
		memcpy(buffer, &(sendmail->mail[sendmail->processed]), next_block_size);
		sendmail->processed += next_block_size;
		return next_block_size;
	}

	void SendMail::send(std::string text) {
		// http://tools.ietf.org/html/rfc5322#section-3.6
		char rfc_date[60];
		struct tm local_time_buffer, *local_time;
		time_t current_time;

		current_time = time(NULL);
		local_time = localtime_r(&current_time, &local_time_buffer);
		if (local_time == NULL) {
			LOG(ERROR) << "localtime";
			throw std::runtime_error("localtime");
		}
		if (strftime(rfc_date, sizeof(rfc_date), "%a, %d %b %Y %T %z", local_time) == 0) {
			LOG(ERROR) << "strftime";
			throw std::runtime_error("strftime");
		}

		mail =
			"Date: " + std::string(rfc_date) + "\r\n"			\
			"To: " + recipient_string + "\r\n"					\
			"From: " SMTP_EMAIL " (Carona Comunitária USP)\r\n"	\
			"Reply-To: " SMTP_EMAIL "\r\n"						\
			"Subject: Confirmação de e-mail - Carona USP\r\n"	\
			"\r\n"												+
			text;
		CURLcode code;
		code = curl_easy_perform(curl);
		if (code != CURLE_OK) {
			LOG(ERROR) << "curl_easy_perform(): " << code << " - " << curl_easy_strerror(code);
			throw std::runtime_error("curl_easy_perform");
		}
	}
}
