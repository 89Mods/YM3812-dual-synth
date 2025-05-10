#include "defines.h"

#include <stdint.h>

#include "ch32fun.h"
#include "ch32v003_GPIO_branchless.h"
#include "../ch32fun/extralibs/ch32v003_SPI.h"
#include "ch32v003_uart.h"
#include "spiflash.h"

#include "patch.h"
#include "YM3812.h"
#include "fixedpoint.h"

#define NO_LED
#define THORINAIR

#ifdef THORINAIR
const uint8_t accepted_channels[] = {14};
const uint8_t accepted_channels_first[] = {13};
const uint8_t accepted_channels_second[] = {14};
#else
const uint8_t accepted_channels[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
const uint8_t accepted_channels_first[] = {0, 1, 2, 3, 4, 5, 6, 7};
const uint8_t accepted_channels_second[] = {8, 9, 10, 11, 12, 13, 14, 15};
#endif

typedef struct {
	uint8_t block;
	uint16_t fnum;
} frequency_s;

frequency_s optimal_settings_for_freq(fixed32 frequency);
fixed32 key_frequency(uint8_t key, uint8_t bend);
void set_freq(ym3812_i* chip, uint8_t channel, fixed32 frequency);
void set_key(ym3812_i* chip, uint8_t midi_channel, uint8_t channel, uint8_t key);

void sound_test();
void load_patches_from_midi();

uint8_t all_ym_channel_notes[YM3812_NUM_CHANNELS*2]; //Mapping YM3812 channels -> MIDI note playing in it. >= 128 = OFF
uint8_t all_ym_channel_channels[YM3812_NUM_CHANNELS*2]; //Mapping YM3812 channels -> MIDI channels
uint32_t all_ym_channel_change_steps[YM3812_NUM_CHANNELS*2]; //Timesteps since last change to a channel’s state
uint8_t all_ym_reloads[YM3812_NUM_CHANNELS*2];
uint8_t all_to_turn_on[YM3812_NUM_CHANNELS*2];

//These are global
uint8_t stereo_mode = 0;
uint8_t independent_mode = 0;
uint8_t midi_channel_programs[16]; //Mapping MIDI channel -> MIDI program no.
patch midi_channel_patches[16];
uint8_t channel_pitch_bends[16];
uint8_t midi_channel_pans[16]; //Mapping MIDI channel -> stereo pan

typedef struct {
	uint8_t num_channels;
	uint8_t *ym_channel_notes;
	uint8_t *ym_channel_channels;
	uint32_t *ym_channel_change_steps;
	uint32_t ym_curr_timestep;
	uint8_t *to_turn_on;
	uint8_t chip_base;
	uint8_t *ym_reloads;
} synth_state;

static uint8_t wait_for_uart_data(uint8_t timeout);
void playNote(uint8_t channel, uint8_t note, uint8_t velocity, synth_state* state);
void releaseNote(uint8_t channel, uint8_t note, synth_state* state);
patch get_patch_for(uint8_t program, uint8_t drum);
void releaseAllNotes(synth_state* state);
uint8_t isAcceptedChannel(uint8_t channel);

ym3812_i chips[2];
synth_state mono_state;
synth_state stereo_state;
synth_state independent_states[2];

uint8_t isAcceptedChannel(uint8_t channel) {
	if(independent_mode) {
		for(uint8_t i = 0; i < sizeof(accepted_channels_first); i++) {
			if(channel == accepted_channels_first[i]) return 1;
		}
		for(uint8_t i = 0; i < sizeof(accepted_channels_second); i++) {
			if(channel == accepted_channels_second[i]) return 2;
		}
		return 0;
	}
	
	if(sizeof(accepted_channels) >= 16) return 1;
	for(uint8_t i = 0; i < sizeof(accepted_channels); i++) {
		if(channel == accepted_channels[i]) return 1;
	}
	return 0;
}

int main() {
	SystemInit();
	GPIO_port_enable(GPIO_port_D);
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_D, 6), GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), low);
	UART_init();
	Delay_Ms(500);
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), high);
	ym3812_init_clock();
	chips[0].cs_port = 0;
	chips[1].cs_port = 1;
	ym3812_reset(chips);
	ym3812_reset(chips + 1);
	
	{
		uint8_t chipID = init_spi();
		//spiflash did not respond correctly to identify command
		if(chipID != 0xEF && chipID != 0xC2) {
			while(1) {
				GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), high);
				Delay_Ms(500);
				GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), low);
				Delay_Ms(500);
			}
		}
		//Get current mode
		rom_begin_read(0x1100);
		for(int i = 0; i < 4; i++) all_ym_channel_channels[i] = SPI_transfer_8(0);
		rom_finish_read();
		if(all_ym_channel_channels[0] != 0x06 || all_ym_channel_channels[1] != 0x21) {
			stereo_mode = independent_mode = 0;
		}else {
			stereo_mode = all_ym_channel_channels[2];
			independent_mode = all_ym_channel_channels[3];
			if(independent_mode) stereo_mode = 0;
		}
	}
	
