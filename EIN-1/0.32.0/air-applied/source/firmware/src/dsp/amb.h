/* SPDX-License-Identifier: GPL-3.0-only
 * ATTRACTOR-1 Galaxy Engine, original composition/UI architecture, Created by Ziao. */
#pragma once
#include <stdint.h>
#include "../ein_features.h"
extern float en_sample_rate;
#define EN_SR en_sample_rate
#define EN_PATCHES 130
#include "user_samples.h"
#define EN_KITS 27
#define EN_NPL 8
#define EN_NGAL 7
#define EN_DRUM_TRACK 7
#define EN_NPIECES 10
#define EN_NFREQ 13
#define EN_NBUS 6
#define EN_STEPS 64
/* Parameter storage is compact enough for ten complete recipes in one flash object. */
enum {P_ANCHOR,P_TEMPO,P_EVOLVE,P_DENSITY,P_REVERB,P_ECHO,P_WIDTH,P_MOTION,P_TUNING,P_AUTO,P_BARS,P_SCENE,P_HUMAN,P_FOCUS,P_THEME,P_OUTPUT,P_MASK=16,P_LAYER=24,P_REST=152,P_SLEEP_TIMER=153,P_TEXT=154,P_DRUMS=155,P_BASS=156,P_KIT=157,P_DRUM_ROOM=158,P_REFERENCE=159,P_DRUM_LEVEL=160,P_DRUM_HITS,P_DRUM_ECHO,P_DRUM_DAMP,P_DRUM_AUTO,P_POWER,P_CHORD,P_STRUM,P_AUTO_LEVEL,P_AUTO_DUCK,P_FPS,P_STRUM_MODE,P_STRUM_DIR,P_STRUM_FEEL,P_STRUM_DYN,P_MOTIF_VARIATION,P_ROOM_DECAY,P_ROOM_PREDELAY,P_ROOM_DAMP,P_ROOM_MOD,P_ECHO_FEEDBACK,P_ECHO_GRID,P_QUANT_MODE,P_QUANT_GRID,P_QUANT_STRENGTH,P_QUANT_SWING,P_CHORD_PLAYBACK,P_CONTEXT_AUTO,P_SCAPE_MODE,P_SCAPE_BANK,P_SCAPE_LEVEL,P_SCAPE_DENSITY,P_NPARAMS=192};
enum {L_LEVEL,L_VOICE,L_DENSITY,L_STEPS,L_RATE,L_GATE,L_PAN,L_REVERB,L_ECHO,L_MUTE,L_SOLO,L_VARIANT,L_ATTACK,L_RELEASE,L_TONE,L_TUNE};
#define LP(i,p) (P_LAYER+16*(i)+(p))
#define P_BASS_STRUM LP(7,L_TUNE) /* unused percussion register; project-compatible */
/* Unused melodic controls of the separate drum lane; no save-size growth. */
#define P_GLIDE LP(7,L_VOICE)
#define P_VIBRATO LP(7,L_VARIANT)
#define P_VIB_RATE LP(7,L_TONE)
#define P_VIB_DELAY LP(7,L_ATTACK)
#define P_MEMORY LP(7,L_STEPS)
enum {EN_MOTIF_OFF,EN_MOTIF_REMEMBER,EN_MOTIF_LEARN,EN_MOTIF_EVOLVE,EN_MOTIF_CONVERSE,EN_MOTIF_VARIATION};
#define P_STYLE P_REST
enum {EN_AMBIENCE=0,EN_NIGHTFALL=1,EN_CINEMATIC=2,EN_NEOCLASSICAL=3};
extern volatile uint32_t en_core_job_us[2],en_core_job_peak_us[2];
extern volatile int8_t en_soundscape_active;
extern volatile uint8_t en_cinematic_stage;
extern volatile float en_cinematic_energy;
#define P_PULL P_MOTION
#define P_TUNE P_ANCHOR
#define P_ROOT P_ANCHOR
extern const char *const GALAXY_NAME[8],*const VOICE_NAME[EN_PATCHES],*const SCENE_NAME[8],*const INTENT_NAME[10];
extern const float HEALING_HZ[EN_NFREQ];
typedef struct {const char *name;int16_t lo,hi,def;} en_param_t;
const en_param_t *en_param_info(int p);
void en_param_text(int p,int v,char *b);
extern volatile uint8_t en_patch_locks;
extern const char *const REFERENCE_NAME[10];
int en_reference(void);
const char *en_scale_name(int mode);unsigned en_scale_mask(int mode);
void en_arrange(int16_t *p,int intention,int reference);
void en_piece(int i,int16_t *p,uint16_t *pool);
const char *en_piece_name(int i);
extern volatile uint32_t en_remote_serial;unsigned en_remote_revision(unsigned p);int en_remote_value(unsigned p);
extern volatile uint8_t en_output_route;void en_output_detect(int external);int en_output_auto(void);
void en_external_drop(unsigned source);
void en_init(void);void en_set(int p,int v);void en_seed(uint32_t s);void en_regrow(void);
void en_play(int on);void en_freeze(int on);void en_touch(void);void en_panic(void);void en_mute(int on);void en_pool(uint32_t p);void en_strike(int layer,int key);void en_key_release(int key);
void en_render(int32_t *out,uint32_t n,uint32_t gain);
void en_midi_out(uint32_t st,uint32_t a,uint32_t b);
enum {EN_CLK_TICK,EN_CLK_START,EN_CLK_CONTINUE,EN_CLK_STOP};void en_clock(int msg);
extern volatile uint8_t en_playing,en_frozen,en_ext,en_step,en_scene;
extern volatile uint32_t en_notes,en_hits,en_greats,en_years;
extern volatile float en_bpm,en_energy[8],en_freq[8],en_scene_phase;
extern volatile uint8_t en_note_grid[8][64],en_layer_step[8];
extern volatile uint8_t en_bus_on;extern float en_bus[];

