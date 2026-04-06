#include "ch32fun.h"

#include <stdint.h>

#include "./ch32fun/extralibs/ch32v003_SPI.h"
#include "spiflash.h"

//Initializes SPI and resets the ROM
uint8_t init_spi() {
	RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
	funPinMode(ROMCS, GPIO_Speed_50MHz | GPIO_CNF_OUT_PP);
	desel_rom();
	SPI_init();
	
	Delay_Ms(1);
	sel_rom();
	SPI_begin_8();
	SPI_transfer_8(0xFF);
	SPI_end();
	desel_rom();
	Delay_Ms(1);
	sel_rom();
	SPI_begin_8();
	SPI_transfer_8(0xAB);
	SPI_end();
	desel_rom();
	Delay_Ms(1);
	sel_rom();
	SPI_begin_8();
	SPI_transfer_8(0x90);
	SPI_transfer_8(0);
	SPI_transfer_8(0);
	SPI_transfer_8(0);
	uint8_t chipID = SPI_transfer_8(0);
	SPI_end();
	desel_rom();
	return chipID;
}

void un_protect_rom() {
	sel_rom();
	SPI_begin_8();
	SPI_transfer_8(0x01);
	SPI_transfer_8(0);
	desel_rom();
	SPI_end();
	Delay_Us(1);
}

void disable_rom_wp() {
	uint8_t status = 0;
	for(uint8_t i = 0; i < 2; i++) {
		sel_rom();
		SPI_begin_8();
		SPI_transfer_8(0x06);
		desel_rom();
		SPI_end();
		Delay_Us(1);
		sel_rom();
		SPI_begin_8();
		SPI_transfer_8(0x05);
		status = SPI_transfer_8(0);
		desel_rom();
		SPI_end();
		if((status & 0b00001100) != 0 && i == 0) {
			un_protect_rom();
		}else break;
	}
	if((status & 0b00001110) != 0b00000010) while(1) funDigitalWrite(LED, FUN_HIGH); //TODO: Proper error handling
}

void chip_erase() {
	disable_rom_wp();
	sel_rom();
	SPI_begin_8();
	SPI_transfer_8(0xC7);
	desel_rom();
	SPI_end();
	for(uint8_t i = 0; i < 4; i++) {
		funDigitalWrite(LED, FUN_HIGH);
		Delay_Ms(400);
		funDigitalWrite(LED, FUN_LOW);
		Delay_Ms(100);
	}
}

void sector_erase(uint8_t which) {
	disable_rom_wp();
	sel_rom();
	SPI_begin_8();
	SPI_transfer_8(0x20);
	SPI_transfer_8(0);
	SPI_transfer_8(which << 4);
	SPI_transfer_8(0);
	desel_rom();
	SPI_end();
	Delay_Ms(120);
}

uint32_t curr_rom_addr;
void rom_begin_write(uint32_t address) {
	curr_rom_addr = address;
	disable_rom_wp();
	sel_rom();
	SPI_begin_8();
	SPI_transfer_8(0x02);
	SPI_transfer_8((curr_rom_addr >> 16) & 0xFF);
	SPI_transfer_8((curr_rom_addr >> 8) & 0xFF);
	SPI_transfer_8(curr_rom_addr & 0xFF);
}

void rom_write_data(uint8_t* data, uint32_t len) {
	for(uint32_t i = 0; i < len; i++) {
		SPI_transfer_8(data[i]);
		curr_rom_addr++;
		if((curr_rom_addr & 255) == 0) { //Respect page boundaries
			desel_rom();
			SPI_end();
			Delay_Ms(5);
			disable_rom_wp();
			sel_rom();
			SPI_begin_8();
			SPI_transfer_8(0x02);
			SPI_transfer_8((curr_rom_addr >> 16) & 0xFF);
			SPI_transfer_8((curr_rom_addr >> 8) & 0xFF);
			SPI_transfer_8(curr_rom_addr & 0xFF);
		}
	}
}

void rom_finish_write() {
	SPI_transfer_8(0);
	desel_rom();
	SPI_end();
	Delay_Ms(5);
}

void rom_begin_read(uint32_t addr) {
	sel_rom();
	SPI_begin_8();
	SPI_transfer_8(0x03);
	SPI_transfer_8((addr >> 16) & 0xFF);
	SPI_transfer_8((addr >> 8) & 0xFF);
	SPI_transfer_8(addr & 0xFF);
}

void rom_finish_read() {
	SPI_end();
	desel_rom();
}