soft_reset:
	mono_state = (synth_state){.ym_reloads = all_ym_reloads, .chip_base = 0, .num_channels = YM3812_NUM_CHANNELS*2, .ym_channel_notes = all_ym_channel_notes, .ym_channel_channels = all_ym_channel_channels, .ym_channel_change_steps = all_ym_channel_change_steps, .ym_curr_timestep = 2, .to_turn_on = all_to_turn_on};
	stereo_state = (synth_state){.ym_reloads = all_ym_reloads, .chip_base = 0, .num_channels = YM3812_NUM_CHANNELS, .ym_channel_notes = all_ym_channel_notes, .ym_channel_channels = all_ym_channel_channels, .ym_channel_change_steps = all_ym_channel_change_steps, .ym_curr_timestep = 2, .to_turn_on = all_to_turn_on};
	independent_states[0] = (synth_state){.ym_reloads = all_ym_reloads, .chip_base = 0, .num_channels = YM3812_NUM_CHANNELS, .ym_channel_notes = all_ym_channel_notes, .ym_channel_channels = all_ym_channel_channels, .ym_channel_change_steps = all_ym_channel_change_steps, .ym_curr_timestep = 2, .to_turn_on = all_to_turn_on};
	independent_states[1] = (synth_state){.ym_reloads = all_ym_reloads + YM3812_NUM_CHANNELS, .chip_base = 1, .num_channels = YM3812_NUM_CHANNELS, .ym_channel_notes = all_ym_channel_notes + YM3812_NUM_CHANNELS, .ym_channel_channels = all_ym_channel_channels + YM3812_NUM_CHANNELS, .ym_channel_change_steps = all_ym_channel_change_steps + YM3812_NUM_CHANNELS, .ym_curr_timestep = 2, .to_turn_on = all_to_turn_on + YM3812_NUM_CHANNELS};
	
	{
		//Save current mode
		sector_erase(1);
		rom_begin_write(0x1100);
		all_ym_channel_channels[0] = 0x06;
		all_ym_channel_channels[1] = 0x21;
		all_ym_channel_channels[2] = stereo_mode;
		all_ym_channel_channels[3] = independent_mode;
		rom_write_data(all_ym_channel_channels, 4);
		rom_finish_write();
	}
	
	//TODO: use memset here
	for(uint8_t i = 0; i < YM3812_NUM_CHANNELS*2; i++) {
		all_ym_channel_notes[i] = 255;
		all_ym_channel_channels[i] = 0;
		all_ym_channel_change_steps[i] = 1;
		all_to_turn_on[i] = 0;
		all_ym_reloads[i] = 0;
	}
	midi_channel_patches[0] = get_patch_for(0, 0);
	for(uint8_t i = 0; i < 16; i++) {
		midi_channel_programs[i] = 0;
		if(i != 0) midi_channel_patches[i] = midi_channel_patches[0];
		channel_pitch_bends[i] = 0;
		midi_channel_pans[i] = 64;
	}
	for(uint8_t i = 0; i < YM3812_NUM_CHANNELS; i++) {
		ym3812_loadPatch(chips, i, midi_channel_patches);
		ym3812_loadPatch(chips + 1, i, midi_channel_patches);
	}
	sound_test();
	
	synth_state *s = stereo_mode ? &stereo_state : &mono_state;
	while(1) {
		GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), low);
		uint8_t next = 0;
		for(uint8_t i = 0; i < YM3812_NUM_CHANNELS*2; i++) {
			if(next == YM3812_NUM_CHANNELS) next = 0;
			if(all_to_turn_on[i]) ym3812_keyOn(chips + (i >= YM3812_NUM_CHANNELS), next, 1);
			all_to_turn_on[i] = 0;
			next++;
		}
		next = wait_for_uart_data(0);
		if((next & 128) == 0) continue;
		while(1) {
#ifndef NO_LED
			GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), high);
