#include "Network.cc"

int main() {
	Network::SecureTCPServer server(41230);
	SecureSocket client = server.accept();
	return 0;
}
