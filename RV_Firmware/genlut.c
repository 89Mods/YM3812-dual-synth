#include <stdint.h>
#include <math.h>
#include <stdio.h>

void main(void) {
	for(int i = 0; i < 128; i++) {
		float f = pow(2, (i - 69.0) / 12.0) * 440.0;
		uint32_t val = (uint32_t)(f * 65536.0f);
		printf("0x%06x", val);
		if(((i + 1) & 3) == 0) printf(",\r\n");
		else printf(",");
	}
}