#endif
			uint8_t upper = next >> 4;
			uint8_t channel = next & 0b1111;
			if(upper == 0b1000) {
				//Note Off event
				uint8_t key = wait_for_uart_data(1);
				wait_for_uart_data(1); //velocity, ignored
				if(!independent_mode) {
					if(key != 255 && isAcceptedChannel(channel)) releaseNote(channel, key & 127, s);
				}else if(key != 255) {
					uint8_t idx = isAcceptedChannel(channel);
					if(idx == 1) releaseNote(channel, key & 127, independent_states);
					else if(idx == 2) releaseNote(channel, key & 127, independent_states + 1);
				}
			}else if(upper == 0b1001) {
				//Note On event
				uint8_t key = wait_for_uart_data(1);
				uint8_t velocity = wait_for_uart_data(1);
				if(!independent_mode) {
					if(key != 255 && velocity != 255 && isAcceptedChannel(channel)) {
						if(velocity == 0) releaseNote(channel, key & 127, s);
						else playNote(channel, key & 127, velocity & 127, s);
					}
				}else if(key != 255 && velocity != 255) {
					uint8_t idx = isAcceptedChannel(channel);
					if(idx == 1) {
						if(velocity == 0) releaseNote(channel, key & 127, independent_states);
						else playNote(channel, key & 127, velocity & 127, independent_states);
					}else if(idx == 2) {
						if(velocity == 0) releaseNote(channel, key & 127, independent_states + 1);
						else playNote(channel, key & 127, velocity & 127, independent_states + 1);
					}
				}
			}else if(upper == 0b1100) {
				//Program Change
				uint8_t program = wait_for_uart_data(1);
				if(program != 255) {
					if(!independent_mode) {
						if(isAcceptedChannel(channel)) {
							program &= 127;
							midi_channel_programs[channel] = program;
							midi_channel_patches[channel] = get_patch_for(program, 0);
							for(uint8_t i = 0; i < s->num_channels; i++) if(s->ym_channel_channels[i] == channel) s->ym_reloads[i] = 1;
						}
					}else {
						program &= 127;
						midi_channel_programs[channel] = program;
						midi_channel_patches[channel] = get_patch_for(program, 0);
						uint8_t idx = isAcceptedChannel(channel);
						if(idx == 1 || idx == 2) {
							synth_state *s2 = idx == 1 ? independent_states : independent_states + 1;
							for(uint8_t i = 0; i < s2->num_channels; i++) if(s2->ym_channel_channels[i] == channel) s2->ym_reloads[i] = 1;
						}
					}
				}
			}else if(upper == 0b1110) {
				//Pitch Bend Change
				wait_for_uart_data(1);
				wait_for_uart_data(1);
				if(!independent_mode) {
					if(isAcceptedChannel(channel)) {
						//TODO
					}
				}else {
					
				}
			}else if(upper == 0b1011) {
				//Channel Mode Message
				uint8_t c = wait_for_uart_data(1);
				uint8_t v = wait_for_uart_data(1);
				if(!independent_mode) {
					if(isAcceptedChannel(channel)) {
						if((c == 120 && v == 0) || (c == 123 && v == 0)) {
							//All off!
							releaseAllNotes(s);
						}
						if((c == 8 || c == 10) && stereo_mode) {
							//Stereo pan
							midi_channel_pans[channel] = v & 0b0111111;
						}
					}
				}else {
					uint8_t idx = isAcceptedChannel(channel);
					if(idx != 0) {
						if((c == 120 && v == 0) || (c == 123 && v == 0)) {
							releaseAllNotes(idx == 1 ? independent_states : independent_states + 1);
						}
					}
				}
			}else if(upper == 0b1010 || next == 0b11110010) {
				//Unsupported two-argument events
				wait_for_uart_data(1);
				wait_for_uart_data(1);
			}else if(upper == 0b1101 || next == 0b11110001 || next == 0b11110011) {
				//Unsupported single-argument events
				wait_for_uart_data(1);
			}else if(next == 0b11110000) {
				//System Exclusive
				uint8_t mid = wait_for_uart_data(1);
				if(mid == 0b01111110) {
					uint8_t id1 = wait_for_uart_data(1);
					uint8_t id2 = wait_for_uart_data(1);
					//New patches
					if(id1 == 0b01010110 && id2 == 0b01111100) {
						releaseAllNotes(s);
						load_patches_from_midi();
						goto soft_reset;
					}
					while(mid != 0b11110111) mid = wait_for_uart_data(1);
					//Sector ERASE
					if(id1 == 0b01010110 && id2 == 0b01110011) sector_erase(0);
					//Chip ERASE
					if(id1 == 0b01010110 && id2 == 0b01111011) chip_erase();
					//Switch to MONO
					if(id1 == 0b01010110 && id2 == 0b01110010) {
						stereo_mode = 0;
						goto soft_reset;
					}
					//Switch to STEREO
					if(id1 == 0b01010110 && id2 == 0b01110110) {
						stereo_mode = 1;
						goto soft_reset;
					}
					//Switch to INDEPENDENT
					if(id1 == 0b01010110 && id2 == 0b01110101) {
						stereo_mode = 0;
						independent_mode = 1;
						goto soft_reset;
					}
				}
			}else if(next == 0b11111111) {
				//Full reset
				releaseAllNotes(s);
				ym3812_reset(chips);
				ym3812_reset(chips + 1);
				goto soft_reset;
			}
			uint16_t a = UART_getc();
			if(a == UART_NO_DATA) break;
			next = a & 0xFF;
		}
	}
}

