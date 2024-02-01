#ifndef FIXEDPOINT_H_
#define FIXEDPOINT_H_

#define fixed32 int32_t

/**
 * @brief Fast multiply for ch32v003 (produces 64-bit result),
 * 			author: CNLohr
 * 			source: https://github.com/cnlohr/ch32v003fun/tree/master/examples/ws2812bdemo
 * 			modified by Tholin to produce 64-bit result
 */
static inline uint64_t FastMultiply(uint32_t a, uint32_t b) __attribute__((section(".data")));
static inline uint64_t FastMultiply(uint32_t a, uint32_t b) {
	uint64_t ret = 0;
	uint32_t multiplicand = a > b ? b : a;
	uint64_t mutliplicant = a > b ? a : b;
	do
	{
		if(multiplicand & 1)
			ret += mutliplicant;
		mutliplicant<<=1;
		multiplicand>>=1;
	}while(multiplicand);
	return ret;
}

static inline fixed32 f_mul(fixed32 a, fixed32 b) {
	uint8_t neg = 0;
	if(a < 0) {
		neg ^= 1;
		a = -a;
	}
	if(b < 0) {
		neg ^= 1;
		b = -b;
	}
	fixed32 res = (fixed32)(FastMultiply(a, b) >> 16UL);
	//fixed32 res = (fixed32)(((uint64_t)a * (uint64_t)b) >> 16UL);
	if(neg) {
		res = -res;
	}
	return res;
}

static inline fixed32 f_div(fixed32 a, fixed32 b) {
	uint8_t neg = 0;
	if(a < 0) {
		neg ^= 1;
		a = -a;
	}
	if(b < 0) {
		neg ^= 1;
		b = -b;
	}
	fixed32 res = (fixed32)(((uint64_t)a << 16UL) / (uint64_t)b);
	if(neg) {
		res = -res;
	}
	return res;
}

#endif
