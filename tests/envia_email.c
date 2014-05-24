#include "mail.h"

int main() {
	char dest[255];
	int cod;
	if (inicializa_email() == -1)
		return 1;
	for (;;) {
		printf("Destinatário: ");
		scanf("%s", dest);
		printf("Código: ");
		scanf("%d", &cod);
		envia_email(dest, cod);
	}
	return 0;
}