static uint8_t wait_for_uart_data(uint8_t timeout) {
	uint16_t in = UART_NO_DATA;
	uint32_t counter = 0;
	while(in == UART_NO_DATA) {
		in = UART_getc();
		counter++;
		if(timeout) {
			if(counter == 750000) Delay_Us(1);
			if(counter == 2000000) return 0xFF;
		}
	}
	in &= 0xFF;
	return in;
}

void releaseAllNotes(synth_state* state) {
	for(uint8_t i = 0; i < state->num_channels; i++) state->ym_reloads[i] = 1;
	for(uint8_t i = 0; i < state->num_channels; i++) {
		if((state->ym_channel_notes[i] & 128) == 0) releaseNote(state->ym_channel_channels[i], state->ym_channel_notes[i], state);
	}
}

void sound_test() {
	uint8_t both = stereo_mode || independent_mode;
	both++;
	for(uint8_t i = 0; i < both; i++) {
		ym3812_frqBlock(chips + i, 0, 4);
		ym3812_frqBlock(chips + i, 1, 4);
		ym3812_frqBlock(chips + i, 2, 4);
		ym3812_frqBlock(chips + i, 3, 4);
		ym3812_frqFnum(chips + i, 0, 0x1C9);
		ym3812_frqFnum(chips + i, 1, 0x240);
		ym3812_frqFnum(chips + i, 2, 0x2AD);
		ym3812_frqFnum(chips + i, 3, 0x360);
	}
	
	ym3812_keyOn(chips, 0, 1);
	if(stereo_mode) ym3812_keyOn(chips + 1, 0, 1);
	Delay_Ms(100);
	ym3812_keyOn(chips, 1, 1);
	if(stereo_mode) ym3812_keyOn(chips + 1, 1, 1);
	Delay_Ms(100);
	ym3812_keyOn(chips, 2, 1);
	if(stereo_mode) ym3812_keyOn(chips + 1, 2, 1);
	Delay_Ms(100);
	ym3812_keyOn(chips, 3, 1);
	if(stereo_mode) ym3812_keyOn(chips + 1, 3, 1);
	Delay_Ms(1000);
	for(uint8_t i = 0; i < both; i++) {
		ym3812_keyOn(chips + i, 0, 0);
		ym3812_keyOn(chips + i, 1, 0);
		ym3812_keyOn(chips + i, 2, 0);
		ym3812_keyOn(chips + i, 3, 0);
	}
	Delay_Ms(2500);
}

