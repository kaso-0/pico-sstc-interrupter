#define PICO_DEFAULT_LED_PIN 25

#include "pico/stdlib.h"
#include "tusb.h"
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define AUDIO_GPIO 13
#define MAX_VOICES 8
#define SAMPLE_RATE 11025

#define MIDI_NOTE_ON  0x90
#define MIDI_NOTE_OFF 0x80

uint8_t rp2040_chip_version(void) {
    return 1; // Stub
}

typedef enum { ATTACK, DECAY, SUSTAIN, RELEASE, OFF } adsr_phase_t;

typedef struct {
    float level;
    adsr_phase_t phase;
    uint32_t ticks;
} adsr_t;

typedef struct {
    bool active;
    uint8_t note;
    float freq;
    float phase;
    float increment;
    adsr_t adsr;
} voice_t;

voice_t voices[MAX_VOICES];

// ADSR parameters (in samples)
const uint32_t ATTACK_TIME = 300;
const uint32_t DECAY_TIME = 200;
const float SUSTAIN_LEVEL = 0.6f;
const uint32_t RELEASE_TIME = 500;

float midi_note_to_freq(uint8_t note) {
    return 440.0f * powf(2.0f, ((int)note - 69) / 12.0f);
}

void reset_adsr(adsr_t *adsr) {
    adsr->phase = ATTACK;
    adsr->ticks = 0;
    adsr->level = 0.0f;
}

void update_adsr(adsr_t *adsr) {
    switch (adsr->phase) {
        case ATTACK:
            adsr->level = (float)adsr->ticks / ATTACK_TIME;
            adsr->ticks++;
            if (adsr->ticks >= ATTACK_TIME) {
                adsr->phase = DECAY;
                adsr->ticks = 0;
            }
            break;
        case DECAY:
            adsr->level = 1.0f - (1.0f - SUSTAIN_LEVEL) * ((float)adsr->ticks / DECAY_TIME);
            adsr->ticks++;
            if (adsr->ticks >= DECAY_TIME) {
                adsr->phase = SUSTAIN;
            }
            break;
        case SUSTAIN:
            adsr->level = SUSTAIN_LEVEL;
            break;
        case RELEASE:
            adsr->level -= (adsr->level / RELEASE_TIME);
            adsr->ticks++;
            if (adsr->ticks >= RELEASE_TIME || adsr->level <= 0.001f) {
                adsr->phase = OFF;
                adsr->level = 0;
            }
            break;
        default:
            adsr->level = 0.0f;
            break;
    }
}

void voice_on(uint8_t note) {
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (!voices[i].active || voices[i].adsr.phase == OFF) {
            voices[i].active = true;
            voices[i].note = note;
            voices[i].freq = midi_note_to_freq(note);
            voices[i].increment = voices[i].freq / SAMPLE_RATE;
            voices[i].phase = 0;
            reset_adsr(&voices[i].adsr);
            return;
        }
    }
}

void voice_off(uint8_t note) {
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voices[i].active && voices[i].note == note) {
            voices[i].adsr.phase = RELEASE;
            voices[i].adsr.ticks = 0;
        }
    }
}

void stop_all_voices() {
    for (int i = 0; i < MAX_VOICES; ++i) {
        voices[i].adsr.phase = RELEASE;
        voices[i].adsr.ticks = 0;
    }
}

void handle_midi(uint8_t *msg) {
    uint8_t cmd = msg[1] & 0xF0;
    uint8_t note = msg[2];
    uint8_t velocity = msg[3];

    if (cmd == MIDI_NOTE_ON && velocity > 0) {
        voice_on(note);
    } else if (cmd == MIDI_NOTE_OFF || (cmd == MIDI_NOTE_ON && velocity == 0)) {
        voice_off(note);
    } else if (msg[1] == 0xFC || msg[1] == 0xFF) { // stop/all notes off
        stop_all_voices();
    }
}

int main() {
    stdio_init_all();
    tusb_init();
    gpio_init(AUDIO_GPIO);
    gpio_set_dir(AUDIO_GPIO, GPIO_OUT);
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    absolute_time_t next_sample = get_absolute_time();

    while (true) {
        tud_task();
        if (tud_midi_available()) {
            uint8_t msg[4];
            tud_midi_packet_read(msg);
            handle_midi(msg);
        }

        if (absolute_time_diff_us(get_absolute_time(), next_sample) <= 0) {
            float mix = 0.0f;
            bool any_on = false;
            for (int i = 0; i < MAX_VOICES; ++i) {
                if (voices[i].active && voices[i].adsr.phase != OFF) {
                    update_adsr(&voices[i].adsr);
                    voices[i].phase += voices[i].increment;
                    if (voices[i].phase >= 1.0f)
                        voices[i].phase -= 1.0f;
                    float val = (voices[i].phase < 0.5f ? 1.0f : -1.0f) * voices[i].adsr.level;
                    mix += val;
                    any_on = true;
                } else {
                    voices[i].active = false;
                }
            }

            gpio_put(PICO_DEFAULT_LED_PIN, any_on);
            gpio_put(AUDIO_GPIO, mix >= 0.0f);
            next_sample = delayed_by_us(next_sample, 1000000 / SAMPLE_RATE);
        }
    }
}
