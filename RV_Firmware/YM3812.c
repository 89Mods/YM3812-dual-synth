#include "defines.h"

#include <stdint.h>

#include "patch.h"
#include "YM3812.h"

#include "ch32fun.h"
#include "ch32v003_GPIO_branchless.h"

const uint8_t channel_map[YM3812_NUM_CHANNELS] = {0,1,2,6,7,8,12,13,14};
const uint8_t op_map[YM3812_NUM_OPERATORS] = {0,1,2,3,4,5,8,9,10,11,12,13,16,17,18,19,20,21};

void ym3812_reset(ym3812_i *ym) {
	GPIO_port_enable(YM3812_RST_PORT);
	GPIO_port_enable(YM3812_A0_PORT);
	GPIO_port_enable(YM3812_FIRST_CS_PORT);
	GPIO_port_enable(YM3812_SECOND_CS_PORT);
	GPIO_port_enable(YM3812_WR_PORT);
	GPIO_port_enable(LS164_DATA_PORT);
	GPIO_port_enable(LS164_CLK_PORT);
	GPIO_pinMode(GPIOv_from_PORT_PIN(YM3812_RST_PORT, YM3812_RST_PORT_NUM), GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
	GPIO_pinMode(GPIOv_from_PORT_PIN(YM3812_A0_PORT, YM3812_A0_PORT_NUM), GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
	GPIO_pinMode(GPIOv_from_PORT_PIN(YM3812_FIRST_CS_PORT, YM3812_FIRST_CS_PORT_NUM), GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
	GPIO_pinMode(GPIOv_from_PORT_PIN(YM3812_SECOND_CS_PORT, YM3812_SECOND_CS_PORT_NUM), GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
	GPIO_pinMode(GPIOv_from_PORT_PIN(YM3812_WR_PORT, YM3812_WR_PORT_NUM), GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
	GPIO_pinMode(GPIOv_from_PORT_PIN(LS164_DATA_PORT, LS164_DATA_PORT_NUM), GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
	GPIO_pinMode(GPIOv_from_PORT_PIN(LS164_CLK_PORT, LS164_CLK_PORT_NUM), GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_FIRST_CS_PORT, YM3812_FIRST_CS_PORT_NUM), high);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_SECOND_CS_PORT, YM3812_SECOND_CS_PORT_NUM), high);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_A0_PORT, YM3812_A0_PORT_NUM), low);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_WR_PORT, YM3812_WR_PORT_NUM), high);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(LS164_DATA_PORT, LS164_DATA_PORT_NUM), low);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(LS164_CLK_PORT, LS164_CLK_PORT_NUM), low);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_RST_PORT, YM3812_RST_PORT_NUM), low);
	Delay_Ms(100);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_RST_PORT, YM3812_RST_PORT_NUM), high);
	
	for(uint8_t i = 0; i < YM3812_NUM_OPERATORS; i++) ym->reg_40[i] = 0;
	for(uint8_t i = 0; i < YM3812_NUM_CHANNELS; i++) ym->reg_B0[i] = 0;
	for(uint8_t i = 0; i < 254; i++) {
		ym3812_sendData(ym, i, 0);
	}
	Delay_Ms(2);
	for(uint8_t i = 0; i < 5; i++) ym3812_sendData(ym, 1, 1 << 5); //Take out of OPL 1 compatibility mode
}

void ym3812_sendData(ym3812_i *ym, uint8_t reg, uint8_t val) {
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_A0_PORT, YM3812_A0_PORT_NUM), low);
	if(ym->cs_port) GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_SECOND_CS_PORT, YM3812_SECOND_CS_PORT_NUM), low);
	else GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_FIRST_CS_PORT, YM3812_FIRST_CS_PORT_NUM), low);
	for(uint8_t i = 0; i < 8; i++) {
		if((reg & 128) != 0) GPIO_digitalWrite(GPIOv_from_PORT_PIN(LS164_DATA_PORT, LS164_DATA_PORT_NUM), high);
		else GPIO_digitalWrite(GPIOv_from_PORT_PIN(LS164_DATA_PORT, LS164_DATA_PORT_NUM), low);
		Delay_Us(0.05);
		GPIO_digitalWrite(GPIOv_from_PORT_PIN(LS164_CLK_PORT, LS164_CLK_PORT_NUM), high);
		Delay_Us(0.05);
		GPIO_digitalWrite(GPIOv_from_PORT_PIN(LS164_CLK_PORT, LS164_CLK_PORT_NUM), low);
		Delay_Us(0.05);
		reg <<= 1;
	}
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_WR_PORT, YM3812_WR_PORT_NUM), low);
	Delay_Us(0.2);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_WR_PORT, YM3812_WR_PORT_NUM), high);
	Delay_Us(0.2);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_A0_PORT, YM3812_A0_PORT_NUM), high);
	for(uint8_t i = 0; i < 8; i++) {
		if((val & 128) != 0) GPIO_digitalWrite(GPIOv_from_PORT_PIN(LS164_DATA_PORT, LS164_DATA_PORT_NUM), high);
		else GPIO_digitalWrite(GPIOv_from_PORT_PIN(LS164_DATA_PORT, LS164_DATA_PORT_NUM), low);
		Delay_Us(0.05);
		GPIO_digitalWrite(GPIOv_from_PORT_PIN(LS164_CLK_PORT, LS164_CLK_PORT_NUM), high);
		Delay_Us(0.05);
		GPIO_digitalWrite(GPIOv_from_PORT_PIN(LS164_CLK_PORT, LS164_CLK_PORT_NUM), low);
		Delay_Us(0.05);
		val <<= 1;
	}
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_WR_PORT, YM3812_WR_PORT_NUM), low);
	Delay_Us(0.2);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_WR_PORT, YM3812_WR_PORT_NUM), high);
	Delay_Us(0.2);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_A0_PORT, YM3812_A0_PORT_NUM), low);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_SECOND_CS_PORT, YM3812_SECOND_CS_PORT_NUM), high);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(YM3812_FIRST_CS_PORT, YM3812_FIRST_CS_PORT_NUM), high);
	Delay_Us(0.2);
}