void playNote(uint8_t channel, uint8_t note, uint8_t velocity, synth_state* state) {
	if(note > 127) return;
	if(velocity > 127) return;
	if(channel > 15) return;
	if(channel == 9 && (note < 35 || note >= 35+47)) return;
	
	//Find the oldest inactive channel
	uint32_t min = 0xFFFFFFFFUL;
	uint32_t argmin = 1000;
	ym3812_i *chip;
	for(uint8_t i = 0; i < state->num_channels; i++) {
		if((state->ym_channel_notes[i] & 128) != 0 && state->ym_channel_change_steps[i] < min) {
			min = state->ym_channel_change_steps[i];
			argmin = i;
		}
	}
	uint8_t localChannel = (argmin >= YM3812_NUM_CHANNELS) ? argmin - YM3812_NUM_CHANNELS : argmin;
	
	if(argmin == 1000) {
		//None found, just replace oldest note instead
		min = 0xFFFFFFFFUL;
		argmin = 1000;
		for(uint8_t i = 0; i < state->num_channels; i++) {
			if(state->ym_channel_change_steps[i] < min) {
				min = state->ym_channel_change_steps[i];
				argmin = i;
			}
		}
		localChannel = (argmin >= YM3812_NUM_CHANNELS) ? argmin - YM3812_NUM_CHANNELS : argmin;
		ym3812_keyOn(chips + (argmin >= YM3812_NUM_CHANNELS), localChannel, 0);
		if(stereo_mode) ym3812_keyOn(chips + 1, localChannel, 0);
		state->ym_curr_timestep++;
	}
	chip = chips + (argmin >= YM3812_NUM_CHANNELS) + state->chip_base;
	
	if(velocity > 100) velocity = 100;
	patch p = midi_channel_patches[channel];
	//original_level = 64 - p.ops[0].level
	fixed32 original_level = (fixed32)((uint32_t)(64 - p.ops[0].level) << 16);
	//scaled_level = original_level * (velocity / 100)
	fixed32 scaled_level = f_mul(original_level, f_div((fixed32)velocity << 16, 6553600 /* 100.0 in fixed-point */));
	uint8_t scaled_level_int = (uint8_t)(scaled_level >> 16);
	uint8_t new_level_l,new_level_r;
	if(stereo_mode) {
		uint8_t pan = midi_channel_pans[channel];
		if(pan == 64) new_level_l = new_level_r = scaled_level_int;
		else if(pan > 64) {
			//To the right
			new_level_r = scaled_level_int;
			//scaled_level *= 63 / (63 - (pan - 64))
			scaled_level = f_mul(scaled_level, (fixed32)(63 - (pan - 64)) << 10);
			new_level_l = (uint8_t)(scaled_level >> 16);
		}else {
			//To the left
			new_level_l = scaled_level_int;
			//scaled_level *= 63 / pan
			scaled_level = f_mul(scaled_level, (fixed32)pan << 10);
			new_level_r = (uint8_t)(scaled_level >> 16);
		}
	}else new_level_l = new_level_r = scaled_level_int;
	if(new_level_l == 0 && stereo_mode == 0) return;
	
	//Play note at channel index
	state->ym_channel_change_steps[argmin] = state->ym_curr_timestep;
	state->ym_curr_timestep++;
	
	uint8_t fixed_note = 0;
	if(channel == 9) {
		patch used_patch = get_patch_for(note - 35, 1);
		fixed_note = used_patch.fixed_note;
		ym3812_loadPatch(chip, localChannel, &used_patch);
		if(stereo_mode) ym3812_loadPatch(chip + 1, localChannel, &used_patch);
		state->ym_reloads[argmin] = 1;
	}else if(state->ym_channel_channels[argmin] != channel || state->ym_reloads[argmin]) { //Need to load patch
		fixed_note = p.fixed_note;
		ym3812_loadPatch(chip, localChannel, &p);
		if(stereo_mode) ym3812_loadPatch(chip + 1, localChannel, &p);
		state->ym_reloads[argmin] = 0;
	}
	
	set_key(chip, channel, localChannel, fixed_note != 0 ? fixed_note : note);
	if(stereo_mode) set_key(chip + 1, channel, localChannel, fixed_note != 0 ? fixed_note : note);
	state->to_turn_on[argmin] = new_level_l != 0;
	if(stereo_mode) state->to_turn_on[argmin + YM3812_NUM_CHANNELS] = new_level_r != 0;
	state->ym_channel_notes[argmin] = note;
	state->ym_channel_channels[argmin] = channel;
	
	ym3812_setLevel(chip, localChannel, 0, 64 - new_level_l);
	if(stereo_mode) ym3812_setLevel(chip + 1, localChannel, 0, 64 - new_level_r);
}