/* Audio-thread field simulation: UI observes it; drawing never clocks the music. */
extern volatile float en_field_x[8],en_field_y[8],en_field_r[8],en_field_angle[8];
extern volatile float en_field_time,en_attractor_mass;
extern volatile uint32_t en_captures,en_field_cycles[8];

extern volatile uint32_t en_sleep_remaining;

extern const char *const EVENT_NAME[8];
extern volatile uint8_t en_event;
extern volatile float en_event_mix;

void en_cosmic(int type,int layer);
extern volatile uint8_t en_event_pending,en_remix_layer;
extern volatile float en_nebula,en_fog,en_star,en_comet,en_nova,en_hole_x,en_hole_y,en_hole_age,en_ripple,en_life[8];
extern volatile uint32_t en_births;

extern volatile float en_growth,en_flash,en_death[8],en_shock[8],en_wave[3],en_wave_power[3];
extern volatile float en_object_r[12],en_object_a[12],en_object_x[12],en_object_y[12];
extern volatile uint32_t en_objects_captured;
extern volatile uint8_t en_hole_swallowed;

extern volatile float en_pulse;

#define EN_PLANETS 8
typedef struct{uint8_t pitch,level,space,hold;} en_planet_t;
typedef struct{uint8_t count[8];en_planet_t planet[8][8];} en_planets_t;
extern volatile en_planets_t en_planets;
extern volatile float en_collision_glow[8];
extern volatile float en_spin[8],en_galaxy_growth[8],en_wave_x[3],en_wave_y[3];
extern volatile uint32_t en_bowls,en_collisions;
extern volatile uint8_t en_bass_section;
extern const char *const BASS_NAME[9];
void en_planet_set(int layer,int planet,int field,int value);
void en_planets_restore(const en_planets_t *state);
void en_planets_snapshot(en_planets_t *state);

void en_planets_default(uint32_t seed,en_planets_t *p);

