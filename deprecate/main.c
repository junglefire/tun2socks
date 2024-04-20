#include <stdlib.h>
#include <stdio.h>

int main() {
	FILE* f = fopen("my.pcap", "rb");
	int i = 0;
	fread(&i, sizeof(int), 1, f);
	printf("%d", i);
	fseek(f, i, SEEK_CUR);
	fread(&i, sizeof(int), 1, f);
	printf("%d", i);
	return 0;
}