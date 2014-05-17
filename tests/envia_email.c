#include "mail.h"

int main() {
	char dest[255];
	int cod;
	curl = curl_easy_init();
	printf("Destinatário: ");
	scanf("%s", dest);
	printf("Código: ");
	scanf("%d", &cod);
	envia_email(dest, cod);
	return 0;
}