extern volatile float en_planet_flash[8][8];
extern volatile uint8_t en_planet_growth[8][8];
void en_planet_position(int l,int p,float *x,float *y);

/* Shared presentation contract: the UI describes the actual scheduler and mix. */
enum{EN_TRACK_OFF=1,EN_TRACK_MUTED=2,EN_TRACK_SOLO_OUT=4,EN_TRACK_SCENE_OUT=8,EN_TRACK_LEVEL_ZERO=16,EN_TRACK_DENSITY_ZERO=32,EN_TRACK_REST=64};
uint32_t en_track_flags(int layer);
uint8_t en_scene_mask(int scene);
int en_note_at(int layer,unsigned position);
extern volatile float en_meter[8];
extern volatile uint16_t en_trigger[8];
extern volatile uint8_t en_scene_pending;

extern const char *const KIT_NAME[EN_KITS];

extern volatile float en_object_life[12],en_core_impact;
extern volatile uint32_t en_chimes,en_core_chords;
extern volatile uint8_t en_last_planet[8];
extern const char *const THEME_NAME[4];

extern volatile float en_comet_vx[12],en_comet_vy[12],en_comet_tail[12][10][2],en_growth_age;
extern volatile float en_balance[8],en_mix_limit;
extern volatile uint8_t en_drum_phase;

extern volatile float en_breath,en_presence[8];
float en_world_growth(int l,int p);

/* GRAVITY BLOOM runtime state. Audio thread owns movement and musical events. */
extern volatile uint8_t en_bloom_mode,en_bloom_eternal,en_bloom_phase,en_bloom_seen,en_bloom_target,en_bloom_combo,en_bloom_links[8];
extern volatile uint32_t en_bloom_visits,en_bloom_starlight,en_bloom_cycles,en_bloom_notes;
extern volatile float en_spirit_x,en_spirit_y,en_spirit_tail[18][2],en_bloom_awake[8],en_bloom_burst,en_bloom_phase_age,en_bloom_spark[12][3];
void en_bloom_aim(int layer);void en_bloom_launch(void);

typedef struct{float x,y,age,power;uint8_t kind,layer;uint16_t serial;uint32_t nova;} en_bloom_fx_t;
extern volatile en_bloom_fx_t en_bloom_fx[6];
extern volatile float en_bloom_blossom[8];

int en_perfect_degree(int semitone);
extern volatile float en_orbit_rate[8];
extern volatile uint32_t en_bumps;

extern volatile uint8_t en_sun_mask,en_solar_remnant;

/* Pre-output visual meters and three bounded musical sky objects. */
extern volatile float en_visual[8];
typedef struct {float x,y,phase,light;uint8_t kind,layer;} en_sky_t;
extern volatile en_sky_t en_sky[3];

/* Latest real supernova, published by the audio thread for camera choreography. */
extern volatile uint32_t en_supernova_serial;
extern volatile float en_supernova_x,en_supernova_y;

/* Clock roles:0 auto/input,1 master,2 follow (no free-running after lost clock). */
extern volatile uint8_t en_sync_role;void en_sync(int role);

/* Session play-along: Explore, Free Key, Perfect Key; Single, Triad, Sus, Seventh. */
extern volatile uint8_t en_live_mode,en_chord_mode,en_cue_kind[8];
extern volatile float en_cue_age[8];
void en_assist(int mode,int chord);
void en_strum(int ms);extern volatile uint16_t en_strum_ms;
typedef struct {float age;uint8_t a,b;} en_discharge_t;
extern volatile en_discharge_t en_discharge[4];

extern volatile float en_cue_x[8],en_cue_y[8];

float en_bass_root(void);

/* Bounded rare singularity / GRB choreography and track-birth publication. */
extern volatile uint8_t en_grb_phase,en_grb_count,en_grb_goal,en_grb_mask;
extern volatile float en_grb_age,en_grb_x,en_grb_y;
extern volatile uint32_t en_grb_serial,en_track_revision[8];
extern volatile uint8_t en_track_patch[8],en_track_variant[8];

