#ifndef YM3812_H_
#define YM3812_H_

#define YM3812_NUM_CHANNELS 9
#define YM3812_NUM_OPERATORS 18
#define YM3812_FREQ 4000000

#define YM3812_RST PC2
#define YM3812_A0 PD4
#define YM3812_WR PC1
#define LS164_DATA PC0
#define LS164_CLK PD2
#define YM3812_FIRST_CS PD3
#define YM3812_SECOND_CS PA1

typedef struct {
    uint8_t cs_port;
    uint8_t reg_B0[YM3812_NUM_CHANNELS];
    uint8_t reg_40[YM3812_NUM_OPERATORS];
} ym3812_i;

#define SET_BITS( regVal, mask, offset, newVal) ((regVal) = ((regVal) & (~((mask)<<(offset)))) | (((newVal) & (mask)) << (offset)))
#define GET_BITS( regVal, mask, shift ) ( ((regVal) & (mask)) >> (shift) )

void ym3812_reset(ym3812_i *ym);
void ym3812_sendData(ym3812_i *ym, uint8_t reg, uint8_t val);

void ym3812_keyOn(ym3812_i *ym, uint8_t ch, uint8_t val);
void ym3812_frqBlock(ym3812_i *ym, uint8_t ch, uint8_t val);
void ym3812_frqFnum(ym3812_i *ym, uint8_t ch, uint16_t frequency);
void ym3812_setLevel(ym3812_i *ym, uint8_t ch, uint8_t op, uint8_t newLevel);
void ym3812_loadPatch(ym3812_i *ym, uint8_t ch, const patch* p);
void ym3812_init_clock();

#endif
