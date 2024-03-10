#ifndef SPIFLASH_H_
#define SPIFLASH_H_

#define ROMCS_PORT GPIO_port_C
#define ROMCS_PORT_NUM 3

uint8_t init_spi();
static inline void sel_rom()   { GPIO_digitalWrite(GPIOv_from_PORT_PIN(ROMCS_PORT, ROMCS_PORT_NUM), low); }
static inline void desel_rom() { GPIO_digitalWrite(GPIOv_from_PORT_PIN(ROMCS_PORT, ROMCS_PORT_NUM), high); }
void chip_erase();
void sector_erase(uint8_t which);
void rom_begin_write();
void rom_begin_read(uint32_t addr);
void rom_finish_read();
void rom_finish_write();
void rom_write_data(uint8_t* data, uint32_t len);

#endif