int en_drum_note(unsigned at);

/* Deadline telemetry and optional graphics protection; no load-driven note cuts. */
void en_audio_pressure(uint32_t used_us,uint32_t budget_us);
extern volatile uint8_t en_graphics_throttle;
extern volatile uint32_t en_audio_max_us,en_pressure_events;
extern volatile uint32_t en_shed_count;extern volatile uint8_t en_voice_budget,en_active_voices;

/* Post-fader peak hold and persistent clip countdown, owned by audio. */
extern volatile float en_peak[8],en_duck[8];
extern volatile uint16_t en_clip[8],en_master_clip;

/* Session-only diagnostic isolation: 0 normal, 1 audio only, 2 graphics only, 3 core0 only. */
void en_diagnostic(int mode);
extern volatile uint8_t en_diagnostic_mode;

void en_intention(int intention);

void en_keyboard_octave(int octave);extern volatile int8_t en_key_octave;

unsigned en_command_room(void);

int en_reference_for(int intention,int anchor);

void en_quality(int high);void en_drum_parts(unsigned mask);
extern volatile uint8_t en_drum_parts_mask;
void en_output_render(int32_t *out,uint32_t n,uint32_t gain);

extern volatile int8_t en_grb_victim;

extern volatile uint8_t en_note_velocity[7], en_held_lanes;
extern volatile uint32_t en_note_serial[8]; /* Actual note-on publication for key lights. */

int en_style_scale(int intention,int reference);

extern volatile uint8_t en_sleep_expired;

/* en_visual: post-track-mix, pre-master artistic response. Includes played voices;
 * unaffected by master volume, output mute/route or hardware attenuation. */

/* Read-only UI response model; not an output spectrum or oscilloscope. */
#define EN_PHRASE_EVENTS 16
typedef struct{uint32_t sample,length;int16_t degree;uint8_t velocity,manual;}en_phrase_event_t;
typedef struct{uint32_t sample,step;int chance,bars,automatic,density,scene,harmony;float energy;}en_evolution_t;
unsigned en_phrase_snapshot(int layer,en_phrase_event_t out[16],uint32_t *now);
unsigned en_motif_template(int layer,uint8_t out[64]);void en_evolution_snapshot(en_evolution_t *out);
void en_core_restore(uint32_t stage);
typedef struct{float room_predelay,room_damping,room_mod,dry_rms,wet_rms,echo_feedback,room_cycle,echo_seconds,echo_mix,room_mix,room_feedback,width,phase,vibrato_hz,vibrato_cents,vibrato_ramp,lane_send,lane_echo,glide_ratio,glide_ms,vibrato_delay;int active;}en_response_t;
void en_response(int layer,en_response_t *r);

extern volatile uint32_t en_core_evolution;extern volatile float en_core_turn;

/* Actual post-fader mono lane energy, before master volume and shared FX. */
extern volatile float en_lane_rms[8],en_lane_peak[8],en_dry_rms,en_wet_rms;

extern volatile uint8_t en_soundscape_reason;
extern volatile uint32_t en_soundscape_decodes,en_soundscape_transitions;

int en_control_pending(int p);
extern volatile uint32_t en_control_apply_samples[3],en_control_event_samples[3],en_control_chosen_samples[3];

unsigned en_core_percentile(unsigned core,unsigned pct);

unsigned en_control_latency(unsigned ci,unsigned event);

extern volatile uint8_t en_host_routes,en_host_slot[8];

extern volatile uint32_t en_dsp_stage_us[4],en_dsp_stage_max_us[4],en_queue_drops,en_clock_coalesced;

void en_air_filter(unsigned track,unsigned cut,unsigned res);
unsigned en_air_applied_percent(unsigned track,unsigned destination);
void en_air_control(unsigned track,unsigned destination,unsigned value);

extern volatile float en_audio_band[3];