void releaseNote(uint8_t channel, uint8_t note, synth_state* state) {
	note &= 0x7F;
	channel &= 0x0F;
	
	for(uint8_t i = 0; i < state->num_channels; i++) {
		uint8_t ym_midi_channel = state->ym_channel_channels[i];
		if(ym_midi_channel != channel) continue;
		if(state->ym_channel_notes[i] != note) continue;
		
		ym3812_keyOn(chips + (i >= YM3812_NUM_CHANNELS) + state->chip_base, i >= YM3812_NUM_CHANNELS ? i - YM3812_NUM_CHANNELS : i, 0);
		state->ym_channel_change_steps[i] = state->ym_curr_timestep;
		state->ym_curr_timestep++;
		
		state->ym_channel_notes[i] = 255;
	}
}

patch get_patch_for(uint8_t program, uint8_t drum) {
	uint32_t address = (uint32_t)program * 14 + (drum ? 128 * 14 : 0);
	rom_begin_read(address);
	
	patch p;
	p.ops[0].tvenvfreq   = SPI_transfer_8(0);
	p.ops[0].level_scale = SPI_transfer_8(0);
	p.ops[0].level       = SPI_transfer_8(0);
	p.ops[0].atck_dec    = SPI_transfer_8(0);
	p.ops[0].sus_rel     = SPI_transfer_8(0);
	p.ops[0].wave        = SPI_transfer_8(0);
	p.ops[1].tvenvfreq   = SPI_transfer_8(0);
	p.ops[1].level_scale = SPI_transfer_8(0);
	p.ops[1].level       = SPI_transfer_8(0);
	p.ops[1].atck_dec    = SPI_transfer_8(0);
	p.ops[1].sus_rel     = SPI_transfer_8(0);
	p.ops[1].wave        = SPI_transfer_8(0);
	p.feedback_algo      = SPI_transfer_8(0);
	p.fixed_note         = SPI_transfer_8(0);
	
	rom_finish_read();
	return p;
}

frequency_s optimal_settings_for_freq(fixed32 frequency) {
	for(uint8_t block = 0; block < 8; block++) {
		fixed32 Ff = f_mul((fixed32)frequency, 668622 /* 10.202361081 in fixed-point */);
		if(block == 0) Ff <<= 1;
		else Ff >>= block - 1;
		uint32_t Ffi = (uint32_t)(Ff >> 16);
		if(Ffi >= 1023) continue;
		frequency_s res;
		res.block = block;
		res.fnum = (uint16_t)Ffi;
		return res;
	}
	frequency_s res;
	res.block = 0;
	res.fnum = 0;
	return res;
}

void set_freq(ym3812_i* chip, uint8_t channel, fixed32 frequency) {
	frequency_s setting = optimal_settings_for_freq(frequency);
	if(setting.block == 0 && setting.fnum == 0) return;
	ym3812_frqBlock(chip, channel, setting.block);
	ym3812_frqFnum(chip, channel, setting.fnum);
}

inline void set_key(ym3812_i* chip, uint8_t midi_channel, uint8_t channel, uint8_t key) {
	set_freq(chip, channel, key_frequency(key, channel_pitch_bends[midi_channel]));
}

