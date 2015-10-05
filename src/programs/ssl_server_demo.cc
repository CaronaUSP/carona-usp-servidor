#include <openssl/err.h>

#include "Network.hh"

int main() {
	// This needs more work
	SSL_load_error_strings();
	SSL_library_init();
	Network::SecureTCPServer server(41230);
	Network::SecureSocket client = server.accept();
	ERR_free_strings();
	return 0;
}