void ym3812_keyOn(ym3812_i *ym, uint8_t ch, uint8_t val) {
	ym3812_sendData(ym, 0xB0 + ch, SET_BITS(ym->reg_B0[ch], 0b00000001, 5, val));
}

void ym3812_frqBlock(ym3812_i *ym, uint8_t ch, uint8_t val) {
	ym3812_sendData(ym, 0xB0 + ch, SET_BITS(ym->reg_B0[ch], 0b00000111, 2, val));
}

void ym3812_frqFnum(ym3812_i *ym, uint8_t ch, uint16_t frequency) {
	ym3812_sendData(ym, 0xA0 + ch, frequency & 0xFF);
	ym3812_sendData(ym, 0xB0 + ch, SET_BITS(ym->reg_B0[ch], 0b00000011, 0, frequency >> 8));
}

void ym3812_setLevel(ym3812_i *ym, uint8_t ch, uint8_t op, uint8_t newLevel) {
	uint8_t opIdx = channel_map[ch] + (op ? 3 : 0);
	uint8_t opAddr = op_map[opIdx];
	ym->reg_40[opIdx] &= 0b11000000;
	ym->reg_40[opIdx] |= newLevel & 0b00111111;
	ym3812_sendData(ym, 0x40 + opAddr, ym->reg_40[opIdx]);
}

void ym3812_loadPatch(ym3812_i *ym, uint8_t ch, const patch* p) {
	uint8_t op1Idx = channel_map[ch];
	uint8_t op2Idx = op1Idx + 3;
	uint8_t op1Addr = op_map[op1Idx];
	uint8_t op2Addr = op_map[op2Idx];
	ym3812_sendData(ym, 1, 1 << 5);
	ym3812_sendData(ym, 0xC0 + ch, p->feedback_algo);
	ym3812_sendData(ym, 0x20 + op1Addr, p->ops[0].tvenvfreq);
	ym3812_sendData(ym, 0x20 + op2Addr, p->ops[1].tvenvfreq);
	ym->reg_40[op1Idx] = (p->ops[0].level_scale << 6) | p->ops[0].level;
	ym->reg_40[op2Idx] = (p->ops[1].level_scale << 6) | p->ops[1].level;
	ym3812_sendData(ym, 0x40 + op1Addr, ym->reg_40[op1Idx]);
	ym3812_sendData(ym, 0x40 + op2Addr, ym->reg_40[op2Idx]);
	ym3812_sendData(ym, 0x60 + op1Addr, p->ops[0].atck_dec);
	ym3812_sendData(ym, 0x60 + op2Addr, p->ops[1].atck_dec);
	
	ym3812_sendData(ym, 0x80 + op1Addr, p->ops[0].sus_rel);
	ym3812_sendData(ym, 0x80 + op2Addr, p->ops[1].sus_rel);
	
	ym3812_sendData(ym, 0xE0 + op1Addr, p->ops[0].wave);
	ym3812_sendData(ym, 0xE0 + op2Addr, p->ops[1].wave);
}

//Begin 4MHz clock output to YM3812
//Accomplished by putting Timer 1 into PWM mode with a very low period, but fixing its duty cycle to 50%
void ym3812_init_clock() {
	// Enable TIM1
	RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
	// PD0 is T1CH1N, 10MHz Output alt func, push-pull
	GPIOD->CFGLR &= ~(0xf);
	GPIOD->CFGLR |= GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF;
	
	// Reset TIM1 to init all regs
	RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;
	
	// Prescaler 
	TIM1->PSC = 0x0000;
	
	// Auto Reload - sets period
	TIM1->ATRLR = 12;
	
	// Reload immediately
	TIM1->SWEVGR |= TIM_UG;
	
	// Enable CH1N output, positive pol
	TIM1->CCER |= TIM_CC1NE | TIM_CC1NP;
	
	// CH1 Mode is output, PWM1 (CC1S = 00, OC1M = 110)
	TIM1->CHCTLR1 |= TIM_OC1M_2 | TIM_OC1M_1;
	
	// Set the Capture Compare Register value to 50%
	TIM1->CH1CVR = 6;
	
	// Enable TIM1 outputs
	TIM1->BDTR |= TIM_MOE;
	
	// Enable TIM1
	TIM1->CTLR1 |= TIM_CEN;
}