const fixed32 key_frequencies[128] = { //Calculated by genlut.c via "2**((key - 69) / 12) * 440"
	0x082d01,0x08a976,0x092d51,0x09b904,
	0x0a4d05,0x0ae9d3,0x0b8ff4,0x0c3ff6,
	0x0cfa70,0x0dc000,0x0e914f,0x0f6f11,
	0x105a02,0x1152ec,0x125aa2,0x137208,
	0x149a0a,0x15d3a6,0x171fe9,0x187fed,
	0x19f4e0,0x1b8000,0x1d229e,0x1ede22,
	0x20b404,0x22a5d8,0x24b545,0x26e410,
	0x293415,0x2ba74d,0x2e3fd2,0x30ffda,
	0x33e9c0,0x370000,0x3a453d,0x3dbc44,
	0x416809,0x454bb0,0x496a8b,0x4dc820,
	0x52682a,0x574e9b,0x5c7fa4,0x61ffb5,
	0x67d380,0x6e0000,0x748a7b,0x7b7888,
	0x82d013,0x8a9760,0x92d517,0x9b9041,
	0xa4d054,0xae9d37,0xb8ff49,0xc3ff6a,
	0xcfa700,0xdc0000,0xe914f6,0xf6f110,
	0x105a026,0x1152ec0,0x125aa2e,0x1372082,
	0x149a0a8,0x15d3a6e,0x171fe92,0x187fed4,
	0x19f4e00,0x1b80000,0x1d229ec,0x1ede220,
	0x20b404c,0x22a5d80,0x24b545c,0x26e4104,
	0x2934150,0x2ba74dc,0x2e3fd24,0x30ffda8,
	0x33e9c00,0x3700000,0x3a453d8,0x3dbc440,
	0x4168098,0x454bb00,0x496a8b8,0x4dc8208,
	0x52682a0,0x574e9b8,0x5c7fa48,0x61ffb50,
	0x67d3800,0x6e00000,0x748a7b0,0x7b78880,
	0x82d0130,0x8a97600,0x92d5170,0x9b90410,
	0xa4d0540,0xae9d370,0xb8ff490,0xc3ff6a0,
	0xcfa7000,0xdc00000,0xe914f60,0xf6f1100,
	0x105a0260,0x1152ec00,0x125aa2e0,0x13720820,
	0x149a0a80,0x15d3a6e0,0x171fe920,0x187fed40,
	0x19f4e000,0x1b800000,0x1d229ec0,0x1ede2200,
	0x20b404c0,0x22a5d800,0x24b545c0,0x26e41040,
	0x29341500,0x2ba74dc0,0x2e3fd240,0x30ffda80
};

fixed32 key_frequency(uint8_t key, uint8_t bend) {
	key &= 127;
	return key_frequencies[key];
}

void load_patches_from_midi() {
	GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), low);
	uint8_t patchData[14];
	uint8_t x;
	rom_begin_write(0);
	for(uint16_t i = 0; i < 176; i++) { //128 melodic + 47 drums + 1 dummy
		for(uint8_t j = 0; j < 14; j++) {
			x = wait_for_uart_data(1);
			if(x == 0b11110111 || x == 255) goto ERROR;
			patchData[j] = x;
			if(!(j == 5 || j == 11 || j == 12 || j == 13)) { //These can be expressed in just 7 bits
				x = wait_for_uart_data(1);
				if(x == 0b11110111 || x == 255) goto ERROR;
				patchData[j] |= x << 7;
			}
		}
		rom_write_data(patchData, 14);
		if(((i + 1) & 3) == 0 && i != 175) {
			if(wait_for_uart_data(1) != 0b11110111) goto ERROR;
			if(wait_for_uart_data(1) != 0b11110000) goto ERROR;
			for(uint8_t k = 0; k < 3; k++) if(wait_for_uart_data(1) != 0) goto ERROR;
		}
	}
	rom_finish_write();
	for(uint8_t i = 0; i < 6; i++) {
		GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), high);
		Delay_Ms(50);
		GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), low);
		Delay_Ms(250);
	}
	return;
ERROR:
	for(uint8_t i = 0; i < 16; i++) {
		GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), high);
		Delay_Ms(250);
		GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), low);
		Delay_Ms(250);
	}
}
