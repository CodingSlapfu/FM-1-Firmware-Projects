#ifndef SERENITY_H
#define SERENITY_H
#include <stdint.h>
#include <stddef.h>
#define SR_RATE 44100
#define SR_TRACKS 8
#define SR_VOICES 16
#define SR_PATCHES 24
#define SR_SCENES 6
#define SR_FREQUENCIES 11
#define SR_DRUM_KITS 3
#define SR_BEAT_PRESETS 8
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {SR_BASS, SR_DRONE, SR_PIANO, SR_GUITAR, SR_FLUTE, SR_WOODWIND, SR_BELL, SR_CHOIR} sr_role;
typedef enum {SR_SINE, SR_TRI, SR_HOLLOW, SR_REED, SR_PLUCK, SR_CHOIR_WAVE} sr_wave;
typedef struct { const char *name; sr_wave wave; float attack, release, brightness, gain; } sr_patch;
typedef struct { const char *name; uint16_t bpm; uint8_t scale, energy, ambiance; } sr_scene;
typedef struct { const char *name; float hz; } sr_frequency;
typedef struct { const char *name; float kick, hand, shaker; } sr_drum_kit;
typedef struct { uint8_t patch, mute; float volume, pan, meter; } sr_track;
typedef struct { uint8_t active, track, gate; float tail, freq, phase, phase2, env, peak, filter, attack_coef, release_coef; uint32_t remain; } sr_voice;
typedef struct {
  sr_track tracks[SR_TRACKS]; sr_voice voices[SR_VOICES];
  uint32_t rng, frame_count, tick_phase, tick_length, sequence_step, phrase;
  uint8_t scene, frequency, tuning_432, running, bed_type;
  uint8_t drums_enabled, drum_density, drum_kit, binaural_enabled;
  float master, reverb_amount, delay_amount, bed_level, current_energy;
  float wind_lp, rain_lp, limiter, dc_x, dc_y, low_bass;
  float drum_level, drum_meter, kick_phase, kick_env, hand_phase, hand_env;
  float shaker_env, shaker_lp, hand_noise_lp;
  float binaural_carrier, binaural_rate, binaural_level, binaural_env;
  float binaural_phase_l, binaural_phase_r;
  float bass_phase,tone_phase,transport_env,tone_level,carrier_now,beat_now;
  float energy,variation,bed_phase,bird_phase,bird_env;
  uint8_t freeze;
  int16_t reverb[4][1789]; int16_t delay[16384];
  uint16_t rev_pos[4], delay_pos;
  int8_t melody[16];
} sr_engine;

extern const sr_patch sr_patches[SR_PATCHES];
extern const sr_scene sr_scenes[SR_SCENES];
extern const sr_frequency sr_frequencies[SR_FREQUENCIES];
extern const sr_drum_kit sr_drum_kits[SR_DRUM_KITS];
extern const float sr_binaural_presets[SR_BEAT_PRESETS];
void sr_init(sr_engine *e, uint32_t seed);
void sr_set_scene(sr_engine *e, uint8_t scene);
void sr_set_frequency(sr_engine *e, uint8_t frequency);
void sr_set_tuning_432(sr_engine *e, uint8_t enabled);
void sr_set_drums(sr_engine *e, uint8_t enabled);
void sr_set_drum_level(sr_engine *e, float level);
void sr_set_drum_density(sr_engine *e, uint8_t density);
void sr_set_drum_kit(sr_engine *e, uint8_t kit);
void sr_set_binaural(sr_engine *e, uint8_t enabled);
void sr_set_binaural_rate(sr_engine *e, float difference_hz);
void sr_set_binaural_carrier(sr_engine *e, float carrier_hz);
void sr_set_track_patch(sr_engine *e, uint8_t track, uint8_t patch);
void sr_start(sr_engine *e);
void sr_stop(sr_engine *e);
void sr_note_on(sr_engine *e, uint8_t track, uint8_t midi, uint8_t velocity, float beats);
void sr_render(sr_engine *e, int16_t *interleaved_stereo, size_t frames);
float sr_note_frequency(const sr_engine *e, int midi);
float sr_sacred_bass_frequency(const sr_engine *e);
#ifdef __cplusplus
}
#endif
#endif
