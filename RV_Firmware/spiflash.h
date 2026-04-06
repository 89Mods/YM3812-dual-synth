#ifndef SPIFLASH_H_
#define SPIFLASH_H_

#define ROMCS PC3

uint8_t init_spi();
static inline void sel_rom()   { funDigitalWrite(ROMCS, FUN_LOW); }
static inline void desel_rom() { funDigitalWrite(ROMCS, FUN_HIGH); }
void chip_erase();
void sector_erase(uint8_t which);
void rom_begin_write(uint32_t address);
void rom_begin_read(uint32_t addr);
void rom_finish_read();
void rom_finish_write();
void rom_write_data(uint8_t* data, uint32_t len);

#endif
