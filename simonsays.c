/* KTHW - Hardware Clone of Keep Talking and Nobody Explodes
Copyright (C) 2017 Toby P., Thomas H.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "simonsays.h"

#include "util.h"

/* Simon Says is an annoying little module that will blink color combinations
at you and expects you to reply by pressing colored buttons back at it. In the beginning,
the sequence is just one color, but when you complete it it just gets longer, for up to 6
colors. */

/* On+Off time for a single color */
#define PHASE_TICKS 75
/* On time for a single color */
#define PHASE_DUTY_TICKS 35
/* How many ticks you may wait between buttons in a sequence */
#define BUTTON_COUNTDOWN_TICKS 500

#define BTN_GREEN	0
#define BTN_BLUE	1
#define BTN_RED		2
#define BTN_YELLOW	3

#define LED_GREEN	0
#define LED_BLUE	1
#define LED_RED		2
#define LED_YELLOW	3


static inline uint8_t simonsays_expected(struct bomb * bomb, struct simonsays * simonsays) {
	static uint8_t expectations[2][3][4] = {
		//	LED_GREEN		LED_BLUE		LED_RED			LED_YELLOW
		{
			{BTN_YELLOW,	BTN_RED,		BTN_BLUE,		BTN_GREEN	},
			{BTN_BLUE,		BTN_GREEN,		BTN_YELLOW,		BTN_RED		},
			{BTN_YELLOW,	BTN_RED,		BTN_GREEN,		BTN_BLUE	},
		},
		{
			{BTN_GREEN,		BTN_YELLOW,		BTN_BLUE,		BTN_RED,	},
			{BTN_YELLOW,	BTN_BLUE,		BTN_RED,		BTN_GREEN,	},
			{BTN_BLUE,		BTN_GREEN,		BTN_YELLOW,		BTN_RED,	},
		}
	};
	return expectations[(bomb->flags & FL_SER_VOW) ? 0 : 1][bomb->strikes % 3][simonsays->seq[simonsays->expected_index]];
}

void simonsays_prepare_tick(struct bomb * bomb, struct module * module) {
	struct simonsays * simonsays = (struct simonsays *)module;

	simonsays->stage_count = rnd_range(4, 6);
	printf("[%s] stage=0/%d sequence=[", module->name, simonsays->stage_count);
	for (uint8_t i=0; i<simonsays->stage_count; ++i) {
		simonsays->seq[i] = rnd_range(0, 4);
		printf("%d,", simonsays->seq[i]);
	}
	printf("\b] expected=%d\n", simonsays_expected(bomb, simonsays));

	module->flags |= MF_READY;
}

void simonsays_tick(struct bomb * bomb, struct module * module) {
	struct simonsays * simonsays = (struct simonsays *)module;

	uint32_t poll_button = simonsays->ticks % 4;

	/* You don't get forever to enter a sequence. */
	if (simonsays->button_countdown != 0) {
		simonsays->button_countdown--;
		if (simonsays->button_countdown == 0) {
			strike(bomb, module);
		}
	}

	uint8_t button = gpio_get(simonsays->in_btn);
	if (simonsays->button_caches[poll_button] != button) {
		simonsays->button_caches[poll_button] = button;
		if (button) {
			uint8_t expected_button = simonsays_expected(bomb, simonsays);
			if (poll_button == expected_button) { //button was the expected one
				simonsays->expected_index++;
				if (simonsays->expected_index > simonsays->stage) {  //stage is done
					simonsays->stage++;
					simonsays->ticks = (simonsays->stage + 1) * PHASE_TICKS;
					simonsays->expected_index = 0;
					simonsays->button_countdown = 0;
					if (simonsays->stage == simonsays->stage_count) {
						module->flags |= MF_COMPLETE;
						return;
					}
				}
				else {
					simonsays->ticks = (simonsays->stage + 1) * PHASE_TICKS;
					simonsays->button_countdown = BUTTON_COUNTDOWN_TICKS;
				}
			}
			else {
				strike(bomb, module);
				simonsays->sr->value = 0;
				simonsays->stage = 0;
				simonsays->expected_index = 0;
				simonsays->button_countdown = 0;
			}
			printf("[%s] stage=%d/%d expected=%d\n", module->name, simonsays->stage, simonsays->stage_count, simonsays_expected(bomb, simonsays));
		}
	}

	simonsays->ticks = (simonsays->ticks + 1) % ((simonsays->stage + 3) * PHASE_TICKS);

	poll_button = simonsays->ticks % 4;
	uint8_t v = 1 << (poll_button + 4);

	if (simonsays->ticks < (simonsays->stage + 1) * PHASE_TICKS) {
		register uint32_t phase_index = simonsays->ticks / PHASE_TICKS;
		register uint32_t phase_progress = simonsays->ticks % PHASE_TICKS;
		v |= (phase_progress <= PHASE_DUTY_TICKS) ? (1 << simonsays->seq[phase_index]) : 0;
	}

	simonsays->sr->value = v;
}

void simonsays_reset(struct bomb * bomb, struct module * module) {
	struct simonsays * simonsays = (struct simonsays *)module;

	simonsays->sr->value = 0;
}