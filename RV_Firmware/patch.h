#ifndef PATCH_H_
#define PATCH_H_

typedef struct {
	uint8_t tvenvfreq; //tremolo, vibrato, env type, env scale, frequency multiplier
	uint8_t level_scale; //level scaling
	uint8_t level; //max total level
	uint8_t atck_dec; //attack rate, decay rate
	uint8_t sus_rel; //sustain level, release rate
	uint8_t wave; //waveform
} op_patch;

typedef struct {
	op_patch ops[2];
	uint8_t feedback_algo;
	uint8_t fixed_note;
} patch;

#endif
