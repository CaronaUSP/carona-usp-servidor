#include <openssl/err.h>
#include <openssl/ssl.h>

void start_server();

int main() {
	SSL_load_error_strings();
	SSL_library_init();
	start_server();
	ERR_free_strings();
	return 0;
}
