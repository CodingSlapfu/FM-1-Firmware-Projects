/* SPDX-License-Identifier: GPL-3.0-only
 * ATTRACTOR-1 v0.2: original Galaxy Engine. Platform is Felucca/GRAV-derived;
 * composition, 8-layer scheduling, tone synthesis and mix below are new. Created by Ziao. */
#include "amb.h"
#include "fastmath.h"
#include "galaxy_plan.h"
#include "neoclassical_phrase.h"
#include "perfect_note.h"
#ifdef EN_HOST
#define BIG
#else
#define BIG __attribute__((section(".pool")))
#endif
#define BARRIER() __asm__ volatile("":::"memory")
const char *const GALAXY_NAME[8]={"PADS","BASS","LEAD","CHORDS","HARMONY","ACCENTS","MELODY","DRUMS"};
const char *const VOICE_NAME[EN_PATCHES]={"Warm pad","Crystal bowl","Soft keys","Glass chime","Vowel pad","Light bell","Silk drone","Soft mallet","Halo Bloom","Temple Bowl","Alpha Keys","Star Glass","Angelic Choir","Distant Bell","Deep Ether","Velvet Pulse","Moonlit Silk","Solar Choir","Cloud Cathedral","Velvet Horizon","Pearl Mist","Deep Constellation","Aurora Strings","Dawn Halo","Ambient Nylon","Distant Steel","Shimmer Guitar","Muted Moon Guitar","Celestial Harp","Glass Harp","Velvet Harp","Starlight Harp","Seraph Breath","Lunar Organ","Opal Choir","Quiet Aurora","Pearl FM Keys","Orbit FM Bell","Velvet FM Pad","FM Glass Harp","Celestial Flutes","Aether Harp","Soft Halo Nylon","Warm Seraph Pad","Whisper Choir","Lunar Bowl","Felt Star Keys","Meridian Drone","Gentle Comet","Sacred Earth Bass","Angel Mallet","Velvet Air","Halo Velvet","Halo Root","Halo Pearl","Halo Pluck","Halo Choir","Halo Dew","Halo Ether","Flow Silk","Flow Anchor","Flow Keys","Flow Harp","Flow Air","Flow Chime","Flow Strings","Still Cloud","Still Root","Still Ember","Still Echo","Still Breath","Still Star","Still Veil","Sub Halo Bed","Sub Halo Root","Sub Halo Opal","Sub Halo Drop","Sub Halo Choir","Sub Halo Bell","Sub Halo Mist","Orbit Warmth","Orbit Root","Orbit Felt","Orbit Nylon","Orbit Choir","Orbit Spark","Orbit Silk","Anchor Dark","Anchor Root","Anchor Pearl","Anchor Echo","Anchor Breath","Anchor Star","Anchor Ether","Tide Cloud","Tide Root","Tide Keys","Tide Ripple","Tide Air","Tide Bell","Tide Horizon","Water Warmth","Water Root","Water Piano","Water Drops","Water Air","Water Twinkle","Water Veil","Seraph Flute","Bamboo Breath","Velvet Woodwind","Aether Oboe","Silk Ensemble","Halo Cello","Chamber Felt","Intimate Upright","Warm Cello","Chamber Strings","Felt Celesta","Chamber Haze","Soft Harmonium","Broken Piano","Angelic Live Chorus","Celestial Female Hum","Velvet Alto Hum","Warm Male Hum","Water Droplet","Water Ripple","Flowing Water","Water Bubble"};
const char *const SCENE_NAME[8]={"AWAKEN","OPEN","BLOOM","SUSPEND","DRIFT","RADIATE","BREATHE","RETURN"};
const char *const INTENT_NAME[10]={"Stillness","Fortune","Release","Renewal","Prosperity","Love","Connection","Clarity","Success","Oneness"};
const float HEALING_HZ[EN_NFREQ]={174,285,396,417,432,444,528,639,741,777,852,888,963};
#include "reference_roles.h"
#include "choir_sample.h"
#include "living_vocals.h"
typedef struct{const int16_t *data;unsigned length;float root;} vocal_bank_t;
static const vocal_bank_t vocal_banks[4]={{choir_sample,CHOIR_LEN,CHOIR_ROOT},{vocal_female,VOCAL_FEMALE_LEN,VOCAL_FEMALE_ROOT},{vocal_alto,VOCAL_ALTO_LEN,VOCAL_ALTO_ROOT},{vocal_male,VOCAL_MALE_LEN,VOCAL_MALE_ROOT}};
static int patch_strum(int p){const role_patch_t *r=role_patch(p);if(r)return r->strum;return (p>=24&&p<32)||p==39||(p>=40&&p<=42);}
static int patch_type(int p){const role_patch_t *r=role_patch(p);if(r)return r->type;static const uint8_t type[EN_PATCHES]={0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,6,4,0,0,4,6,0,4,7,7,7,7,5,5,5,5,0,6,4,0,2,1,0,5,0,5,7,0,4,1,2,6,3,6,1,4};return type[(unsigned)p<ROLE_PATCH_BASE?p:0];}
static int patch_expressive(int p,int layer){return layer==1||p==40||(p>=108&&p<114)||p==116||p==117||p==119||p==120||(!patch_strum(p)&&(patch_type(p)==0||patch_type(p)==4||patch_type(p)==6));}
volatile uint32_t en_track_revision[8];volatile uint8_t en_track_patch[8],en_track_variant[8];
volatile uint8_t en_grb_phase,en_grb_count,en_grb_goal,en_grb_mask;
volatile float en_grb_age,en_grb_x,en_grb_y;volatile uint32_t en_grb_serial;
volatile int8_t en_grb_victim=-1;
static float grb_wait,grb_feed_age,grb_start_life;static int grb_target=-1;
static float cosmic_gain[8],cosmic_tail[8],cosmic_bloom;static int cosmic_last=-1;
static void track_consume(int l);
static void track_silence(int l);static void track_destroy(int l);static void track_birth(int l);static void grb_begin(void);
volatile uint32_t en_note_serial[8];
volatile uint8_t en_note_velocity[7], en_held_lanes;
volatile float en_lane_rms[8],en_lane_peak[8],en_dry_rms,en_wet_rms;
static uint32_t lane_last_note[8];
volatile float en_meter[8];volatile uint16_t en_trigger[8];volatile uint8_t en_scene_pending=255;
volatile uint8_t en_playing=1,en_frozen,en_ext,en_step,en_scene,en_bus_on;
volatile uint32_t en_notes,en_hits,en_greats,en_years;
volatile float en_bpm=48,en_energy[8],en_freq[8],en_scene_phase;
volatile uint8_t en_note_grid[8][64],en_layer_step[8];
#ifdef EN_HOST
/* Multibus export belongs to the browser/host adapter, not stereo FM1 hardware. */
float en_bus[EN_NBUS*1024];
#endif
float en_sample_rate=44100.0f;
volatile uint8_t en_drum_parts_mask=31;
static int16_t par[P_NPARAMS];
volatile uint8_t en_output_route=1;static uint8_t output_external;
void en_output_detect(int external){output_external=(uint8_t)(external!=0);en_output_route=(uint8_t)(par[P_OUTPUT]==2?!output_external:par[P_OUTPUT]==1);}
int en_output_auto(void){return par[P_OUTPUT]==2;}
volatile uint32_t en_remote_serial;static volatile uint8_t remote_rev[P_NPARAMS];
unsigned en_remote_revision(unsigned p){return p<P_NPARAMS?remote_rev[p]:0;}
int en_remote_value(unsigned p){return p<P_NPARAMS?par[p]:0;}
volatile uint8_t en_patch_locks;
const char *const REFERENCE_NAME[10]={"Auto","1 Warm Halo","2 Open Flow","3 Deep Still","4 Sub Halo","5 Warm Orbit","6 Deep Anchor","7 Octave Tide","8 Quiet Water","9 Chamber Piano"};
/* Original profiles keep their IDs and voicing. New octave-related bodies follow
 * measured ref4-7 low-frequency dominance, never fixed recording pitches. */
/* Profiles measured against the supplied recordings: dark registers, slow
 * envelopes and bass-pair width. No sample playback or fixed recording notes. */
typedef struct {float ceiling,carrier,partial,attack,hold,release,retrigger,strum,swell,basspan,bodyratio;} reference_t;
static const reference_t reference[8]={
 {165,.27f,1,2.5f,22,10,16,3.8f,.024f,.42f,1.5f},
 {352,.48f,1.25f,1.8f,16,8,10,2.6f,.035f,.92f,2.2f},
 {110,.18f,.95f,3.6f,30,13,24,5.8f,.014f,.46f,1.33f},
 {110,.16f,.90f,3,28,12,20,5,.020f,.95f,1.5f},
 {220,.30f,1.10f,2.2f,24,10,16,4,.027f,.65f,3},
 {110,.08f,.75f,4.2f,34,14,26,6.2f,.012f,.42f,1.5f},
 {110,.14f,.95f,3.2f,30,13,22,5.6f,.018f,.93f,2.75f},
 {110,.12f,.80f,3.8f,32,14,25,6.4f,.013f,.48f,1.5f}
};
/* Seven separately measured references: restrained low registers, role-aware
 * breathing and stereo space. Values are musical interpretations, not copies. */
typedef struct{float melody_gap,bass_gap,melody_hold,melody_tail,width,accent,drums;} phrasing_t;
static const phrasing_t phrasing[8]={
 {4.8f,9,2.6f,5.8f,.58f,.12f,.72f},
 {3.2f,6.5f,2.0f,4.8f,.95f,.22f,.78f},
 {6.0f,12,4.2f,8.0f,.62f,.10f,.60f},
 {5.2f,10,3.6f,7.4f,.98f,.07f,.62f},
 {4.0f,8,2.8f,6.2f,.74f,.09f,.70f},
 {6.8f,14,4.8f,9.0f,.60f,.08f,.54f},
 {5.6f,11,4.0f,8.2f,.94f,.10f,.62f},
 {8.2f,14,2.8f,10.0f,.78f,.06f,.42f}
};
static int8_t auto_reference=-1;static uint8_t mix_intention=5;
static uint32_t seed=1,clock_samples,last_clock_sample,clock_interval,clock_count,ext_ticks,local_ticks,bar;
int en_reference(void){return par[P_REFERENCE]>=1&&par[P_REFERENCE]<=9?fm_minf(7,par[P_REFERENCE]-1):auto_reference>=0?auto_reference:en_reference_for(mix_intention,par[P_ANCHOR])-1;}
static int current_harmony(void){static const uint8_t chord[8]={0,5,3,4,0,2,5,4};return par[P_STYLE]==EN_NEOCLASSICAL?chord[(bar/4u)&7u]:(int)gx_harmony(en_scene,bar,seed);}
volatile uint8_t en_sync_role;static float midi_clock_phase;static uint8_t clock_seen,master_was_playing,master_has_started;
static float step_phase,gate,mute_gain=1,wetrev,wetecho,width_s;
volatile uint8_t en_cinematic_stage;
volatile float en_cinematic_energy;
static uint32_t cinematic_start_step;
/* Four eight-bar phrases, smooth transitions; once per audio block. */
static float cinematic_form_target(uint32_t steps){
 float q=(float)(steps%128u)/128; q=q*q*(3-2*q);
 unsigned stage=(steps/128u)&3u;
 static const float edge[5]={.28f,.50f,.95f,.85f,.28f};
 return edge[stage]+(edge[stage+1]-edge[stage])*q;
}
static uint32_t sequence_step,scene_bars;static uint8_t muted;
volatile float en_field_x[8],en_field_y[8],en_field_r[8],en_field_angle[8],en_field_time,en_attractor_mass;
volatile uint32_t en_captures,en_field_cycles[8];
static uint8_t field_pending,field_force;
volatile float en_pulse;
static float shock_s[8];static uint8_t wave_degree[8];
volatile float en_growth,en_flash,en_death[8],en_shock[8],en_wave[3],en_wave_power[3];
volatile float en_object_r[12],en_object_a[12],en_object_x[12],en_object_y[12];
volatile uint32_t en_objects_captured;
volatile uint8_t en_hole_swallowed;
static uint8_t wave_hit[3],wave_next,core_priority[3];
volatile en_planets_t en_planets;
volatile float en_planet_flash[8][8];
volatile uint8_t en_planet_growth[8][8];
static uint8_t planet_wave_hit[3][8];
/* Authoritative child orbit: local ellipse, then parent rotation. The renderer
 * projects parent and children through one camera; a central sun is local0. */
void en_planet_position(int l,int p,float *x,float *y){
 float u=0,v=0;
 if(!(p==0&&(en_sun_mask&(1u<<l)))){float a=(float)p/(float)(en_planets.count[l]?en_planets.count[l]:1),r=(.100f+(float)p*.013f)*(1+en_galaxy_growth[l])*fm_maxf(.08f,en_life[l]);u=r*fm_sin_turns(a+.25f);v=r*fm_sin_turns(a)*.63f;}
 /* Rotate in the99:76 display plane, matching the spiral's parent frame. */
 float a=en_spin[l]+(float)l*.137f,cs=fm_sin_turns(a+.25f),sn=fm_sin_turns(a);
 *x=en_field_x[l]+u*cs-v*(76.0f/99.0f)*sn;*y=en_field_y[l]+u*(99.0f/76.0f)*sn+v*cs;
}

volatile float en_spin[8],en_galaxy_growth[8],en_wave_x[3],en_wave_y[3];
volatile uint32_t en_bowls,en_collisions;
volatile uint8_t en_bass_section;
const char *const KIT_NAME[EN_KITS]={"Soft Studio","Brush Room","Warm 808","Temple Air","Star Dust","Deep Halo","Lunar Dust","Glass Rain","Silk Hats","FM Moon Kit","Warm Pocket","Veil Brushes","Cloud Dust","FM Kit","Ref 1 Halo Pulse","Ref 2 Flow Brush","Ref 3 Still Felt","Ref 4 Sub Air","Ref 5 Orbit Wood","Ref 6 Anchor Dust","Ref 7 Tide Metal","Ref 8 Water Drop","Ref 9 Chamber","Ambient Mist","Nightfall Soft","Cinematic Frame","Neoclassical Wood"};
const char *const BASS_NAME[9]={"Auto","Foundation","Undertow","Bloom","Suspended","Descent","Breathe","Off","Sacred / Test"};
static float spin_speed[8];volatile float en_collision_glow[8];
#define collision_wait en_collision_glow
volatile uint8_t en_sun_mask,en_solar_remnant;
volatile uint32_t en_core_evolution;volatile float en_core_turn;
static float orbit_vx[7],orbit_vy[7];
static float orbit_turn(float x,float y){float ay=fm_fabsf(y)+.000001f,r,a;if(x>=0){r=(x-ay)/(x+ay);a=.125f;}else{r=(x+ay)/(ay-x);a=.375f;}a+=(.031244f*r*r-.156242f)*r;if(y<0)a=-a;return a<0?a+1:a;}
static void orbit_seed(int l){float x=en_field_x[l],y=en_field_y[l],r=fm_maxf(.1f,en_field_r[l]);float soft=r*r+.065f,pull=(float)par[P_PULL]*.01f;float speed=.995f*fm_sqrtf((.0012f+.007f*pull)*r*r/(soft*fm_sqrtf(soft)));float sign=en_orbit_rate[l]>0?1:-1;orbit_vx[l]=-y/r*speed*sign-x*.00015f;orbit_vy[l]=x/r*speed*sign-y*.00015f;}
volatile float en_visual[8];
volatile en_sky_t en_sky[3];
static uint8_t supernova_pending,bloom_ascension_burst;
static uint32_t lane_next_sample[8];static uint8_t phrase_cursor;
/* Each generated recipe/reference enters with audible enabled roles.
 * One anchor per lane; steady-state motifs remain separate. */
static uint8_t reference_entry;
static void core_impact(float power);
static uint16_t object_wave_hit[3];static uint8_t bowl_pending,bowl_next,bowl_override;
static uint32_t bowl_clock,impact_clock,core_clock;
static uint8_t chime_pending,impact_voice;
volatile float en_orbit_rate[8];volatile uint32_t en_bumps;
volatile uint16_t en_strum_ms;
volatile uint8_t en_live_mode,en_chord_mode,en_cue_kind[8];volatile float en_cue_age[8];
volatile float en_cue_x[8],en_cue_y[8];static float object_bowl_x[8],object_bowl_y[8];static uint8_t object_bowl_mask,object_bowl_play;
volatile en_discharge_t en_discharge[4];static uint8_t discharge_next,live_next,live_strike;
static float thunder_env,thunder_low;static uint32_t thunder_rng=0x987135u;
static float bump_r[8],bump_hold[8];static uint8_t pair_touch[8];
static float core_pending,wave_push[8],object_wait[12];
volatile float en_comet_vx[12],en_comet_vy[12],en_comet_tail[12][10][2],en_growth_age;
volatile float en_balance[8],en_mix_limit;
volatile float en_peak[8],en_duck[8];
volatile uint16_t en_clip[8],en_master_clip;
volatile uint8_t en_diagnostic_mode;
volatile uint8_t en_drum_phase;
static float balance_peak[8],duck_peak[8],comet_sample,breath_phase;
volatile float en_breath,en_presence[8];
static uint32_t universe_samples;
static float ease(float x){x=fm_clampf(x,0,1);return x*x*(3-2*x);}
float en_world_growth(int l,int p){return ease((en_life[l]-(float)p*.035f)/(.9f-(float)p*.035f));}
static uint32_t drum_cycle;
volatile float en_object_life[12],en_core_impact;
volatile uint32_t en_chimes,en_core_chords;
volatile uint8_t en_last_planet[8];
const char *const THEME_NAME[4]={"Sanctuary","Glacier","Sakura Veil","Solar Gold"};
/* GRAVITY BLOOM: bounded, audio-clocked journey; drawing never advances it. */
volatile uint8_t en_bloom_mode,en_bloom_eternal,en_bloom_phase,en_bloom_seen,en_bloom_target,en_bloom_combo,en_bloom_links[8];
volatile uint32_t en_bloom_visits,en_bloom_starlight,en_bloom_cycles,en_bloom_notes;
volatile float en_spirit_x,en_spirit_y,en_spirit_tail[18][2],en_bloom_awake[8],en_bloom_burst,en_bloom_phase_age;
volatile float en_bloom_spark[12][3];
static float bloom_from[2],bloom_ctrl[2],bloom_t,bloom_duration,bloom_orbit,bloom_dwell,bloom_tail_clock,bloom_combo_age;
static uint8_t bloom_flying,bloom_aim,bloom_launch_pending,bloom_pending,bloom_chord,bloom_last;
volatile en_bloom_fx_t en_bloom_fx[6];
volatile float en_bloom_blossom[8];
static uint16_t bloom_fx_serial;
/* Dedicated camera event survives ordinary effect serial resets and skipped UI frames. */
volatile uint32_t en_supernova_serial;volatile float en_supernova_x,en_supernova_y;
static void bloom_fx(float x,float y,float power,int kind,int layer){
 int slot=-1;for(int i=0;i<6;i++)if(((en_bloom_fx[i].kind!=6&&en_bloom_fx[i].kind!=7)||en_bloom_fx[i].age>=24)){if(slot<0||en_bloom_fx[i].age>en_bloom_fx[slot].age)slot=i;}
 if(slot<0){if(kind!=6&&kind!=7)return;slot=0;for(int i=1;i<6;i++)if(en_bloom_fx[i].age>en_bloom_fx[slot].age)slot=i;}
 en_bloom_fx[slot]=(en_bloom_fx_t){x,y,0,fm_clampf(power,0.1f,1),(uint8_t)kind,(uint8_t)(layer&7),++bloom_fx_serial,(kind==6||kind==7)?en_supernova_serial+1u:0};
 if(kind==6||kind==7){en_supernova_x=x;en_supernova_y=y;BARRIER();en_supernova_serial++;}
}
static void bloom_init(void){
 en_spirit_x=0.18f;en_spirit_y=-0.24f;en_bloom_target=0;en_bloom_seen=en_bloom_phase=en_bloom_combo=0;
 en_bloom_visits=en_bloom_starlight=en_bloom_cycles=en_bloom_notes=0;en_bloom_phase_age=en_bloom_burst=0;
 bloom_flying=bloom_aim=bloom_launch_pending=bloom_pending=bloom_chord=bloom_ascension_burst=0;bloom_last=255;
 bloom_t=bloom_orbit=bloom_tail_clock=bloom_combo_age=0;bloom_dwell=0;
 for(int j=0;j<18;j++){en_spirit_tail[j][0]=en_spirit_x;en_spirit_tail[j][1]=en_spirit_y;}
 bloom_fx_serial=0;for(int i=0;i<6;i++){en_bloom_fx[i].age=24;en_bloom_fx[i].nova=0;}
 for(int l=0;l<EN_NGAL;l++){en_bloom_blossom[l]=0;en_bloom_awake[l]=0;en_bloom_links[l]=0;}
 for(int i=0;i<12;i++){uint32_t h=gx_hash(seed+(uint32_t)i*137u);float a=(float)(h&1023u)/1024,r=0.28f+(float)((h>>12)%600u)*0.001f;en_bloom_spark[i][0]=r*fm_sin_turns(a+0.25f);en_bloom_spark[i][1]=r*fm_sin_turns(a);en_bloom_spark[i][2]=1;}
}
static void bloom_depart(void){
 int target=bloom_aim%EN_NGAL;if(en_death[target]>0||en_life[target]<0.4f)return;
 en_bloom_target=(uint8_t)target;bloom_from[0]=en_spirit_x;bloom_from[1]=en_spirit_y;
 float dx=en_field_x[target]-en_spirit_x,dy=en_field_y[target]-en_spirit_y;
 float d=fm_sqrtf(dx*dx+dy*dy),bend=(target&1?1:-1)*0.48f;
 bloom_ctrl[0]=(en_spirit_x+en_field_x[target])*0.5f-dy*bend;
 bloom_ctrl[1]=(en_spirit_y+en_field_y[target])*0.5f+dx*bend;
 bloom_duration=4.0f+d*3.5f;bloom_t=0;bloom_flying=1;bloom_launch_pending=0;
}
static void bloom_arrive(int l){
 en_bloom_blossom[l]=fm_minf(1,en_bloom_blossom[l]+0.18f);bloom_fx(en_field_x[l],en_field_y[l],0.8f,0,l);
 en_bloom_visits++;en_bloom_awake[l]=1;bloom_pending|=(uint8_t)(1u<<l);
 if(bloom_last<EN_NGAL&&bloom_last!=l){en_bloom_links[l]|=(uint8_t)(1u<<bloom_last);en_bloom_links[bloom_last]|=(uint8_t)(1u<<l);}
 if(bloom_combo_age<18&&bloom_last!=l)en_bloom_combo=(uint8_t)(en_bloom_combo<4?en_bloom_combo+1:1);else en_bloom_combo=1;
 bloom_combo_age=0;bloom_last=(uint8_t)l;if(en_bloom_combo==4){bloom_chord=1;en_bloom_burst=0.6f;bloom_fx(0,0,0.65f,4,l);}
 en_bloom_seen|=(uint8_t)(1u<<l);int count=0;for(int i=0;i<EN_NGAL;i++)count+=(en_bloom_seen>>i)&1;
 uint8_t phase=count==EN_NGAL?2:count>=3?1:0;
 if(phase!=en_bloom_phase){en_bloom_phase=phase;en_bloom_phase_age=0;en_bloom_burst=1;bloom_fx(0,0,phase==2?1:0.7f,4,l);if(phase==2)bloom_chord=1;}
 /* Pattern answers evolve at a bar boundary; the main motif survives. */
 if(par[P_EVOLVE]&&gx_hash(seed^en_bloom_visits)%100u<(unsigned)par[P_EVOLVE])field_pending|=(uint8_t)(1u<<l);
}
static void bloom_advance(float dt){
 if(en_frozen||!en_playing)return;
 if((par[P_REST]==EN_NIGHTFALL))dt*=0.55f;
 en_bloom_phase_age+=dt;bloom_combo_age+=dt;en_bloom_burst=fm_maxf(0,en_bloom_burst-dt*0.18f);
 for(int l=0;l<EN_NGAL;l++)en_bloom_awake[l]=fm_maxf(0,en_bloom_awake[l]-dt*0.014f);
 if(en_bloom_phase==2&&en_bloom_eternal&&en_bloom_phase_age>=28&&!bloom_ascension_burst){bloom_ascension_burst=1;bloom_fx(0,0,1,7,0);core_impact(.65f);en_scene_pending=(uint8_t)((en_scene+1u)&7u);field_pending|=0x7du;}
 if(en_bloom_phase==2&&en_bloom_eternal&&en_bloom_phase_age>32){bloom_ascension_burst=0;en_bloom_cycles++;en_bloom_seen=en_bloom_phase=0;en_bloom_phase_age=0;for(int l=0;l<EN_NGAL;l++)en_bloom_links[l]=0;}
 bloom_orbit+=dt*0.048f;bloom_orbit-=fm_floorf(bloom_orbit);
 if(bloom_flying){
  bloom_t=fm_minf(1,bloom_t+dt/bloom_duration);float u=bloom_t,q=1-u;
  en_spirit_x=q*q*bloom_from[0]+2*q*u*bloom_ctrl[0]+u*u*en_field_x[en_bloom_target];
  en_spirit_y=q*q*bloom_from[1]+2*q*u*bloom_ctrl[1]+u*u*en_field_y[en_bloom_target];
  if(u>=1){bloom_flying=0;bloom_dwell=0;bloom_orbit=0;if(en_life[en_bloom_target]>.4f&&!en_death[en_bloom_target])bloom_arrive(en_bloom_target);}
 }else{
  bloom_dwell+=dt;
  float radius=fm_minf(0.15f,bloom_dwell*0.045f),x=en_field_x[en_bloom_target]+fm_sin_turns(bloom_orbit+0.25f)*radius,y=en_field_y[en_bloom_target]+fm_sin_turns(bloom_orbit)*radius;
  /* Smooth orbit following also handles a galaxy's invisible respawn. */
  float a=fm_minf(1,dt*0.7f);en_spirit_x+=(x-en_spirit_x)*a;en_spirit_y+=(y-en_spirit_y)*a;
  if(en_bloom_mode==0&&bloom_dwell>3){
   /* Coprime visitation order touches all seven, with seeded cycle variation. */
   int stride=1+(int)(gx_hash(seed^en_bloom_cycles)%(EN_NGAL-1));
   bloom_aim=(uint8_t)((en_bloom_target+stride)%EN_NGAL);bloom_launch_pending=1;
  }
  if(bloom_launch_pending)bloom_depart();
 }
 en_spirit_x=fm_clampf(en_spirit_x,-1.15f,1.15f);en_spirit_y=fm_clampf(en_spirit_y,-1.15f,1.15f);
 bloom_tail_clock+=dt;if(bloom_tail_clock>0.075f){bloom_tail_clock-=0.075f;for(int j=17;j>0;j--){en_spirit_tail[j][0]=en_spirit_tail[j-1][0];en_spirit_tail[j][1]=en_spirit_tail[j-1][1];}en_spirit_tail[0][0]=en_spirit_x;en_spirit_tail[0][1]=en_spirit_y;}
 for(int i=0;i<12;i++){
  if(en_bloom_spark[i][2]<1){en_bloom_spark[i][2]=fm_minf(1,en_bloom_spark[i][2]+dt*0.04f);continue;}
  float x=en_bloom_spark[i][0],y=en_bloom_spark[i][1];float a=dt*0.008f;en_bloom_spark[i][0]=x-y*a;en_bloom_spark[i][1]=y+x*a;
  float dx=x-en_spirit_x,dy=y-en_spirit_y;if(dx*dx+dy*dy<0.0036f){en_bloom_spark[i][2]=0;en_bloom_starlight++;bloom_fx(x,y,0.30f,3,en_bloom_target);if(en_playing)bloom_pending|=(uint8_t)(1u<<en_bloom_target);}
 }
}
static en_planets_t restore_planets;
static void planets_new(int l){en_galaxy_growth[l]=0;
 uint32_t h=gx_hash(seed^en_field_cycles[l]*0x9e3779b9u^(uint32_t)l*97u);
 for(int i=0;i<8;i++){en_planet_flash[l][i]=0;en_planet_growth[l][i]=0;uint32_t v=gx_hash(h+(uint32_t)i*83u);en_planets.planet[l][i]=(en_planet_t){(uint8_t)(l==0?0:v%7u),(uint8_t)(65+v%36u),(uint8_t)(20+(v>>8)%71u),(uint8_t)(35+(v>>16)%46u)};}
 en_planets.count[l]=(uint8_t)(3+h%6u);en_spin[l]=(float)(h&1023u)/1024;
 spin_speed[l]=(0.006f+(float)((h>>12)%31u)*0.00025f)*(h&1?1:-1);
}
void en_planets_default(uint32_t value,en_planets_t *p){for(int l=0;l<8;l++){uint32_t h=gx_hash(value^(uint32_t)l*97u);p->count[l]=(uint8_t)(3+h%6u);for(int i=0;i<8;i++){uint32_t v=gx_hash(h+(uint32_t)i*83u);p->planet[l][i]=(en_planet_t){(uint8_t)(l==0?0:v%7u),(uint8_t)(65+v%36u),(uint8_t)(20+(v>>8)%71u),(uint8_t)(35+(v>>16)%46u)};}}}
void en_planets_snapshot(en_planets_t *p){for(int l=0;l<8;l++){if(en_planets.count[l]<3||en_planets.count[l]>8)continue;p->count[l]=en_planets.count[l];for(int i=0;i<8;i++)p->planet[l][i]=en_planets.planet[l][i];}}
static float core_edge(void){return 0.045f;}
static void burst_at(float x,float y,float power){
 if(power<0.28f&&x*x+y*y<0.01f){en_pulse=fm_maxf(en_pulse,0.13f);en_attractor_mass=fm_maxf(en_attractor_mass,0.08f);return;}
 if(power<0.95f&&x*x+y*y>0.01f){for(int i=0;i<3;i++)if(en_wave_power[i]>0.25f)return;}
 int w=-1;wave_next++;
 for(int j=0;j<3;j++){if(core_priority[j]&&en_wave_power[j]>0&&en_wave[j]<1.4f&&power<0.95f)continue;if(w<0||en_wave_power[j]<en_wave_power[w])w=j;}
 if(w<0){en_pulse=fm_maxf(en_pulse,power);return;}
 if(power>=en_wave_power[w]){en_wave[w]=0;en_wave_power[w]=power;wave_hit[w]=0;object_wave_hit[w]=0;for(int l=0;l<8;l++)planet_wave_hit[w][l]=0;en_wave_x[w]=x;en_wave_y[w]=y;core_priority[w]=(uint8_t)(power>=0.95f&&x*x+y*y<0.01f);}
 en_pulse=fm_minf(1,en_pulse+power);
 if(x*x+y*y<0.01f){en_flash=fm_minf(1,en_flash+power);en_attractor_mass=fm_minf(1,en_attractor_mass+power*0.5f);}
}
static void core_burst(float power){burst_at(0,0,power);}
static void core_impact(float power){bloom_fx(0,0,power,2,0);core_burst(power);en_core_impact=1;if(en_playing)core_pending=fm_maxf(core_pending,power);}
static void galaxy_collision(int l,float x,float y){
 if(collision_wait[l]>0 || en_death[l]>0)return;
 bloom_fx(x,y,0.65f,1,l);
 collision_wait[l]=4;uint32_t spin=gx_hash(seed^en_collisions^(uint32_t)l*193u);spin_speed[l]=(spin&1?1:-1)*(.015f+(float)((spin>>8)%21u)*.001f);en_collisions++;en_galaxy_growth[l]=fm_minf(0.25f,en_galaxy_growth[l]+0.025f);
 /* Collisions are visual/physical only; no chime, bowl or phrase injection. */
 /* bloom_fx retains the visible impact ripple without an audio wave. */
}
static void galaxy_capture(int l){
 if(en_death[l]>0)return;
 if((en_sun_mask&(1u<<l))&&en_core_evolution<0xffffffffu)en_core_evolution++;
 en_growth_age=0;
 if(en_sun_mask&(1u<<l)){bloom_fx(en_field_x[l],en_field_y[l],1,6,l);supernova_pending|=(uint8_t)(1u<<l);}else bloom_fx(0,0,.7f,6,l);
 en_captures++;en_field_cycles[l]++;en_death[l]=6;en_life[l]=0;track_consume(l);
 en_growth=fm_minf(1,en_growth+0.018f*(1-en_growth));
 core_impact(1);
}
/* Audio-clocked weather; a single bounded eight-slot universe. */
const char *const EVENT_NAME[8]={"LIVING COSMOS","FILAMENT ALIGN","TIDAL BLOOM","QUIET ECLIPSE","DISTANT NOVA","ROGUE SINGULARITY","GALAXY REBIRTH","COMET CHOIR"};
volatile uint8_t en_event,en_event_pending,en_remix_layer;
volatile float en_event_mix,en_nebula,en_fog,en_star,en_comet,en_nova;
volatile float en_hole_x,en_hole_y,en_hole_age,en_ripple;
volatile float en_life[8];
volatile uint32_t en_births;
static uint8_t event_bars,event_next=1,remix_pending,ripple_hit;
static float weather_time,sun_event_wait;
volatile uint32_t en_solar_novas,en_solar_holes;
static void solar_evolve(uint32_t h){
 if(sun_event_wait>0||en_grb_phase||(h&31u))return;
 for(unsigned q=0;q<EN_NGAL;q++){unsigned l=((h>>8)+q)%EN_NGAL;if(!(en_sun_mask&(1u<<l))||(en_solar_remnant&(1u<<l))||en_death[l]||en_life[l]<.9f)continue;
  bloom_fx(en_field_x[l],en_field_y[l],1,6,l);supernova_pending|=(uint8_t)(1u<<l);en_solar_remnant|=(uint8_t)(1u<<l);en_solar_novas++;sun_event_wait=90;
  if((h>>16)%10u==0){grb_begin();en_grb_x=en_field_x[l];en_grb_y=en_field_y[l];en_solar_holes++;}break;
 }
}

static void event_begin(int type,int layer){
 en_event=(uint8_t)type;event_bars=4;en_event_mix=0;
 en_remix_layer=(uint8_t)((unsigned)layer%EN_NGAL);en_pulse=1;
 if(type==4){bloom_fx(en_sky[2].x,en_sky[2].y,.8f,6,layer);core_burst(0.5f);}
 if(type==5){grb_begin();en_hole_age=0;en_ripple=0;ripple_hit=0;en_hole_swallowed=1;en_hole_x=0.85f*fm_sin_turns(en_field_angle[layer&7]+0.25f);en_hole_y=0.85f*fm_sin_turns(en_field_angle[layer&7]);}
 if(type==6)remix_pending=1;
}
static void event_bar(void){
 if(en_frozen)return;
 if(par[P_EVOLVE]&&bar&&!(bar&7u))solar_evolve(gx_hash(seed^bar*977u^en_births));
 if(en_event_pending){int type=en_event_pending;en_event_pending=0;event_begin(type,en_remix_layer);}
 else if(en_event){if(en_event==5 && en_hole_age<36)event_bars=1;else if(event_bars)--event_bars;}
 else if(par[P_EVOLVE] && par[P_PULL] && bar && !(bar%((par[P_REST]==EN_NIGHTFALL)?32u:16u))){
  /* Singularities are rare: one per 320 bars, rather than every cycle. */
  int type=!(bar%320u)?5:event_next;
  event_next=(uint8_t)(event_next%7+1);if(event_next==5)event_next=6;
  event_begin(type,(int)(gx_hash(seed^bar)%EN_NGAL));
 }
}
static void grb_begin(void){if(en_grb_phase)return;en_grb_phase=1;en_grb_age=0;en_grb_count=en_grb_mask=0;en_grb_goal=(uint8_t)(2+(gx_hash(seed^en_captures)&1u));grb_target=-1;float a=(float)(gx_hash(seed^en_births)&1023u)/1024;en_grb_x=.72f*fm_sin_turns(a+.25f);en_grb_y=.72f*fm_sin_turns(a);}

volatile float en_audio_band[3];static float band_low,band_mid,band_sum[3];
static volatile uint32_t air_filter_readout;
static float air_current[8][4];static uint32_t air_smoothing;static unsigned air_target_track,air_target_dest,air_target;
static uint32_t sleep_elapsed;static float night_mix,night_gain=1,wind_depth,wind_l,wind_r;
volatile uint32_t en_sleep_remaining;
volatile uint8_t en_sleep_expired;
/* Legacy timer parameter retained as a reserved zero-valued storage slot.
 * No timed stop, fade, scheduling or automatic shutdown remains. */


static void field_init(void){cosmic_bloom=0;cosmic_last=-1;for(int q=0;q<8;q++){cosmic_gain[q]=1;cosmic_tail[q]=0;}en_solar_remnant=0;sun_event_wait=90;en_solar_novas=en_solar_holes=0;int l;phrase_cursor=0;reference_entry=127u;for(int i=0;i<8;i++)lane_next_sample[i]=0;en_sun_mask=(uint8_t)((seed>>16)&0x55u)|2u;supernova_pending=0;for(int i=0;i<3;i++){en_sky[i]=(en_sky_t){(i==0?-.72f:i==1?.68f:.85f),(i==0?.40f:i==1?-.48f:.36f),0,0,(uint8_t)(i==2?2:1),(uint8_t)(i+2)};}en_field_time=en_attractor_mass=0;en_captures=0;field_pending=0;en_event=0;en_event_mix=0;event_bars=0;event_next=1;en_event_pending=remix_pending=0;en_births=0;weather_time=0;en_nebula=en_fog=en_star=en_comet=en_nova=0;en_hole_age=100;en_ripple=0;en_hole_swallowed=0;
 en_grb_phase=en_grb_count=en_grb_mask=0;en_grb_age=0;grb_target=-1;grb_wait=225+(float)(gx_hash(seed)%301u);for(int q=0;q<EN_NGAL;q++){track_silence(q);en_track_patch[q]=(uint8_t)(q==7?par[P_KIT]:par[LP(q,L_VOICE)]);en_track_variant[q]=(uint8_t)par[LP(q,L_VARIANT)];}
 en_growth_age=10;comet_sample=0;en_bumps=0;breath_phase=0;en_breath=0;
 en_bowls=en_collisions=bowl_clock=0;impact_clock=core_clock=0;chime_pending=impact_voice=0;core_pending=en_core_impact=0;en_chimes=en_core_chords=0;bowl_pending=bowl_next=bowl_override=0;en_bass_section=0;en_pulse=0;en_growth=en_flash=0;field_force=wave_next=0;en_objects_captured=0;
 for(l=0;l<3;l++){en_wave[l]=3;en_wave_power[l]=0;wave_hit[l]=0;object_wave_hit[l]=0;en_wave_x[l]=en_wave_y[l]=0;core_priority[l]=0;}
 for(l=0;l<12;l++){object_wait[l]=0;en_object_life[l]=1;en_object_r[l]=0.55f+(float)l*0.04f;en_object_a[l]=(float)l/12+(float)(seed&1023u)/1024;en_object_x[l]=en_object_r[l]*fm_sin_turns(en_object_a[l]+0.25f);en_object_y[l]=en_object_r[l]*fm_sin_turns(en_object_a[l]);}
 for(l=0;l<12;l++){en_comet_vx[l]=-en_object_y[l]*0.052f-en_object_x[l]*0.012f;en_comet_vy[l]=en_object_x[l]*0.052f-en_object_y[l]*0.012f;for(int j=0;j<10;j++){en_comet_tail[l][j][0]=en_object_x[l];en_comet_tail[l][j][1]=en_object_y[l];}}
 object_bowl_mask=object_bowl_play=0;thunder_env=0;for(int q=0;q<4;q++)en_discharge[q].age=0;for(int q=0;q<EN_NGAL;q++)en_cue_age[q]=0;
 bloom_init();
 for(l=0;l<EN_NGAL;l++){en_field_r[l]=0.90f+(float)(l%3)*0.065f;en_field_angle[l]=(float)l/(float)EN_NGAL+(float)(seed&65535u)/65536;
 en_orbit_rate[l]=((seed>>(l+8))&1u?-1.0f:1.0f)*(0.65f+(float)((seed>>(l+1))&7u)*0.05f);bump_r[l]=bump_hold[l]=0;pair_touch[l]=0;shock_s[l]=0;wave_degree[l]=0;wave_push[l]=0;en_last_planet[l]=0;en_galaxy_growth[l]=collision_wait[l]=0;en_field_cycles[l]=0;planets_new(l);en_death[l]=en_shock[l]=0;en_life[l]=en_presence[l]=1;en_field_x[l]=en_field_r[l]*fm_sin_turns(en_field_angle[l]+0.25f);en_field_y[l]=en_field_r[l]*fm_sin_turns(en_field_angle[l]);orbit_seed(l);}
 en_life[EN_DRUM_TRACK]=en_presence[EN_DRUM_TRACK]=en_field_r[EN_DRUM_TRACK]=en_field_x[EN_DRUM_TRACK]=en_field_y[EN_DRUM_TRACK]=0;en_death[EN_DRUM_TRACK]=0;planets_new(EN_DRUM_TRACK);en_sun_mask&=(1u<<EN_NGAL)-1u;
}
static void field_advance(float dt){int l;
 if(en_frozen||!en_playing)return;
 for(int i=0;i<6;i++){float old=en_bloom_fx[i].age;en_bloom_fx[i].age=fm_minf(24,old+dt);if((en_bloom_fx[i].kind==6||en_bloom_fx[i].kind==7)){for(int w=0;w<4;w++){float at=.2f+(float)w*3.5f;if(old<at&&en_bloom_fx[i].age>=at)burst_at(en_bloom_fx[i].x,en_bloom_fx[i].y,1-(float)w*.12f);}}}
 for(int i=0;i<3;i++){en_sky[i].phase+=dt*(i==2?.025f:.16f+(float)i*.07f);en_sky[i].phase-=fm_floorf(en_sky[i].phase);en_sky[i].light=fm_maxf(0,fm_sin_turns(en_sky[i].phase));}
 en_growth_age=fm_minf(10,en_growth_age+dt);
 breath_phase+=dt*((par[P_REST]==EN_NIGHTFALL)?0.065f:0.09f);breath_phase-=fm_floorf(breath_phase);en_breath=0.5f+0.5f*fm_sin_turns(breath_phase);
 /* Musical activity drives weather, weather feeds the scheduler and voices. */
 float energy=0;for(l=0;l<EN_NGAL;l++)energy+=en_energy[l]/(float)EN_NGAL;
 weather_time+=dt*((par[P_REST]==EN_NIGHTFALL)?0.35f:1);
 float amount=(float)par[P_HUMAN]*0.01f*((par[P_REST]==EN_NIGHTFALL)?0.35f:1);
 en_nebula=(0.5f+0.5f*fm_sin_turns(weather_time*0.006f))*amount;
 en_fog=(0.30f+0.70f*(0.5f+0.5f*fm_sin_turns(weather_time*0.0062f+0.3f)))*amount;
 en_star=(0.4f+energy*0.6f)*amount;
 en_comet=en_event==7?en_event_mix:amount*(0.5f+0.5f*fm_sin_turns(weather_time*0.021f));
 sun_event_wait=fm_maxf(0,sun_event_wait-dt);
 en_nova=en_event==4?en_event_mix:0;
 if(en_event==5){en_hole_age+=dt*((par[P_REST]==EN_NIGHTFALL)?0.35f:1);en_ripple=en_hole_age>18?fm_clampf((en_hole_age-18)/18,0,1):0;}
 for(l=0;l<EN_NGAL;l++){
  for(int p=0;p<8;p++)en_planet_flash[l][p]*=fm_maxf(0,1-dt*1.3f);
  en_spin[l]+=dt*spin_speed[l]*1.5f;en_spin[l]-=fm_floorf(en_spin[l]);collision_wait[l]=fm_maxf(0,collision_wait[l]-dt);
  en_shock[l]*=fm_maxf(0,1-dt*0.12f);
  if(en_death[l]>0){if(!(en_grb_mask&(1u<<l))||en_grb_phase==0)en_death[l]=fm_maxf(0,en_death[l]-dt*((par[P_REST]==EN_NIGHTFALL)?.5f:1));
   if(en_death[l]==0){en_field_r[l]=0.97f;en_field_angle[l]+=(float)(l+1)*0.071f;en_field_angle[l]-=fm_floorf(en_field_angle[l]);en_field_x[l]=en_field_r[l]*fm_sin_turns(en_field_angle[l]+.25f);en_field_y[l]=en_field_r[l]*fm_sin_turns(en_field_angle[l]);orbit_seed(l);en_births++;track_birth(l);planets_new(l);en_bloom_blossom[l]*=0.4f;bloom_fx(en_field_r[l]*fm_sin_turns(en_field_angle[l]+0.25f),en_field_r[l]*fm_sin_turns(en_field_angle[l]),0.6f,5,l);}
  }else if(en_life[l]<1)en_life[l]=fm_minf(1,en_life[l]+dt*((par[P_REST]==EN_NIGHTFALL)?0.06f:0.12f));
 }
 if((par[P_REST]==EN_NIGHTFALL))dt*=0.35f;
 float target=event_bars?1.0f:0.0f;
 en_event_mix+=(target-en_event_mix)*fm_minf(1,dt*0.18f);
 if(en_event && !event_bars && en_event_mix<0.01f){en_event=0;en_event_mix=0;en_ripple=0;}
 float pull=(float)par[P_PULL]*0.01f;en_core_turn+=dt*(.055f+.095f*pull);en_core_turn-=fm_floorf(en_core_turn);
 en_core_impact*=fm_maxf(0,1-dt*0.85f);en_flash*=fm_maxf(0,1-dt*2.4f);en_pulse*=fm_maxf(0,1-dt*0.28f);
 for(int w=0;w<3;w++)if(en_wave_power[w]>0){
  en_wave[w]+=dt*0.40f;en_wave_power[w]*=fm_maxf(0,1-dt*0.22f);
  for(l=0;l<EN_NGAL;l++)if(collision_wait[l]<=0&&!(wave_hit[w]&(1u<<l)) && en_wave[w]>=fm_sqrtf((en_field_x[l]-en_wave_x[w])*(en_field_x[l]-en_wave_x[w])+(en_field_y[l]-en_wave_y[w])*(en_field_y[l]-en_wave_y[w]))){
   wave_hit[w]|=(uint8_t)(1u<<l);en_shock[l]=fm_maxf(en_shock[l],fm_maxf(0.4f,en_wave_power[w]));en_pulse=fm_maxf(en_pulse,0.6f);
   wave_degree[l]=(uint8_t)(1+gx_hash(seed^en_field_cycles[l]^(uint32_t)l)%3u);
   en_field_cycles[l]++;field_pending|=(uint8_t)(1u<<l);bowl_pending|=(uint8_t)(1u<<l);
   int nearby=1;for(int g=0;g<EN_NGAL;g++)if(g!=l){float dx=en_field_x[g]-en_field_x[l],dy=en_field_y[g]-en_field_y[l];if(dx*dx+dy*dy<.065f)nearby++;}
   float physical=fm_clampf((en_wave_power[w]-.35f)/.65f,0,1);
   bump_r[l]=fm_minf(.18f,bump_r[l]+(.035f+(nearby>=5?.07f:.006f*nearby))*physical);
   wave_push[l]+=(bump_hold[l]>0?.001f:.004f)*en_wave_power[w]*(bump_hold[l]>0?(en_orbit_rate[l]>0?1:-1):(l&1?1:-1));
  }
  for(int l=0;l<EN_NGAL;l++)for(int p=0;p<en_planets.count[l];p++)if(!(planet_wave_hit[w][l]&(1u<<p))){float x,y;en_planet_position(l,p,&x,&y);float dx=x-en_wave_x[w],dy=y-en_wave_y[w];if(en_wave[w]>=fm_sqrtf(dx*dx+dy*dy)){planet_wave_hit[w][l]|=(uint8_t)(1u<<p);en_planet_flash[l][p]=1;if(en_planet_growth[l][p]<25){en_planet_growth[l][p]++;en_galaxy_growth[l]=fm_minf(0.25f,en_galaxy_growth[l]+0.0015f);}bowl_pending|=(uint8_t)(1u<<l);}}
  for(int o=0;o<12;o++)if(!(object_wave_hit[w]&(1u<<o))){float dx=en_object_x[o]-en_wave_x[w],dy=en_object_y[o]-en_wave_y[w];if(en_wave[w]>=fm_sqrtf(dx*dx+dy*dy)){object_wave_hit[w]|=(uint16_t)(1u<<o);int dest=o%7;bowl_pending|=(uint8_t)(1u<<dest);object_bowl_mask|=(uint8_t)(1u<<dest);object_bowl_x[dest]=en_object_x[o];object_bowl_y[dest]=en_object_y[o];}}
  if(en_wave[w]>2.4f)en_wave_power[w]=0;
 }
 comet_sample+=dt;int sample_tail=comet_sample>=0.14f;if(sample_tail)comet_sample-=0.14f;
 for(int o=0;o<12;o++){
  if(object_wait[o]>0){object_wait[o]=fm_maxf(0,object_wait[o]-dt);if(object_wait[o]==0){en_object_r[o]=1.20f;en_object_a[o]+=0.173f+(float)o*0.009f;en_object_x[o]=en_object_r[o]*fm_sin_turns(en_object_a[o]+0.25f);en_object_y[o]=en_object_r[o]*fm_sin_turns(en_object_a[o]);en_comet_vx[o]=-en_object_y[o]*0.052f-en_object_x[o]*0.018f;en_comet_vy[o]=en_object_x[o]*0.052f-en_object_y[o]*0.018f;for(int j=0;j<10;j++){en_comet_tail[o][j][0]=en_object_x[o];en_comet_tail[o][j][1]=en_object_y[o];}}continue;}
  en_object_life[o]=fm_minf(1,en_object_life[o]+dt*0.65f);
  float rr=en_object_r[o],aa=en_object_a[o];
  if(en_grb_phase==2){
   /* Feeding owns Cartesian position until capture. Orbit metadata is rebuilt
    * only on respawn; never overwrite the previous inward step. */
  }else{
   /* Inertial flight with softened central gravity and gentle orbital drag.
    * Tangential momentum survives: the attractor bends a comet into a slingshot. */
   float x=en_object_x[o],y=en_object_y[o],r2=x*x+y*y;
   float gravity=pull*0.0048f/((r2+0.065f)*fm_sqrtf(r2+0.065f));
   en_comet_vx[o]+=dt*(-x*gravity-en_comet_vx[o]*pull*0.027f);
   en_comet_vy[o]+=dt*(-y*gravity-en_comet_vy[o]*pull*0.027f);
   float speed=fm_sqrtf(en_comet_vx[o]*en_comet_vx[o]+en_comet_vy[o]*en_comet_vy[o]);
   if(speed>0.24f){en_comet_vx[o]*=0.24f/speed;en_comet_vy[o]*=0.24f/speed;}
   x+=dt*en_comet_vx[o];y+=dt*en_comet_vy[o];rr=fm_sqrtf(x*x+y*y);
   en_object_x[o]=x;en_object_y[o]=y;
   if(sample_tail){for(int j=9;j>0;j--){en_comet_tail[o][j][0]=en_comet_tail[o][j-1][0];en_comet_tail[o][j][1]=en_comet_tail[o][j-1][1];}en_comet_tail[o][0][0]=x;en_comet_tail[o][0][1]=y;}
   /* Metadata angle is unneeded for inertial rendering; retain it for respawns. */
  }

  if(rr>1.4f){en_object_life[o]=0;object_wait[o]=0.65f;}
  if(en_grb_phase!=2&&rr<core_edge()){en_objects_captured++;core_impact(o%3==2?0.20f:0.14f);en_object_life[o]=0;object_wait[o]=0.65f;}
  for(l=0;l<EN_NGAL;l++)if(en_grb_phase!=2&&en_life[l]>0.5f && !en_death[l]){float dx=en_object_x[o]-en_field_x[l],dy=en_object_y[o]-en_field_y[l];if(dx*dx+dy*dy<0.011f && collision_wait[l]==0){galaxy_collision(l,en_field_x[l],en_field_y[l]);en_object_life[o]=0;object_wait[o]=0.65f;break;}}
  en_object_r[o]=rr;en_object_a[o]=aa;
 }

 en_field_time+=dt*(0.2f+pull*0.8f);en_attractor_mass*=fm_maxf(0,1-dt*0.22f);
 for(l=0;l<EN_NGAL;l++){
  float r=en_field_r[l],a=en_field_angle[l];r+=dt*bump_r[l];bump_r[l]*=fm_maxf(0,1-dt*.8f);bump_hold[l]=fm_maxf(0,bump_hold[l]-dt);a+=dt*wave_push[l];wave_push[l]*=fm_maxf(0,1-dt*1.15f);
  if(en_death[l]>0|| (en_grb_phase==2&&l==grb_target)){continue;}
  r+=dt*en_shock[l]*0.003f*fm_sin_turns(en_field_time*0.14f+(float)l*0.13f);

  /* Fixed60Hz softened gravity, with tangential momentum and bounded drag. */
  float x=en_field_x[l],y=en_field_y[l],r2=x*x+y*y;
  float grav=(.0012f+.007f*pull)/((r2+.065f)*fm_sqrtf(r2+.065f));
  float drag=.00010f+.0040f*pull*pull*pull;
  orbit_vx[l]+=dt*(-x*grav-orbit_vx[l]*drag);
  orbit_vy[l]+=dt*(-y*grav-orbit_vy[l]*drag);
  if(en_grb_phase==1||en_grb_phase==2){float hx=en_grb_x-x,hy=en_grb_y-y,d2=hx*hx+hy*hy;float hg=.005f/((d2+.035f)*fm_sqrtf(d2+.035f));orbit_vx[l]+=dt*hx*hg;orbit_vy[l]+=dt*hy*hg;}
  float inv=1/fm_maxf(.04f,fm_sqrtf(r2));
  orbit_vx[l]+=dt*(x*inv*bump_r[l]-y*wave_push[l]);
  orbit_vy[l]+=dt*(y*inv*bump_r[l]+x*wave_push[l]);
  float speed=fm_sqrtf(orbit_vx[l]*orbit_vx[l]+orbit_vy[l]*orbit_vy[l]);
  if(speed>.24f){orbit_vx[l]*=.24f/speed;orbit_vy[l]*=.24f/speed;}
  x+=dt*orbit_vx[l];y+=dt*orbit_vy[l];r=fm_sqrtf(x*x+y*y);
  if(r>1.15f){float outward=(x*orbit_vx[l]+y*orbit_vy[l])/r;if(outward>0){orbit_vx[l]-=x/r*outward*.08f;orbit_vy[l]-=y/r*outward*.08f;}}
  en_field_r[l]=r;en_field_angle[l]=orbit_turn(x,y);en_field_x[l]=x;en_field_y[l]=y;
  if(l!=grb_target&&en_life[l]>=.99f&&r<core_edge())galaxy_capture(l);
 }
 for(l=0;l<EN_NGAL;l++)for(int g=l+1;g<EN_NGAL;g++){
  float dx=en_field_x[l]-en_field_x[g],dy=en_field_y[l]-en_field_y[g],d2=dx*dx+dy*dy;
  float reach=(.085f+(float)(l%3)*.006f)*(1+en_galaxy_growth[l]+en_bloom_blossom[l]*.12f)+(.085f+(float)(g%3)*.006f)*(1+en_galaxy_growth[g]+en_bloom_blossom[g]*.12f);
  if(!(en_bloom_seen&(1u<<l))||!(en_bloom_seen&(1u<<g))||en_life[l]<.8f||en_life[g]<.8f||en_death[l]||en_death[g]||d2>(reach+.05f)*(reach+.05f)){pair_touch[l]&=(uint8_t)~(1u<<g);continue;}
  if(d2<reach*reach&&collision_wait[l]<=0&&collision_wait[g]<=0&&!(pair_touch[l]&(1u<<g))){
   pair_touch[l]|=(uint8_t)(1u<<g);en_bumps++;
   unsigned slot=discharge_next++&3u;en_discharge[slot].a=l;en_discharge[slot].b=g;en_discharge[slot].age=.65f;
   uint32_t h=gx_hash(seed^en_bumps*97u^(uint32_t)(l*8+g));
   if(bump_hold[l]<=5.5f){orbit_vx[l]*=-.65f;orbit_vy[l]*=-.65f;}
   if(bump_hold[l]<=5.5f)en_orbit_rate[l]=(en_orbit_rate[l]>0?-1:1)*(.45f+(float)(h%61u)*.01f);
   if(bump_hold[g]<=5.5f){orbit_vx[g]*=-.65f;orbit_vy[g]*=-.65f;}
   if(bump_hold[g]<=5.5f)en_orbit_rate[g]=(en_orbit_rate[g]>0?-1:1)*(.45f+(float)((h>>8)%61u)*.01f);
   float inv=1/fm_maxf(.025f,fm_sqrtf(d2));dx*=inv;dy*=inv;
   bump_r[l]+=.018f*(dx*en_field_x[l]+dy*en_field_y[l])/fm_maxf(.05f,en_field_r[l]);
   bump_r[g]-=.018f*(dx*en_field_x[g]+dy*en_field_y[g])/fm_maxf(.05f,en_field_r[g]);
   bump_r[l]=fm_clampf(bump_r[l],-.18f,.18f);bump_r[g]=fm_clampf(bump_r[g],-.18f,.18f);bump_hold[l]=bump_hold[g]=6;
   galaxy_collision(l,en_field_x[l],en_field_y[l]);galaxy_collision(g,en_field_x[g],en_field_y[g]);
  }
 }
 /* One rare event, two or three victims; simulation stays on the audio clock. */
 if(!en_grb_phase){grb_wait-=dt;if(grb_wait<=0)grb_begin();}
 else{
  en_grb_age+=dt;
  if(en_grb_phase==1&&en_grb_age>=6){en_grb_phase=2;en_grb_age=0;grb_feed_age=0;}
  if(en_grb_phase==2){
   if(grb_target<0){float best=100;for(int q=0;q<EN_NGAL;q++)if(!en_death[q]&&!(en_grb_mask&(1u<<q))){float dx=en_field_x[q]-en_grb_x,dy=en_field_y[q]-en_grb_y,d=dx*dx+dy*dy;if(d<best){best=d;grb_target=q;}}grb_feed_age=0;if(grb_target>=0)grb_start_life=en_life[grb_target];}
   if(grb_target>=0){int q=grb_target;grb_feed_age+=dt;float hx0=en_grb_x-en_field_x[q],hy0=en_grb_y-en_field_y[q],d0=fm_sqrtf(hx0*hx0+hy0*hy0);
    /* A damped local spiral retains velocity; no position lerp/teleport. */
    float inv0=1/fm_maxf(.025f,d0),speed=fm_minf(.25f,.045f+d0*.38f);
    float radial=speed,swirl=.035f*fm_minf(1,d0/.12f)*(q&1?1:-1);
    float vx=hx0*inv0*radial-hy0*inv0*swirl,vy=hy0*inv0*radial+hx0*inv0*swirl;
    float smooth=fm_minf(1,dt*2.5f);orbit_vx[q]+=(vx-orbit_vx[q])*smooth;orbit_vy[q]+=(vy-orbit_vy[q])*smooth;
    float x=en_field_x[q]+dt*orbit_vx[q],y=en_field_y[q]+dt*orbit_vy[q];
    en_field_x[q]=x;en_field_y[q]=y;en_field_r[q]=fm_sqrtf(x*x+y*y);float ay=fm_fabsf(y)+.000001f,r,angle;if(x>=0){r=(x-ay)/(x+ay);angle=.125f;}else{r=(x+ay)/(ay-x);angle=.375f;}angle+=(.031244f*r*r-.156242f)*r;if(y<0)angle=-angle;en_field_angle[q]=angle<0?angle+1:angle;
    /* Keep the victim visible through the inward travel; shrink/fade
     * only near the core rather than disappearing halfway there. */
    float hx=x-en_grb_x,hy=y-en_grb_y;float core_distance=fm_sqrtf(hx*hx+hy*hy);
    en_life[q]=grb_start_life*fm_clampf(core_distance/.065f,0,1);
    if(core_distance<.03f&&grb_feed_age>.8f){en_grb_mask|=(uint8_t)(1u<<q);en_grb_count++;en_captures++;en_field_cycles[q]++;en_death[q]=6;en_life[q]=0;track_consume(q);bloom_fx(en_grb_x,en_grb_y,.75f,6,q);grb_target=-1;}
   }
   for(int o=0;o<12;o++)if(!object_wait[o]){float dx=en_grb_x-en_object_x[o],dy=en_grb_y-en_object_y[o],d=dx*dx+dy*dy;float k=fm_minf(.35f,dt*(.8f+1/(.1f+d)));en_object_x[o]+=dx*k;en_object_y[o]+=dy*k;en_object_r[o]=fm_sqrtf(en_object_x[o]*en_object_x[o]+en_object_y[o]*en_object_y[o]);if(d<.0064f){object_wait[o]=24;en_object_life[o]=0;en_objects_captured++;}}
   if(en_grb_count>=en_grb_goal||(en_grb_age>22&&grb_target<0)){en_grb_phase=3;en_grb_age=0;en_grb_serial++;cosmic_bloom=1;{unsigned previous=(unsigned)en_reference();unsigned next=(previous+1u+gx_hash(seed^en_grb_serial^clock_samples)%7u)%8u;if(!par[P_REFERENCE])auto_reference=(int8_t)next;}bloom_fx(en_grb_x,en_grb_y,1,7,0);burst_at(en_grb_x,en_grb_y,1);core_pending=fm_maxf(core_pending,.5f);}
  }
  if(en_grb_phase==3&&en_grb_age>=12){en_grb_phase=0;grb_wait=300+(float)(gx_hash(seed^en_captures)%301u);for(int q=0;q<EN_NGAL;q++)if(en_grb_mask&(1u<<q))en_death[q]=.8f;en_grb_mask=0;}
 }

 en_grb_victim=(int8_t)grb_target;
 for(int l=0;l<EN_NGAL;l++){float f=(en_death[l]>0?0:ease(en_life[l]));if(l!=grb_target)f*=ease((en_field_r[l]-core_edge())/.025f);en_presence[l]=f;}
}

#define QN 448u
typedef struct{int16_t c,a;int32_t b;} command_t;
static command_t cq[QN];static volatile uint32_t qr,qw;
enum {C_SET,C_SEED,C_REGEN,C_PLAY,C_FREEZE,C_TOUCH,C_PANIC,C_MUTE,C_CLOCK,C_STRIKE,C_COSMIC,C_PLANET,C_PLANETS_RESTORE,C_BLOOM,C_SYNC,C_ASSIST,C_STRUM,C_DIAGNOSTIC,C_INTENTION,C_KEY_OCTAVE,C_QUALITY,C_DRUMPARTS,C_MIDI_DROP,C_KEY_RELEASE,C_CORE_STAGE,C_USER_ASSIGN,C_USER_HIT,C_AIR,C_AIR_FILTER};
unsigned en_command_room(void){uint32_t used=qw-qr;return used<QN?QN-used:0;}
static int context_index(int p);
static int bounded(int,int,int);
static volatile int16_t control_chosen[7];static volatile uint16_t control_serial[7];static uint16_t control_seen[7],control_applied_serial[3],control_event_serial[3];
volatile uint32_t en_control_apply_samples[3],en_control_event_samples[3],en_control_chosen_samples[3];
static void control_event(unsigned ci){if(ci<3&&control_applied_serial[ci]==control_serial[ci]&&control_event_serial[ci]!=control_serial[ci]){en_control_event_samples[ci]=clock_samples;control_event_serial[ci]=control_serial[ci];}}
unsigned en_control_latency(unsigned ci,unsigned event){if(ci>=3||control_applied_serial[ci]!=control_serial[ci]||(event&&control_event_serial[ci]!=control_serial[ci]))return ~0u;uint32_t end=event?en_control_event_samples[ci]:en_control_apply_samples[ci];uint32_t delta=end-en_control_chosen_samples[ci];return (delta/44100u)*1000u+(delta%44100u)*1000u/44100u;}

volatile uint32_t en_queue_drops,en_clock_coalesced;
static void post(int c,int a,int32_t b){uint32_t w=qw;if(w-qr>=QN){en_queue_drops++;return;}cq[w%QN]=(command_t){(int16_t)c,(int16_t)a,b};BARRIER();qw=w+1;}

volatile int8_t en_key_octave;
void en_core_restore(uint32_t stage){post(C_CORE_STAGE,0,(int32_t)stage);}
void en_key_release(int key){if(key>=-12&&key<=14)post(C_KEY_RELEASE,0,key);}
void en_keyboard_octave(int octave){post(C_KEY_OCTAVE,0,octave<-2?-2:octave>2?2:octave);}
void en_intention(int intention){post(C_INTENTION,0,intention<0?0:intention>9?9:intention);}
void en_diagnostic(int mode){post(C_DIAGNOSTIC,0,mode<0?0:mode>3?3:mode);}
void en_external_drop(unsigned source){if(source<3)post(C_MIDI_DROP,0,(int)source);}
void en_quality(int high){(void)high;post(C_QUALITY,0,44100);}
void en_drum_parts(unsigned mask){post(C_DRUMPARTS,0,(int)(mask&31));}
void en_strum(int ms){post(C_STRUM,0,ms);}
void en_assist(int mode,int chord){post(C_ASSIST,mode,chord);}
void en_sync(int role){if(!EN_BLE&&role==2)role=0;if((unsigned)role<4u)post(C_SYNC,0,role);}
static unsigned air_manual_track,air_manual_cut,air_manual_res;
void en_air_filter(unsigned track,unsigned cut,unsigned res){
 if(track<8&&cut<=100&&res<=100)post(C_AIR_FILTER,track,(int)(cut*128+res));
}
unsigned en_air_applied_percent(unsigned track,unsigned destination){
 if(track>=8||destination>=6)return 0;
 if(destination>=4){
  uint32_t readout=air_filter_readout;
  if((readout>>16)!=track+1)return 0;
  return (readout>>(8*(destination-4)))&255u;
 }
 return (unsigned)(air_current[track][destination]*100.0f+.5f);
}
void en_air_control(unsigned track,unsigned dest,unsigned v){if(track<8&&dest<6&&(track!=EN_DRUM_TRACK||dest!=3))post(C_AIR,track*6+dest,(int)(v>32767?32767:v));}
void en_set(int p,int v){if(p==P_SLEEP_TIMER)v=0;if((unsigned)p>=P_NPARAMS)return;int ci=context_index(p);if(ci>=0){const en_param_t *info=en_param_info(p);v=bounded(v,info->lo,info->hi);if(p==P_STRUM&&v>0)v=bounded(v,80,200);control_chosen[ci]=(int16_t)v;if(ci<3)en_control_chosen_samples[ci]=clock_samples;BARRIER();control_serial[ci]++;}else post(C_SET,p,v);}void en_seed(uint32_t s){post(C_SEED,0,(int32_t)s);}
void en_planet_set(int l,int p,int f,int v){if((unsigned)l<8&&(unsigned)p<8&&(unsigned)f<4)post(C_PLANET,(l*8+p)*4+f,v);}
void en_planets_restore(const en_planets_t *p){restore_planets=*p;post(C_PLANETS_RESTORE,0,0);}
void en_cosmic(int type,int layer){if(type>=1&&type<=7)post(C_COSMIC,type,(unsigned)layer%EN_NGAL);}
void en_regrow(void){post(C_REGEN,0,0);}void en_play(int x){post(C_PLAY,0,x);}void en_freeze(int x){post(C_FREEZE,0,x);}
void en_touch(void){post(C_TOUCH,0,0);}void en_panic(void){post(C_PANIC,0,0);}void en_mute(int x){post(C_MUTE,0,x);}
/* The formerly unused pool word carries a tagged visualizer preference without
 * enlarging the full 3840-byte save. Legacy words default to Bloom Watch. */
void en_pool(uint32_t p){post(C_BLOOM,0,(int32_t)p);}
void en_bloom_aim(int l){if((unsigned)l<EN_NGAL)post(C_BLOOM,1,l);}
void en_bloom_launch(void){post(C_BLOOM,2,0);}void en_clock(int m){post(C_CLOCK,m,0);}void en_strike(int l,int key){if((unsigned)l<EN_NGAL)post(C_STRIKE,l,key);}
static int bounded(int x,int lo,int hi){return x<lo?lo:x>hi?hi:x;}
const en_param_t *en_param_info(int p){
 if(p>=P_QUANT_MODE&&p<P_NPARAMS){static const en_param_t next[10]={{"Timing",0,2,0},{"Grid",0,6,2},{"Attraction",0,100,55},{"Swing",0,40,0},{"Chord playback",0,2,0},{"Context",0,1,1},{"Soundscapes",0,2,0},{"Environment",0,5,0},{"Ambience level",0,100,20},{"Event density",0,100,25}};return &next[p-P_QUANT_MODE];}
 if(p>=P_ROOM_DECAY&&p<=P_ECHO_GRID){static const en_param_t space[6]={{"Room decay",0,100,70},{"Pre-delay ms",0,16,15},{"Room damping",0,100,55},{"Diffusion motion",0,100,20},{"Echo feedback",0,70,32},{"Echo grid",0,3,2}};return &space[p-P_ROOM_DECAY];}
 static const en_param_t glide={"Glide ms",0,150,0},vib={"Vibrato cents",0,25,0},rate={"Vibrato Hz",1,8,5},delay={"Vibrato delay",0,100,40},memory={"Motif mode",0,5,1};
 if(p==P_MOTIF_VARIATION){static const en_param_t variation={"Variation",0,3,1};return &variation;}
 if(p>=P_STRUM_MODE&&p<=P_STRUM_DYN){static const en_param_t strum[4]={{"Strum memory",0,3,0},{"Direction",0,3,0},{"Human feel",0,100,50},{"Dynamics",0,100,50}};return &strum[p-P_STRUM_MODE];}
 if(p==P_GLIDE)return &glide;if(p==P_VIBRATO)return &vib;if(p==P_VIB_RATE)return &rate;if(p==P_VIB_DELAY)return &delay;if(p==P_MEMORY)return &memory;
 static const en_param_t global[24]={{"Anchor",0,EN_NFREQ-1,6},{"Tempo",24,100,48},{"Evolve",0,100,28},{"Overall Activity",0,100,72},{"Space",0,100,38},{"Echo",0,100,22},{"Width",0,100,80},{"Pull",0,100,60},{"Scale",0,7,0},{"Arrange",0,1,1},{"Scene bars",4,64,16},{"Scene",0,7,0},{"Weather",100,100,100},{"Galaxy",0,EN_NGAL-1,0},{"Theme",0,3,0},{"Output route",0,2,2},
 {"Scene mask",0,255,91},{"Scene mask",0,255,127},{"Scene mask",0,255,239},{"Scene mask",0,255,89},{"Scene mask",0,255,243},{"Scene mask",0,255,255},{"Scene mask",0,255,71},{"Scene mask",0,255,89}};
 static const en_param_t layer[16]={{"Level",0,100,65},{"Sound",0,EN_PATCHES-1,0},{"Track Density",0,100,60},{"Steps",0,2,2},{"Rate",0,3,1},{"Gate",10,200,120},{"Pan",-100,100,0},{"Space send",0,100,50},{"Echo send",0,100,20},{"Mute",0,1,0},{"Solo",0,1,0},{"Variation",0,127,0},{"Attack",0,100,45},{"Release",0,100,65},{"Colour",0,100,50},{"Register",-2,2,0}};
 static const en_param_t rest={"Music mode",0,3,0},timer={"Reserved",0,4,0},text={"Text size",0,2,1},drums={"Drums",0,2,0},bass={"Bass part",0,8,0},kit={"Drum kit",0,EN_KITS-1,0},drumroom={"Drum reverb",0,100,62};
 if(p>=P_DRUM_LEVEL&&p<=P_FPS){static const en_param_t extra[11]={{"Drum level",0,100,65},{"Drum Hits",0,100,62},{"Drum echo",0,100,35},{"Drum damping",0,100,75},{"Kit / FX mode",0,1,1},{"Display",0,5,4},{"Chord mode",0,6,0},{"Strum gap",0,200,0},{"Auto level",0,1,1},{"Auto duck",0,1,1},{"FPS overlay",0,1,0}};return &extra[p-P_DRUM_LEVEL];}
 if(p==P_BASS_STRUM){static const en_param_t bs={"Bass strum",-2,2,0};return &bs;}
 if(p==P_REFERENCE){static const en_param_t ref={"Reference",0,9,0};return &ref;}
 if(p==P_KIT)return &kit;if(p==P_DRUM_ROOM)return &drumroom;if(p==P_DRUMS)return &drums;if(p==P_BASS)return &bass;if(p==P_TEXT)return &text;if(p==P_REST)return &rest;if(p==P_SLEEP_TIMER)return &timer;
 if(p>=P_LAYER && p<P_REST)return &layer[(p-P_LAYER)%16];return &global[(unsigned)p<24?p:0];
}
static char *num(int n,char *b){char t[12];int k=0;if(n<0){*b++='-';n=-n;}do{t[k++]=(char)('0'+n%10);n/=10;}while(n);while(k)*b++=t[--k];*b=0;return b;}
static void strcopy(char *b,const char *a){int k=0;while(a[k]&&k<15){b[k]=a[k];k++;}b[k]=0;}
void en_param_text(int p,int v,char *b){
 if(p==P_ECHO_GRID){strcopy(b,(const char *const[]){"1/16","Dotted 1/16","1/8","1/4"}[bounded(v,0,3)]);return;}
 int l=p>=P_LAYER&&p<P_REST?(p-P_LAYER)%16:-1;
 if(p==P_QUANT_MODE){strcopy(b,(const char *const[]){"Off","Smart","Strict"}[bounded(v,0,2)]);return;}
 if(p==P_QUANT_GRID){strcopy(b,(const char *const[]){"1/4","1/8","1/16","1/32","1/8 triplet","1/16 triplet","1/4 triplet"}[bounded(v,0,6)]);return;}
 if(p==P_CHORD_PLAYBACK){strcopy(b,(const char *const[]){"Auto","Together","Strum"}[bounded(v,0,2)]);return;}
 if(p==P_CONTEXT_AUTO){strcopy(b,v?"Auto":"Custom");return;}
 if(p==P_SCAPE_MODE){strcopy(b,(const char *const[]){"Off","Auto","Manual"}[bounded(v,0,2)]);return;}
 if(p==P_SCAPE_BANK){strcopy(b,(const char *const[]){"Florida Night","Wetland Frogs","Birds / Leaves","Gentle Rain","Soft Static","Play: unavailable"}[bounded(v,0,5)]);return;}
 if(p==P_GLIDE||p==P_VIBRATO||p==P_VIB_RATE){num(v,b);}
 else if(p==P_STRUM_MODE)strcopy(b,(const char *const[]){"Classic","Remember","Call + Answer","Evolve"}[bounded(v,0,3)]);
 else if(p==P_STRUM_DIR)strcopy(b,(const char *const[]){"Auto","Up","Down","Alternate"}[bounded(v,0,3)]);
 else if(p==P_MEMORY)strcopy(b,(const char *const[]){"Off","Motif Remember","Learn + Answer","Motif Evolution","Call & Response","Variation / Repeat"}[bounded(v,0,5)]);
 else if(p==P_MOTIF_VARIATION)strcopy(b,(const char *const[]){"Repeat","Subtle","Expressive","Fresh"}[bounded(v,0,3)]);
 else if(p==P_VIB_DELAY){num(v*10,b);}
 else if(p==P_BASS_STRUM)strcopy(b,(const char *const[]){"Off","Octaves","Root + Fifth"}[bounded(v,0,2)]);
 else if(p==P_DRUM_HITS){char *e=num(bounded(v,0,100),b);*e++='%';*e=0;}
 else if(p==P_REFERENCE)strcopy(b,REFERENCE_NAME[bounded(v,0,9)]);
 else if(p==P_KIT)strcopy(b,KIT_NAME[bounded(v,0,EN_KITS-1)]);
 else if(p==P_DRUM_AUTO)strcopy(b,v?"Auto":"Manual");
 else if(p==P_AUTO_LEVEL||p==P_AUTO_DUCK||p==P_FPS)strcopy(b,v?"On":"Off");
 else if(p==P_CHORD)strcopy(b,(const char *const[]){"Single","Triad","Sus","Seventh","Split Triad","Split Sus","Split Seventh"}[bounded(v,0,6)]);
 else if(p==P_POWER){strcopy(b,(const char *const[]){"HD / 20 FPS","HD / 20 FPS","Unlock FPS","HD / 20 FPS","HD / 20 FPS","Saver / 10 FPS"}[bounded(v,0,5)]);}
 else if(p==P_STRUM){if(!v)strcopy(b,"Together");else num(v,b);}
 else if(p==P_DRUMS)strcopy(b,(const char *const[]){"Off","Sparse","Groove"}[bounded(v,0,2)]);
 else if(p==P_BASS)strcopy(b,BASS_NAME[bounded(v,0,8)]);
 else if(p==P_TEXT)strcopy(b,(const char *const[]){"Small","Medium","Large"}[bounded(v,0,2)]);
 else if(p==P_REST)strcopy(b,(const char *const[]){"Ambience","Nightfall","Cinematic","Neoclassical"}[bounded(v,0,3)]);
 else if(p==P_SLEEP_TIMER){if(!v)strcopy(b,"Off");else{char *e=num((const int[]){0,30,60,120,300}[v],b);*e++='m';*e=0;}}
 else if(p==P_ANCHOR){char *e=num((int)HEALING_HZ[bounded(v,0,EN_NFREQ-1)],b);*e++='H';*e++='z';*e=0;}
 else if(p==P_OUTPUT)strcopy(b,v==2?"Auto":v?"Speaker":"External");
 else if(p==P_TUNING)strcopy(b,en_scale_name(v));
 else if(p==P_THEME)strcopy(b,THEME_NAME[bounded(v,0,3)]);
 else if(p==P_AUTO || l==L_MUTE || l==L_SOLO)strcopy(b,v?"On":"Off");
 else if(p==P_SCENE)strcopy(b,SCENE_NAME[v&7]);
 else if(l==L_VOICE)strcopy(b,VOICE_NAME[bounded(v,0,EN_PATCHES-1)]);
 else if(l==L_STEPS)num(16<<bounded(v,0,2),b);
 else if(l==L_RATE)strcopy(b,(const char *const[]){"1x","1/2","1/4","1/8"}[v&3]);
 else num(v,b);
}
const char *en_piece_name(int i){return INTENT_NAME[(unsigned)i<10?i:0];}
static int patch_busy(const int16_t *p,int l,int patch){for(int q=0;q<EN_NGAL;q++)if(q!=l&&p[LP(q,L_VOICE)]==patch)return 1;return 0;}
int en_style_scale(int intention,int ref){
 static const uint8_t mood[10]={3,1,2,7,5,3,0,6,1,4};
 static const uint8_t profile[8]={0,5,2,4,3,6,7,2};
 return ref>=0&&ref<8?profile[ref]:mood[bounded(intention,0,9)];
}
int en_reference_for(int intention,int anchor){return 1+(int)(gx_hash((uint32_t)intention*193u+(uint32_t)anchor*97u)%7u);}
/* Bed / bass / motif / pluck / choir / bells / answering strings. */
static const uint8_t cinematic_family[7][8]={
 {117,119,112,19,38,16,47,35},{116,49,113,21,6,16,0,23},
 {114,115,108,111,109,46,36,110},{121,42,24,28,41,40,25,39},
 {120,112,44,18,32,34,20,4},{118,45,50,37,9,3,11,36},
 {113,110,115,117,23,38,33,43}};
static int patch_pick(const int16_t *p,int l,int old,uint32_t salt){
 if(p[P_STYLE]==EN_NEOCLASSICAL){static const uint8_t chamber[7]={117,115,114,121,119,118,120};return chamber[l];}
 if(role_patch(old))return old;
 static const uint8_t bass[8]={49,6,8,14,47,18,21,23};
 static const uint8_t halo[24]={43,44,47,51,16,18,19,21,22,23,0,4,6,8,12,14,17,20,32,33,34,35,38,40};
 static const uint8_t accent[24]={40,41,42,45,46,48,50,24,28,25,29,26,30,27,31,17,20,22,23,2,5,10,36,39};
 uint32_t h=gx_hash(salt^(uint32_t)(p[P_ANCHOR]+1)*0x85ebca6bu^(uint32_t)p[P_TUNING]*97u^(uint32_t)p[LP(l,L_VARIANT)]);
 int n=l==1?8:24;const uint8_t *pool=l==1?bass:(l==0||l==4||l==6||p[P_ANCHOR]>=8)?halo:accent;
 if(p[P_STYLE]==EN_CINEMATIC){pool=cinematic_family[l];n=8;}
 for(int i=0;i<n;i++){int x=pool[(h+(unsigned)i)%n];if(x!=old&&!patch_busy(p,l,x))return x;}
 for(int x=0;x<EN_PATCHES;x++)if(x!=old&&!patch_busy(p,l,x))return x;return old;
}
/* Preserve the full factory bank. Rotation happens only while arranging a
 * new phrase; live voices retain their captured patch and locked lanes win. */
static uint8_t patch_use[EN_PATCHES];static uint32_t patch_arrangements;
static int patch_eligible(const int16_t *p,int lane,int patch,int ref,int intention){
 if((unsigned)patch>=EN_PATCHES)return 0;
 /* Quiet Water spaces its pad onsets25s apart; short winds/keys belong to other roles. */
 if(lane==0&&ref==7&&patch<EN_USER_PATCH_BASE){const role_patch_t *r=role_patch(patch);if((r&&r->hold<20)||patch==40)return 0;}
 if(p[P_STYLE]==EN_NEOCLASSICAL&&patch<108&&!(patch>=24&&patch<=31))return 0;
 if(patch>=52&&patch<108)return (patch-52)%7==lane;
 if(patch>=126)return p[P_STYLE]==EN_AMBIENCE&&p[P_REFERENCE]!=9&&ref==7&&((lane==0&&patch==128)||lane==5||lane==6);
 if(patch>=122)return p[P_STYLE]!=EN_NEOCLASSICAL&&(lane==0||lane==4)&&(p[P_STYLE]==EN_CINEMATIC||intention==5||intention==9||p[P_ANCHOR]==6||p[P_ANCHOR]==12);
 if(patch>=108&&patch<=111)return lane==2||lane==4||lane==6;
 if(patch==114||patch==115||patch==118||patch==121)return lane==1||lane==2||lane==3||lane==5||lane==6;
 if(patch>=112)return lane==0||lane==1||lane==4||lane==6;
 int type=patch_type(patch);if(lane==1)return type==0||type==6;
 if(lane==0||lane==4)return type==0||type==4||type==6;
 return type!=6;
}
static unsigned patch_weight(const int16_t *p,int patch,int ref,int intention){unsigned w=1u+24u/(1u+patch_use[patch]);if(patch>=52&&patch<108&&(patch-52)/7==ref)w*=3;if(patch>=122&&(intention==5||intention==9))w*=2;if(patch>=126&&(p[P_ANCHOR]==0||p[P_ANCHOR]==4||p[P_ANCHOR]==6))w*=2;return w;}
static void patch_rotate(int16_t *p,int ref,int intention){unsigned serial=++patch_arrangements;
 for(int lane=0;lane<7;lane++){if(en_patch_locks&(1u<<lane))continue;if((p[P_STYLE]==EN_NEOCLASSICAL||p[P_REFERENCE]==9)&&(lane==1||lane==2||lane==3))continue;
  uint32_t h=gx_hash(serial*0x9e3779b9u+(unsigned)p[P_ANCHOR]*193u+(unsigned)p[P_STYLE]*97u+(unsigned)p[P_REFERENCE]*71u+(unsigned)lane*31u);if(h%3u)continue;
  unsigned total=0;for(int patch=0;patch<EN_PATCHES;patch++)if(patch_eligible(p,lane,patch,ref,intention)&&!patch_busy(p,lane,patch))total+=patch_weight(p,patch,ref,intention);
  if(!total)continue;unsigned pick=gx_hash(h^0x50415443u)%total;for(int patch=0;patch<EN_PATCHES;patch++)if(patch_eligible(p,lane,patch,ref,intention)&&!patch_busy(p,lane,patch)){unsigned weight=patch_weight(p,patch,ref,intention);if(pick<weight){p[LP(lane,L_VOICE)]=(int16_t)patch;break;}pick-=weight;}
 }
 for(int lane=0;lane<7;lane++){unsigned patch=p[LP(lane,L_VOICE)];if(patch<EN_PATCHES){if(patch_use[patch]==255)for(int q=0;q<EN_PATCHES;q++)patch_use[q]/=2;patch_use[patch]++;}}
}
/* Curated role families: sustained bed, bass anchor, keys, plucked accents,
 * choir, bells and answering pad. Profiles reorder timbres, never bass pitch. */
void en_arrange(int16_t *p,int intention,int ref){
 static const uint8_t family[7][8]={
  {43,38,16,18,19,21,47,35},{49,6,8,14,16,18,21,23},
  {46,36,2,10,17,20,48,37},{40,41,42,24,28,25,29,39},
  {44,51,4,12,20,22,32,34},{45,50,37,1,9,3,11,36},
  {47,51,35,19,23,38,33,43}};
 static const uint8_t kit[8][4]={{11,12,8,3},{12,1,11,10},{11,8,4,12},{4,3,12,11},{12,11,9,5},{11,4,8,12},{12,10,11,8},{11,3,12,8}};
 static const uint8_t rate[7]={3,2,2,2,3,3,3};
 static const uint8_t density[7]={44,60,38,32,40,24,36};
 int dedicated9=ref==8||p[P_REFERENCE]==9;int primary9=dedicated9||p[P_REFERENCE]==0;intention=bounded(intention,0,9);ref=bounded(ref,0,7);
 int16_t previous[P_NPARAMS];for(int q=0;q<P_NPARAMS;q++)previous[q]=p[q];
 int old[7];for(int l=0;l<7;l++)old[l]=p[LP(l,L_VOICE)];
 for(int l=0;l<7;l++){
  int start=(intention*3+ref*(l%3+1)+l)%8,x=0;
  for(int n=0;n<8;n++){x=(p[P_STYLE]==EN_CINEMATIC?cinematic_family:family)[l][(start+n)%8];int busy=0;for(int q=0;q<l;q++)busy|=p[LP(q,L_VOICE)]==x;if(!busy&&x!=old[l])break;}
  p[LP(l,L_VOICE)]=(int16_t)(p[P_STYLE]==EN_CINEMATIC?x:ROLE_PATCH_BASE+ref*7+l);
  p[LP(l,L_VARIANT)]=(int16_t)((intention*11+ref*17+l*7)&127);
  p[LP(l,L_RATE)]=(int16_t)bounded(rate[l]+((ref==1||intention==7||p[P_STYLE]==EN_CINEMATIC)?-1:0),0,3);
  p[LP(l,L_DENSITY)]=(int16_t)bounded(density[l]+(intention%5)*3-ref*2,12,80);
  if(l!=1){p[LP(l,L_ATTACK)]=(int16_t)((l==3||l==5)?12+ref*2:58+ref*4);p[LP(l,L_RELEASE)]=(int16_t)bounded(66+ref*4+(intention%3)*4,0,100);}
 }
 if(p[P_STYLE]==EN_NEOCLASSICAL){static const uint8_t bank[7]={117,115,114,121,119,118,120},density[7]={48,42,62,38,34,18,38};for(int l=0;l<7;l++){p[LP(l,L_VOICE)]=!primary9&&l==1?114:!primary9&&l==2?115:bank[l];p[LP(l,L_DENSITY)]=density[l];p[LP(l,L_RATE)]=l==2?0:l==3?1:2;p[LP(l,L_ATTACK)]=(l==2||l==3||l==5)?12:48;p[LP(l,L_RELEASE)]=72;}}
 if(p[P_STYLE]==EN_CINEMATIC&&(intention==5||intention==9)&&(ref==0||ref==4))p[LP(4,L_VOICE)]=122;
 if((intention==5||intention==9)&&(ref==0||ref==4)&&p[P_STYLE]!=EN_NEOCLASSICAL){
  if(p[P_STYLE]==EN_NIGHTFALL)p[LP(4,L_VOICE)]=125;
  else if(p[P_STYLE]==EN_AMBIENCE)p[LP(4,L_VOICE)]=(intention==5?123:124);
 }
 if(ref==7&&!dedicated9&&p[P_STYLE]==EN_AMBIENCE){p[LP(5,L_VOICE)]=126;p[LP(6,L_VOICE)]=127;if(intention==0||intention==9)p[LP(0,L_VOICE)]=128;}
 if(dedicated9&&p[P_STYLE]!=EN_NEOCLASSICAL){p[LP(2,L_VOICE)]=114;p[LP(1,L_VOICE)]=115;p[LP(3,L_VOICE)]=121;}
 int selected=kit[ref][intention%4];if(selected==p[P_KIT])selected=kit[ref][(intention+1)%4];p[P_KIT]=(int16_t)selected;
 /* Percussion role follows the mood even when its master switch is off. */
 p[P_DRUM_HITS]=(int16_t)bounded(22+(intention%5)*7-ref*2,8,70);
 p[P_DRUM_ROOM]=(int16_t)bounded(30+ref*5,0,100);
 if(p[P_STYLE]==EN_NEOCLASSICAL){p[P_KIT]=12;p[P_DRUM_HITS]=8;}
 if(p[P_DRUM_AUTO]){p[P_KIT]=(int16_t)(p[P_STYLE]==EN_NEOCLASSICAL?(intention%3==0?26:22):p[P_STYLE]==EN_NIGHTFALL?24:p[P_STYLE]==EN_CINEMATIC?25:(intention==0||intention==9)?23:p[P_REFERENCE]==9?22:14+ref);}
 if(ref==7&&p[P_STYLE]!=EN_NEOCLASSICAL){for(int l=0;l<7;l++){p[LP(l,L_DENSITY)]=(int16_t)(l==2||l==3||l==5?18:36);p[LP(l,L_RATE)]=3;}p[P_DRUM_HITS]=10;}
 for(int l=0;l<7;l++)if(en_patch_locks&(1u<<l)){for(unsigned q=0;q<16;q++)p[LP(l,q)]=previous[LP(l,q)];}
 patch_rotate(p,ref,intention);
 if(p[P_DRUM_AUTO]&&!(en_patch_locks&128u))p[LP(7,L_VOICE)]=22;

 /* Never let an automatic reassignment take a manually locked timbre. */
 for(int l=0;l<7;l++)if(!(en_patch_locks&(1u<<l))){int busy=0;for(int q=0;q<7;q++)if(q!=l&&((en_patch_locks&(1u<<q))||q<l)&&p[LP(q,L_VOICE)]==p[LP(l,L_VOICE)])busy=1;if(busy)for(int n=0;n<8;n++){int x=(p[P_STYLE]==EN_CINEMATIC?cinematic_family:family)[l][n],used=0;for(int q=0;q<7;q++)if(q!=l&&((en_patch_locks&(1u<<q))||q<l)&&p[LP(q,L_VOICE)]==x)used=1;if(!used){p[LP(l,L_VOICE)]=x;break;}}}
 if(!previous[P_DRUM_AUTO])p[P_KIT]=previous[P_KIT];
 if(p[P_CONTEXT_AUTO]){static const uint8_t tempo[4][8]={{48,58,42,46,54,40,50,44},{36,40,32,35,38,30,36,34},{60,76,52,64,68,56,72,58},{66,72,60,64,68,58,70,62}};static const int8_t mood[10]={-2,0,-4,2,0,-2,-3,4,2,-4};p[P_TEMPO]=bounded((dedicated9&&p[P_STYLE]!=EN_NIGHTFALL?66:tempo[p[P_STYLE]][ref])+mood[intention],24,100);p[P_STRUM]=p[P_STYLE]==EN_NIGHTFALL?180:p[P_STYLE]==EN_NEOCLASSICAL?110:ref==1?90:ref==7?160:130;}
 else{p[P_TEMPO]=previous[P_TEMPO];p[P_STRUM]=previous[P_STRUM];}
}
static void patches_unique(int16_t *p,uint32_t salt){for(int l=0;l<EN_NGAL;l++)for(int q=0;q<l;q++)if(p[LP(l,L_VOICE)]==p[LP(q,L_VOICE)]){p[LP(l,L_VOICE)]=(int16_t)patch_pick(p,l,p[LP(l,L_VOICE)],salt+(unsigned)l);break;}}
static void patch_publish(int l){en_track_patch[l]=(uint8_t)par[LP(l,L_VOICE)];en_track_variant[l]=(uint8_t)par[LP(l,L_VARIANT)];BARRIER();en_track_revision[l]++;}
void en_piece(int i,int16_t *p,uint16_t *pool){
 int k,l; i=bounded(i,0,9);for(k=0;k<P_NPARAMS;k++)p[k]=en_param_info(k)->def;
 p[P_ANCHOR]=(int16_t[]){0,9,2,3,11,6,7,8,11,12}[i];p[P_TEMPO]=(int16_t)(36+i*2);p[P_SCENE]=0;
 for(l=0;l<8;l++){
  p[LP(l,L_LEVEL)]=(int16_t[]){46,56,32,26,48,22,42,28}[l];
  p[LP(l,L_VOICE)]=(int16_t[]){0,6,2,1,4,3,6,7}[l];
  p[LP(l,L_DENSITY)]=(int16_t[]){76,68,30,22,60,16,70,38}[l];
  p[LP(l,L_RATE)]=(int16_t[]){2,1,2,2,2,2,3,0}[l];
  p[LP(l,L_ATTACK)]=(int16_t[]){65,45,18,4,75,8,90,0}[l];
  p[LP(l,L_RELEASE)]=(int16_t[]){80,70,60,85,95,75,100,25}[l];
  p[LP(l,L_PAN)]=(int16_t[]){-16,8,-40,42,-24,56,20,0}[l];
  p[LP(l,L_TUNE)]=(int16_t[]){0,-1,1,1,0,1,0,0}[l];
  p[LP(l,L_STEPS)]=(int16_t)(l%3);p[LP(l,L_VARIANT)]=(i*7+l*3)&127;
  if(l==7){p[LP(l,L_LEVEL)]=32;p[LP(l,L_DENSITY)]=24;p[LP(l,L_RATE)]=3;p[LP(l,L_REVERB)]=42;}
  if(l==1){p[LP(l,L_VOICE)]=0;p[LP(l,L_ATTACK)]=24;p[LP(l,L_RELEASE)]=48;p[LP(l,L_LEVEL)]=78;p[LP(l,L_GATE)]=190;}
 }
 /* Intention recipes are musical moods, never biological or attraction protocols. */
 p[P_DRUMS]=0;
 if(i==0||i==5||i==9){for(int l=0;l<7;l++)p[LP(l,L_VOICE)]+=8;p[P_KIT]=3+i%3;p[P_DRUM_ROOM]=28;}
 p[P_REVERB]=(int16_t[]){57,48,62,55,49,64,60,43,47,66}[i];
 p[P_ECHO]=(int16_t[]){18,27,16,25,24,28,30,22,26,20}[i];
 p[P_DENSITY]=(int16_t[]){60,72,59,68,70,65,69,70,73,58}[i];
 p[P_EVOLVE]=(int16_t[]){20,32,24,35,28,22,30,26,33,18}[i];
 for(l=0;l<7;l++)if(l!=1){p[LP(l,L_RELEASE)]=bounded(p[LP(l,L_RELEASE)]+8,0,100);p[LP(l,L_TONE)]=(int16_t[]){34,48,30,42,45,32,38,52,48,28}[i];}
 for(l=0;l<7;l++)if(l!=1&&((i+l)&1))p[LP(l,L_VOICE)]=(int16_t)(l==0?16+(i%8):16+((i*5+l*3)%16));
 /* Anchor and intention choose the initial reference and per-role patch recipe. */
 p[P_TUNING]=(int16_t)en_style_scale(i,-1);p[P_REFERENCE]=0;p[P_KIT]=(const int16_t[]){8,9,1,3,10,8,10,7,9,3}[i];
 for(int scene=0;scene<8;scene++){unsigned h=gx_hash((unsigned)i*193u+(unsigned)scene*97u+(unsigned)p[P_ANCHOR]);p[P_MASK+scene]=(int16_t)(scene==0?127u:(23u|(h&0x7cu)));}
 for(int lane=0;lane<EN_NGAL;lane++)if(lane!=1)p[LP(lane,L_VOICE)]=(int16_t)patch_pick(p,lane,p[LP(lane,L_VOICE)],gx_hash((uint32_t)i*97u+(uint32_t)p[P_ANCHOR]*193u+(unsigned)lane));
 en_arrange(p,i,en_reference_for(i,p[P_ANCHOR])-1);p[P_GLIDE]=p[P_VIBRATO]=0;p[P_VIB_RATE]=5;p[P_VIB_DELAY]=40;p[P_MEMORY]=1;patches_unique(p,(uint32_t)i*0x9e3779b9u);if(pool)*pool=0xAB5u;
}
#define SN 1024u
static float sint[SN+1];
static inline float sine(float ph){ph-=(float)(int)ph;float x=ph*(float)SN;int i=(int)x;return sint[i]+(sint[i+1]-sint[i])*(x-(float)i);}
/* Oscillator phases are maintained in [0,1); avoid redundant range reduction
 * and a float/integer round-trip for every oscillator sample. FM offsets
 * retain the general sine() path. */
static inline float sine_phase(float ph){float x=ph*(float)SN;int i=(int)x;return sint[i]+(sint[i+1]-sint[i])*(x-(float)i);}
#define DRUM_VOICE_BASE (EN_NGAL*2)
#define LIVE_VOICE_BASE (DRUM_VOICE_BASE+4)
#define NV (LIVE_VOICE_BASE+4)
typedef struct{float phase[3],inc[3],pitch_ratio,glide_k,vib_phase;uint32_t age;float amp,env,attack,release,pan,level,rev,carrier,partial,last,tail,noise_low,noise_mid,mix_gain;uint32_t held;uint8_t on,layer,drum,kit,sfx,patch,strummed,midi_released;uint16_t midi_tag;uint32_t rng;} voice_t;
static voice_t voice[NV];

volatile uint32_t en_shed_count;
volatile uint8_t en_voice_budget=NV,en_active_voices;
/* Audio pressure may reduce optional drawing, never terminate musical notes.
 * The former85% policy repeatedly forced overlap voices into .90 release. */
volatile uint8_t en_graphics_throttle;
volatile uint32_t en_audio_max_us,en_pressure_events;
static uint8_t pressure_history;static uint16_t quiet_halves;
void en_audio_pressure(uint32_t used,uint32_t budget){
 if(!budget)return;
 if(used>en_audio_max_us)en_audio_max_us=used;
 pressure_history=(uint8_t)((pressure_history<<1)|(used>budget-budget/4u));
 if((pressure_history&3u)==3u){if(!en_graphics_throttle)en_pressure_events++;en_graphics_throttle=1;quiet_halves=0;}
 else if(used<budget-budget*35u/100u){if(++quiet_halves>=512u){quiet_halves=0;en_graphics_throttle=0;}}
 else quiet_halves=0;
}
#include "voice_runtime.h"
static float air_filter_low[2],air_filter_band[2],air_filter_mix;
static unsigned air_filter_track;
static uint8_t alternate[8];
static uint32_t field_samples;
/* Anchor-derived consonant ratios. Bass uses octave relatives; acoustic-style
 * drums include broadband noise. Intentions are artistic, not treatment claims. */
float en_bass_root(void){float f=HEALING_HZ[par[P_ANCHOR]];while(f>64)f*=.5f;while(f<32)f*=2;return f;}
static const uint8_t scale_notes[8][7]={
 {0,2,4,5,7,9,11},{0,2,4,4,7,9,9},{0,2,3,5,7,9,10},
 {0,2,4,6,7,9,11},{0,2,3,5,7,8,10},{0,2,4,5,7,9,10},
 {0,3,5,5,7,10,10},{0,2,5,5,7,9,9}};
const char *en_scale_name(int mode){static const char *const names[8]={"Just Major","Major Pent","Dorian","Lydian","Natural Minor","Mixolydian","Minor Pent","Suspended Pent"};return names[(unsigned)mode<8u?mode:0];}
unsigned en_scale_mask(int mode){unsigned mask=0;if((unsigned)mode>=8u)mode=0;for(int i=0;i<7;i++)mask|=1u<<scale_notes[mode][i];return mask;}
static float tone_hz(int layer,int degree){
 static const float ratio[7]={1,1.125f,1.25f,1.3333333333f,1.5f,1.6666666667f,1.875f};
 
 int d=degree%7;if(d<0)d+=7;
 float f=HEALING_HZ[par[P_ANCHOR]]*(par[P_TUNING]==0?ratio[d]:fm_exp2f((float)scale_notes[par[P_TUNING]][d]/12))*fm_exp2f((float)par[LP(layer,L_TUNE)]);
 while(f>963)f*=0.5f;while(f<174)f*=2.0f;
 if(layer==1){float lo=en_bass_root(),hi=lo*2;while(f>=hi)f*=.5f;while(f<lo)f*=2;}
 return f;
}
/* One octave choice per eight-bar phrase; integer shifts preserve tuning.
 * Played notes bypass automatic registers. No additional oscillators. */
static int generated_octave(int layer){
 unsigned phrase=bar/8u,ref=(unsigned)en_reference();
 unsigned h=gx_hash(seed^(phrase+1u)*0x9e3779b9u^(unsigned)layer*193u^(unsigned)mix_intention*977u^ref*7919u);
 int dark=(par[P_REST]==EN_NIGHTFALL)||ref==2||ref==3||ref==5||ref==6;
 if(layer==1)return !dark&&h%5u==0u?1:0;
 if(ref==7&&(layer==2||layer==3||layer==5))return h%3u?1:0;
 if(dark||ref==7)return h%4u? -1:0;
 if(layer==3||layer==5)return h%4u==0u?1:h%4u==1u?-1:0;
 return h%5u==0u?1:h%3u==0u?-1:0;
}
static float generated_register(int layer,float f){
 int octave=generated_octave(layer);if(octave<0)f*=.5f;else if(octave>0)f*=2;
 static const float bass_floor[8]={48,64,40,40,56,36,40,40};
 float floor=bass_floor[en_reference()];if((par[P_REST]==EN_NIGHTFALL))floor=fm_minf(floor,40);
 float lo=layer==1?(par[P_BASS]==8?20:floor):55,hi=layer==1?(par[P_BASS]==8?60:floor*2):((par[P_REST]==EN_NIGHTFALL)||en_reference()==5)?432:963;
 if(layer==1&&par[P_BASS]==8)f*=.5f;while(f>=hi)f*=.5f;while(f<lo)f*=2;if(layer!=1&&(par[P_TUNING]==1||par[P_TUNING]>=6))while(f>528)f*=.5f;
 const role_patch_t *r=role_patch(par[LP(layer,L_VOICE)]);
 if(r&&r->fm&&layer!=1){float melodic_floor=layer==5?261.6256f:layer==3?130.8128f:174.6141f;if(par[P_REST]==EN_NIGHTFALL)melodic_floor*=.75f;while(f<melodic_floor)f*=2;}
 return f;
}
static uint16_t midi_tag;static uint8_t midi_sustain[64];static float midi_bend[64];
static int generated_strike;
static int manual_pitch,manual_octave,manual_keyboard;
int en_perfect_degree(int semitone){
 int mode=par[P_TUNING],note=px8_perfect_snap(semitone+48,48,en_scale_mask(mode)),offset=note-48,oct=offset/12;
 if(offset<0&&offset%12)oct--;int pc=offset-oct*12,d=0;
 while(d<6&&scale_notes[mode][d]<pc)d++;return oct*7+d;
}
static int safe_degree(int d){d%=7;if(d<0)d+=7;int mode=par[P_TUNING];while(d>0&&scale_notes[mode][d]==scale_notes[mode][d-1])d--;return d;}

static int shared_degree(int layer,int degree,int impact){
 int d=degree%7;if(d<0)d+=7;int root=current_harmony();
 if(layer==0&&!impact&&par[LP(layer,L_VOICE)]<24)return safe_degree(root);
 if(impact||layer==1||layer==4||layer==6||!(sequence_step&3u)){
  int best=safe_degree(root),dist=8;for(int k=0;k<3;k++){int q=safe_degree(root+k*2),delta=d-q;if(delta<0)delta=-delta;if(delta>3)delta=7-delta;if(delta<dist){best=q;dist=delta;}}return best;
 }
 return safe_degree(d);
}
static int audible(int l);
static uint8_t motif_memory[7][64];
/* Fixed storage and phrase-boundary decisions; no allocations or extra voices.
 * Transform from the signature each time, never accumulate random drift. */
static uint8_t motif_active, motif_variation_active;
static int8_t motif_caller=-1,motif_responder=-1,motif_call[32];
static uint16_t motif_caller_epoch,motif_responder_epoch;
static void motif_boundary(void){
 uint8_t previous=motif_active;motif_active=(uint8_t)par[P_MEMORY];motif_variation_active=(uint8_t)par[P_MOTIF_VARIATION];
 if(!(sequence_step&63u)){
  motif_caller=motif_responder=-1;for(int q=0;q<32;q++)motif_call[q]=-1;
  static const uint8_t candidates[4]={2,5,3,6};
  for(int q=0;q<4;q++){int l=candidates[(q+sequence_step/64u)%4u];
   if(audible(l)&&!en_death[l]&&en_life[l]>=.35f&&par[LP(l,L_DENSITY)]&&par[LP(l,L_LEVEL)]){
    if(motif_caller<0)motif_caller=(int8_t)l;else{motif_responder=(int8_t)l;break;}
   }
  }
  if(motif_caller>=0)motif_caller_epoch=en_track_revision[motif_caller];
  if(motif_responder>=0)motif_responder_epoch=en_track_revision[motif_responder];
 }
 if(motif_active<EN_MOTIF_EVOLVE){if(previous>=EN_MOTIF_EVOLVE)for(int l=0;l<7;l++)if(!en_death[l])for(unsigned q=0;q<gx_len(par[LP(l,L_STEPS)]);q++)en_note_grid[l][q]=motif_memory[l][q];return;}
 unsigned phrase=sequence_step/64u;
 for(int l=0;l<7;l++)if(!en_death[l]){
  unsigned len=gx_len(par[LP(l,L_STEPS)]);
  for(unsigned q=0;q<len;q++)en_note_grid[l][q]=motif_memory[l][q];
  if(l==0||l==1||l==4||motif_active==EN_MOTIF_CONVERSE)continue;
  int strength=motif_active==EN_MOTIF_EVOLVE?(par[P_EVOLVE]?1+(par[P_EVOLVE]-1)/34:0):motif_variation_active;
  if(!strength||!(phrase&7u))continue; /* familiar return every eighth phrase */
  unsigned changes=strength==1?1u:strength==2?2u:4u;
  for(unsigned n=0;n<changes;n++){
   unsigned q=1u+gx_hash(seed^(unsigned)l*97u^(phrase/2u)*193u^n)%(len-1u);
   if(!(q&3u))continue; /* preserve signature accents/cadence */
   int token=motif_memory[l][q];if(!token)continue; /* no density increase */
   int delta=(gx_hash(seed^phrase^q)&1u)?1:-1;
   if(strength==3&&n==3)en_note_grid[l][q]=0;
   else en_note_grid[l][q]=(uint8_t)(1+safe_degree(token-1+delta));
  }
 }
}

static void rebuild(int layer){gx_plan(layer,seed^(uint32_t)par[LP(layer,L_VARIANT)]*0x9e3779b9u,par[LP(layer,L_DENSITY)],par[LP(layer,L_STEPS)],en_note_grid[layer]);if(layer<7)for(int q=0;q<64;q++)motif_memory[layer][q]=en_note_grid[layer][q];}
typedef struct{uint32_t due;float velocity;int8_t degree;uint8_t layer;uint16_t epoch,midi_tag;} strum_note_t;
static strum_note_t strum_notes[16];static uint8_t strum_shared;
/* Bounded phrase capture: local keys only, never alters patch/recipe state. */
static int8_t learned_degree[8];static uint32_t learned_gap[8],learned_last,learned_held,learned_epoch;static uint8_t learned_count,learned_layer;static uint16_t learned_tags[8];static uint32_t learned_answers;
static void learned_reset(void){learned_count=0;learned_held=0;learned_last=0;learned_epoch=0;}
static void learned_note(int l,int key){if(par[P_MEMORY]!=2)return;
 if(learned_count&&(l!=learned_layer||learned_epoch!=en_track_revision[l]||(uint32_t)(clock_samples-learned_last)>(uint32_t)(EN_SR*4)))learned_count=0;
 learned_layer=(uint8_t)l;learned_epoch=en_track_revision[l];
 if(learned_count<8){learned_tags[learned_count]=midi_tag;learned_held|=1u<<learned_count;learned_degree[learned_count]=(int8_t)(en_perfect_degree(key)+en_key_octave*7);learned_gap[learned_count]=learned_count?clock_samples-learned_last:0;learned_count++;}learned_last=clock_samples;
}
static void learned_release(uint16_t tag){for(unsigned i=0;i<learned_count;i++)if(learned_tags[i]==tag){learned_held&=~(1u<<i);learned_last=clock_samples;}}
static void learned_answer(void){
 if(par[P_MEMORY]!=2){learned_reset();return;}
 if(learned_count<3||learned_held||!en_playing||en_frozen||(uint32_t)(clock_samples-learned_last)<(uint32_t)(EN_SR*1.5f))return;
 int l=learned_layer;if(learned_epoch!=en_track_revision[l]||en_death[l]||!audible(l)){learned_reset();return;}
 uint32_t due=clock_samples+64;for(unsigned i=0;i<learned_count;i++){
  if(i)due+=(uint32_t)fm_clampf((float)learned_gap[i],EN_SR*.16f,EN_SR*2);
  int d=learned_degree[i]+(i+1==learned_count?-2:0); /* soft answering cadence */
  for(int q=0;q<16;q++)if(!strum_notes[q].due){strum_notes[q]=(strum_note_t){due,.20f,(int8_t)d,(uint8_t)(l|64|16),(uint16_t)en_track_revision[l],0};break;}
 }learned_answers++;learned_reset();
}

static void track_silence(int l){for(int k=0;k<NV;k++)if(voice[k].layer==l&&!voice[k].drum&&voice[k].sfx!=2&&cosmic_tail[l]<=0){voice[k].held=0;voice[k].release=.96f;}for(int k=0;k<16;k++)if((strum_notes[k].layer&7u)==(unsigned)l)strum_notes[k].due=0;}
static void track_destroy(int l){track_silence(l);for(int k=0;k<64;k++)en_note_grid[l][k]=0;en_trigger[l]=0;en_visual[l]=en_energy[l]=0;field_pending&=(uint8_t)~(1u<<l);field_force&=(uint8_t)~(1u<<l);chime_pending&=(uint8_t)~(1u<<l);bowl_pending&=(uint8_t)~(1u<<l);}
/* Consumed voices keep their existing phase/envelope for a bounded tail.
 * Never create or unmute voices for the final bloom. */
static void track_consume(int l){cosmic_last=l;cosmic_tail[l]=8;
 for(int k=0;k<NV;k++)if(voice[k].on&&voice[k].layer==l&&!voice[k].midi_tag&&!voice[k].drum){voice[k].held=0;voice[k].release=fm_maxf(voice[k].release,fm_expf(-6.9f/(8*EN_SR)));}
 for(int k=0;k<16;k++)if((strum_notes[k].layer&7u)==(unsigned)l)strum_notes[k].due=0;
 for(int k=0;k<64;k++)en_note_grid[l][k]=0;
 field_pending&=(uint8_t)~(1u<<l);field_force&=(uint8_t)~(1u<<l);
}
static void track_birth(int l){cosmic_gain[l]=1;cosmic_tail[l]=0;uint32_t h=gx_hash(seed^en_captures*97u^en_field_cycles[l]*0x9e3779b9u^(uint32_t)l);int old=par[LP(l,L_VOICE)],patch;en_solar_remnant&=(uint8_t)~(1u<<l);if((h&3u)==0)en_sun_mask|=(uint8_t)(1u<<l);else en_sun_mask&=(uint8_t)~(1u<<l);
 {patch=(en_patch_locks&(1u<<l))?old:patch_pick(par,l,old,h);par[LP(l,L_VOICE)]=(int16_t)patch;}
 par[P_MASK+en_scene]|=(int16_t)(1u<<l);
 if(!(en_patch_locks&(1u<<l)))par[LP(l,L_VARIANT)]=(int16_t)((par[LP(l,L_VARIANT)]+1+(h>>8)%126u)&127u);rebuild(l);lane_next_sample[l]=0;en_track_patch[l]=(uint8_t)patch;en_track_variant[l]=(uint8_t)par[LP(l,L_VARIANT)];BARRIER();en_track_revision[l]++;
}

static int drum_start;static uint8_t drum_alternate;
static const float decay[EN_KITS][5]={{0.38f,0.20f,0.11f,0.13f,1.8f},{0.28f,0.31f,0.17f,0.14f,1.6f},{0.72f,0.23f,0.09f,0.18f,2.2f},{.8f,.32f,.18f,.55f,2.8f},{.35f,.22f,.28f,.7f,2.4f},{1.1f,.4f,.13f,.6f,3.2f},{.9f,.55f,.20f,.85f,3.1f},{.45f,.7f,.36f,1.2f,2.6f},{.65f,.28f,.065f,.38f,1.8f},{.8f,.4f,.09f,.55f,2.7f},{.55f,.22f,.07f,.28f,1.6f},{.65f,.30f,.07f,.36f,1.8f},{.85f,.42f,.09f,.48f,2.2f},{0.500f,0.240f,0.100f,0.300f,1.700f},{0.550f,0.260f,0.120f,0.360f,1.900f},{0.360f,0.290f,0.110f,0.280f,1.500f},{0.700f,0.350f,0.150f,0.420f,2.200f},{0.820f,0.250f,0.130f,0.450f,2.400f},{0.460f,0.240f,0.090f,0.320f,1.700f},{0.920f,0.380f,0.120f,0.500f,2.500f},{0.620f,0.300f,0.180f,0.540f,2.600f},{0.360f,0.200f,0.160f,0.380f,1.800f},{0.300f,0.180f,0.100f,0.280f,1.400f},{0.650f,0.300f,0.160f,0.420f,2.300f},{0.800f,0.360f,0.130f,0.480f,2.800f},{0.720f,0.280f,0.100f,0.500f,2.600f},{0.300f,0.180f,0.100f,0.260f,1.500f}};
static float drum_release[EN_KITS][5];
static en_phrase_event_t phrase_events[EN_NGAL][EN_PHRASE_EVENTS];static volatile uint32_t phrase_written[EN_NGAL];
static void phrase_record(int l,int d,uint32_t hold,float velocity,int manual){uint32_t w=phrase_written[l];phrase_events[l][w&15u]=(en_phrase_event_t){clock_samples,hold,(int16_t)d,(uint8_t)fm_clampf(velocity*255,0,255),(uint8_t)manual};BARRIER();phrase_written[l]=w+1;}
static void start_voice(int l,int d,float velocity){
 uint8_t saved_alternate=(unsigned)l<8u?alternate[l]:0;
 int is_drum=drum_start&&!bowl_override;if((unsigned)l>=EN_NGAL&&!is_drum)return;if(is_drum&&!(en_drum_parts_mask&(1u<<(d%5))))return;
 int anchor_voice=l==0&&par[LP(l,L_VOICE)]<24&&!bowl_override&&!manual_pitch&&!(alternate[l]&1u);
 int planet=(int)((sequence_step/(1u<<par[LP(l,L_RATE)]))%en_planets.count[l]);
 en_planet_t pp=en_planets.planet[l][planet];
 int k=bowl_override?LIVE_VOICE_BASE+(bowl_next++&3u):is_drum?DRUM_VOICE_BASE+(drum_alternate++&3u):l*2+(alternate[l]++&1u),vpatch=is_drum?0:par[LP(l,L_VOICE)],vtype=bowl_override?1:patch_type(vpatch);
 if(!is_drum){en_cue_kind[l]=live_strike?1:impact_voice==2?4:impact_voice==1?2:impact_voice==3?5:bowl_override?3:0;en_cue_age[l]=1;en_cue_x[l]=en_field_x[l];en_cue_y[l]=en_field_y[l];if(object_bowl_play){en_cue_kind[l]=6;en_cue_x[l]=object_bowl_x[l];en_cue_y[l]=object_bowl_y[l];}
 }
 if(manual_pitch&&!bowl_override&&!is_drum){
  if(!generated_strike)k=LIVE_VOICE_BASE+(live_next++&3u);
  else{
   /* Six bounded candidates: own pair plus spare live voices. Never steal a
    * held/releasing player-owned voice to extend a generated chord. */
   float best=1e9f;int chosen=l*2;
   for(int q=0;q<6;q++){int slot=q<2?l*2+q:LIVE_VOICE_BASE+q-2;
    if(voice[slot].midi_tag)continue;
    float cost=voice[slot].on?fm_fabsf(voice[slot].amp*voice[slot].env)+.0001f:0;
    if(cost<best){best=cost;chosen=slot;}
   }k=chosen;
  }
 }
 /* Two dedicated voices per galaxy; no hidden halo/choir overflow pool. */
 if(!bowl_override){if(!is_drum&&!manual_pitch)d+=(int)pp.pitch;if(!is_drum)velocity*=(float)pp.level*.01f;}
 if(!is_drum||bowl_override)d=manual_pitch?safe_degree(d):shared_degree(l,d,bowl_override);
 float f=is_drum?0:anchor_voice?HEALING_HZ[par[P_ANCHOR]]:tone_hz(bowl_override&&l==1?0:l,d),color=fm_clampf((float)par[LP(l,L_TONE)]*0.01f+(l<7?air_current[l][2]*.35f:0),0,1);
 /* Reference clips reserve the upper range for sparse accents: generated
  * keys/plucks use an octave-equivalent mid register; played notes stay direct. */
 if(!manual_pitch&&!bowl_override&&l!=1&&!is_drum&&(vtype==2||vtype==3||vtype==5||vtype==7)){while(f>432)f*=.5f;}
 if(manual_pitch&&!is_drum){if(manual_keyboard)f*=fm_exp2f((float)manual_octave);else if(l!=1){f*=fm_exp2f((float)manual_octave);while(f>963)f*=.5f;while(f<174)f*=2;}}
 if(!is_drum&&!bowl_override&&(!manual_pitch||generated_strike)){
  f=generated_register(l,f);if(l==1&&par[P_BASS_STRUM]==1&&generated_strike!=2)while(f>=64)f*=.5f;if(l==1&&generated_strike==2){while(f>=64)f*=.5f;f*=2;}
 }
 if(l==1&&par[P_BASS]==8&&!is_drum&&!bowl_override&&!manual_keyboard){while(f>=60)f*=.5f;while(f<20)f*=2;}
 /* Dark references reserve attacks for quieter accents, preserving played
  * keys and selected anchor frequencies. */
 if(!is_drum&&!manual_pitch&&l!=1&&!role_patch(vpatch)){int ref=en_reference();
  if(bowl_override||impact_voice)velocity*=(const float[]){.8f,.9f,.40f,.65f,.85f,.22f,.65f,.48f}[ref];
  else if(vtype!=0&&vtype!=4&&vtype!=6)velocity*=(const float[]){.8f,1,.45f,.7f,.9f,.30f,.7f,.62f}[ref];
 }
 /* Octave-equivalent changes preserve the actual chord and reference tuning. */
 int air_pitch=!is_drum&&!bowl_override&&!anchor_voice&&(!manual_pitch||generated_strike)&&
     l!=1&&air_current[l][3]>=.5f;
 if(air_pitch)f=f<=480.0f?f*2.0f:f*.5f;
 float legato_from=0;int expressive=!is_drum&&!bowl_override&&patch_expressive(vpatch,l);
 if(expressive&&!air_pitch&&par[P_GLIDE])for(int q=0;q<NV;q++)if(voice[q].on&&voice[q].held&&voice[q].layer==l&&voice[q].midi_tag==midi_tag)legato_from=voice[q].inc[0]*voice[q].pitch_ratio;
 /* Fixed alternating slots retain the preceding note independently of load. */
 /* one of two voices per galaxy overlaps the preceding note, avoiding mix gaps */

 voice[k].midi_tag=midi_tag;voice[k].midi_released=0;if(midi_tag)f*=midi_bend[(midi_tag-1u)>>7];
 voice[k].tail=voice[k].on?voice[k].last:0;voice[k].on=1;voice[k].layer=(uint8_t)l;voice[k].amp=velocity*(l==1&&par[P_BASS]==8?.80f:1);
 voice[k].pan=(!bowl_override&&l!=1&&!is_drum&&(vtype==0||vtype==4||vtype==6))?((alternate[l]&1u)?-.82f:.82f):0;
 if(l==1&&!is_drum&&!bowl_override&&!manual_pitch)voice[k].pan=(alternate[l]&1u)?-reference[en_reference()].basspan:reference[en_reference()].basspan;
 voice[k].level=0;voice[k].env=0;voice[k].phase[0]=voice[k].phase[1]=voice[k].phase[2]=0;
 voice[k].mix_gain=en_balance[l]*en_duck[l];voice[k].strummed=(uint8_t)(patch_strum(vpatch)&&(!manual_keyboard||en_strum_ms));voice[k].patch=(uint8_t)vpatch;voice[k].carrier=1;voice[k].sfx=impact_voice;voice[k].drum=(uint8_t)(is_drum&&!bowl_override?1+(d%5):0);voice[k].kit=(uint8_t)par[P_KIT];voice[k].noise_low=voice[k].noise_mid=0;voice[k].rng=gx_hash(seed^clock_samples^(uint32_t)k*0x9e3779b9u)|1u;
 if(is_drum){f=en_bass_root()*(voice[k].drum==2?2:1);}
 if(voice[k].drum==1){f=HEALING_HZ[par[P_ANCHOR]];while(f>96)f*=0.5f;while(f<48)f*=2;}
 /* Percussion has no melodic articulation or glide. Use precomputed kit
  * decay and bypass discarded pitch/envelope/planet work at every hit. */
 if(is_drum){voice[k].pitch_ratio=1;voice[k].glide_k=1;voice[k].vib_phase=0;voice[k].age=0;
  voice[k].inc[0]=f/EN_SR;voice[k].inc[1]=((f*2<=963)?f*2:f)/EN_SR;voice[k].inc[2]=((f*3<=963)?f*3:f)/EN_SR;
  voice[k].held=220;voice[k].attack=.045f;voice[k].release=drum_release[voice[k].kit][voice[k].drum-1];voice[k].rev=voice[k].partial=0;goto voice_started;
 }
 voice[k].pitch_ratio=legato_from>0?legato_from/(f/EN_SR):1;
 voice[k].glide_k=par[P_GLIDE]?1-fm_expf(-1/(par[P_GLIDE]*.001f*EN_SR)):1;voice[k].vib_phase=0;voice[k].age=0;
 voice[k].inc[0]=f/EN_SR;
 voice[k].inc[1]=((f*2<=963)?f*2:f)/EN_SR;
 voice[k].inc[2]=((l==1&&!bowl_override)?f*4:((f*3<=963)?f*3:f))/EN_SR;
 voice[k].rev=bowl_override?0.65f:(float)pp.space*.01f;
 voice[k].partial=(const float[]){0.12f,0.42f,0.24f,0.30f,0.20f,0.38f,0.06f,0.28f}[vtype]*(0.35f+color)*(1-0.45f*en_fog)*(1-en_shock[l]*0.18f);
 float attack=is_drum?0.004f:0.012f+(float)par[LP(l,L_ATTACK)]*0.035f;
 attack*=(const float[]){1.3f,0.15f,0.30f,0.12f,1.8f,0.18f,1.6f,0.1f}[vtype];
 if(is_drum)attack=0.002f;else if(l==1&&!bowl_override)attack=0.012f;
 float release=is_drum?0.09f+(float)par[LP(l,L_RELEASE)]*0.008f:0.5f+(float)par[LP(l,L_RELEASE)]*0.14f;
 release*=(const float[]){1.0f,1.4f,0.6f,1.1f,1.2f,0.8f,1.4f,0.5f}[vtype];
 voice[k].attack=1.0f-fm_expf(-1.0f/(attack*EN_SR));voice[k].release=fm_expf(-6.9f/(release*EN_SR));
 float duration=(60.0f/(float)par[P_TEMPO])*0.25f*(float)(1u<<par[LP(l,L_RATE)])*(float)par[LP(l,L_GATE)]*0.01f;
 if(!is_drum)duration*=(0.5f+(float)pp.hold*0.02f)*(1+0.35f*en_nebula+en_shock[l]*0.9f);
 if(en_event==2 && !is_drum)duration*=1+0.8f*en_event_mix;
 if((par[P_REST]==EN_NIGHTFALL)&&!is_drum){attack=fm_maxf(attack,0.8f);voice[k].attack=1-fm_expf(-1/(attack*EN_SR));duration=fm_maxf(duration,3.5f);}
 if(vtype==6&&l!=1&&!is_drum&&!bowl_override){duration=fm_maxf(duration,24);voice[k].attack=1-fm_expf(-1/(1.2f*EN_SR));voice[k].partial=.035f;}
 if(l==1&&!bowl_override){duration=fm_clampf(duration*1.6f,12.0f,20.0f);voice[k].release=fm_expf(-6.9f/(6.0f*EN_SR));voice[k].partial=0.28f+color*0.22f;voice[k].rev*=0.16f;}
 if(l!=1&&!is_drum&&!bowl_override&&(vtype==0||vtype==4)){duration=fm_maxf(duration,12);voice[k].attack=1-fm_expf(-1/(1.1f*EN_SR));voice[k].release=fm_expf(-6.9f/(8*EN_SR));voice[k].partial*=.75f;}
 if(vpatch>=8&&vpatch<36&&!bowl_override){if(l!=1)voice[k].partial*=.55f;voice[k].inc[1]*=1.0014f;voice[k].inc[2]*=.9992f;voice[k].attack*=.65f;voice[k].release=fm_expf(-6.9f/(10*EN_SR));}
 /* Reference-derived low-mid bed uses the existing three oscillators. Keep
  * the displayed anchor oscillator exact; soften its high carrier in the mix. */
 if(l!=1&&!is_drum&&!bowl_override&&!manual_pitch&&(vtype==0||vtype==4||vtype==6)){
  int ref=en_reference();float ceiling=reference[ref].ceiling,body=f;while(body>ceiling)body*=.5f;while(body<ceiling*.5f)body*=2;
  voice[k].inc[1]=body*(vpatch>=8?1.0014f:1.0006f)/EN_SR;
  voice[k].inc[2]=body*reference[ref].bodyratio*(vpatch>=8?.9992f:.9996f)/EN_SR;
  voice[k].partial=.65f;voice[k].carrier=fm_clampf(200.0f/f,.18f,.70f);
 }
 if(vpatch>=16&&vpatch<24&&l!=1&&!bowl_override){int p=vpatch-16;voice[k].partial=(const float[]){.38f,.50f,.62f,.72f,.42f,.58f,.48f,.68f}[p];voice[k].attack=1-fm_expf(-1/((1.4f+(float)(p%4)*.45f)*EN_SR));voice[k].release=fm_expf(-6.9f/((8+(float)p*.8f)*EN_SR));duration=fm_maxf(duration,18+(p%3)*5);voice[k].carrier*=.75f+.025f*p;}
 if(l!=1&&!is_drum&&!bowl_override&&!manual_pitch&&(vtype==0||vtype==4||vtype==6)){
  int ref=en_reference();voice[k].carrier*=reference[ref].carrier;voice[k].partial*=reference[ref].partial;
  float seconds=reference[ref].attack;voice[k].attack=1-fm_expf(-1/(seconds*EN_SR));
  duration=fm_maxf(duration,reference[ref].hold);
  voice[k].release=fm_expf(-6.9f/(reference[ref].release*EN_SR));
 }
 if(vpatch>=24&&vpatch<32&&l!=1&&!bowl_override){int p=vpatch-24;duration=.012f+(float)(p%3)*.004f;voice[k].attack=.016f+(p>=4?.012f:0);voice[k].release=fm_expf(-6.9f/((4+(float)(p%4)*1.8f+(p>=4?2:0))*EN_SR));voice[k].partial=.36f+.07f*(p%4);voice[k].noise_low=1;voice[k].pan=(alternate[l]&1u)?-.35f:.35f;}
 if(!is_drum&&!bowl_override&&!impact_voice){
  if(vpatch<24||vpatch>=32)duration*=1.35f;
  else voice[k].release=fm_expf(-6.9f/((.88f*(7+(float)((vpatch-24)%4)*2))*EN_SR));
 }
 if(vpatch>=32&&vpatch<36&&!is_drum&&!bowl_override){int p=vpatch-32;
  voice[k].carrier*=.55f+(float)p*.08f;voice[k].partial*=.65f+(float)p*.10f;
  voice[k].attack=1-fm_expf(-1/((2.2f+(float)p*.6f)*EN_SR));
  voice[k].inc[2]=voice[k].inc[1]*(const float[]){1.5f,2,.75f,1.25f}[p];
  voice[k].release=fm_expf(-6.9f/((14+(float)p*2)*EN_SR));duration=fm_maxf(duration,30);
 }
 if(!is_drum&&!bowl_override&&!impact_voice&&!manual_pitch)duration*=.85f;
 if(vpatch>=36&&vpatch<40&&!is_drum&&!bowl_override){
  int fmp=vpatch-36;voice[k].inc[1]=f*(const float[]){2,3,1,2}[fmp]/EN_SR;voice[k].inc[2]=f*(const float[]){1,2,2,4}[fmp]/EN_SR;
  voice[k].partial=(const float[]){.14f,.24f,.07f,.18f}[fmp];voice[k].carrier=1;
  voice[k].attack=1-fm_expf(-1/((fmp==2?1.7f:.008f)*EN_SR));voice[k].release=fm_expf(-6.9f/((fmp==2?12.0f:6.5f)*EN_SR));duration=fmp==2?20:.03f;
 }
 if(vpatch>=40&&vpatch<52&&!is_drum&&!bowl_override){
  int p=vpatch-40;static const float attacks[12]={.38f,.009f,.025f,2.4f,3.2f,.025f,.035f,4.0f,.20f,.025f,.015f,2.8f};
  static const float holds[12]={3,.025f,.08f,24,28,.05f,.18f,32,2,16,.04f,26};
  static const float tails[12]={2.8f,5.5f,4.2f,11,12,8,3.4f,14,3.5f,6,5,10};
  static const float colors[12]={.10f,.22f,.17f,.18f,.20f,.26f,.12f,.055f,.10f,.14f,.18f,.12f};
  voice[k].attack=1-fm_expf(-1/(attacks[p]*EN_SR));voice[k].release=fm_expf(-6.9f/(tails[p]*EN_SR));
  duration=holds[p];voice[k].partial=colors[p]*(.65f+color*.35f);voice[k].carrier*=.85f;voice[k].amp*=.85f;
  if(p==0){voice[k].inc[1]=f*1.001f/EN_SR;voice[k].inc[2]=f*2/EN_SR;}
  if(p==4||p==11){voice[k].inc[1]=f*.5005f/EN_SR;voice[k].inc[2]=f*1.5f/EN_SR;}
  if(p==5||p==10){voice[k].inc[1]=f*1.5f/EN_SR;voice[k].inc[2]=f*2/EN_SR;}
 }
 /* Leads/keys never inherit drone holds; pads retain their longer role. */
 if(!is_drum&&!manual_pitch&&!bowl_override&&l!=1&&(vtype==2||vtype==3))duration=fm_minf(duration,(par[P_REST]==EN_NIGHTFALL)?6:4);
 if(live_strike&&!generated_strike&&!bowl_override){duration=fm_minf(duration,5);voice[k].release=fm_minf(voice[k].release,fm_expf(-6.9f/(1.2f*EN_SR)));}
 if(!is_drum&&!bowl_override&&!impact_voice&&!manual_pitch){
  int ref=en_reference();const phrasing_t *shape=&phrasing[ref];
  if(l==1){duration=fm_maxf(duration,shape->bass_gap*.80f);voice[k].release=fm_expf(-6.9f/(((par[P_REST]==EN_NIGHTFALL)?7:4.5f)*EN_SR));}
  else if(vtype==2||vtype==3||vtype==5||vtype==7){duration=fm_maxf(duration,shape->melody_hold);voice[k].release=fm_expf(-6.9f/(shape->melody_tail*EN_SR));}
  voice[k].pan*=l==1?1:shape->width/0.82f;
 }
 /* Sustain belongs to the instrument envelope, before the shared FX sends.
  * Reference and intention shape a soft hold and a naturally fading release. */
 if(voice[k].strummed&&!is_drum&&!bowl_override&&!impact_voice&&l!=1){
  int ref=en_reference();float calm=(par[P_REST]==EN_NIGHTFALL)?1.0f:fm_clampf((reference[ref].hold-16)/18,0,1);
  float hold=1.25f+calm*1.75f,tail=3.2f+calm*3.0f;
  duration=hold;voice[k].release=fm_expf(-6.9f/(tail*EN_SR));
  voice[k].attack=fm_minf(voice[k].attack,1-fm_expf(-1/((.008f+calm*.010f)*EN_SR)));
  voice[k].amp*=.72f;
 }
 if(par[P_STYLE]==EN_CINEMATIC&&!is_drum&&!manual_pitch&&!bowl_override&&!impact_voice){
  float e=en_cinematic_energy;
  if(l==1){duration=2.2f+e*2.8f;voice[k].release=fm_expf(-6.9f/(3.8f*EN_SR));}
  else if(l==0||l==4||l==6){duration=10+e*12;voice[k].attack=1-fm_expf(-1/((2-e*.8f)*EN_SR));}
  else{duration=1.3f+e*2.2f;voice[k].release=fm_expf(-6.9f/(4.5f*EN_SR));}
 }
 if(!is_drum&&!bowl_override&&!impact_voice){const role_patch_t *r=role_patch(vpatch);if(r){
  float a=fm_clampf((par[LP(l,L_ATTACK)]+25)/70.0f,.35f,1.8f),tail=fm_clampf((par[LP(l,L_RELEASE)]+35)/100.0f,.35f,1.35f);
  voice[k].attack=1-fm_expf(-1/(r->attack*a*EN_SR));voice[k].release=fm_expf(-6.9f/(r->release*tail*EN_SR));
  duration=r->hold;voice[k].partial=r->partial*(.55f+.9f*color);voice[k].amp*=r->gain;
  voice[k].inc[1]=voice[k].inc[0]*r->ratio1;voice[k].inc[2]=voice[k].inc[0]*r->ratio2;
  if(vpatch<108&&l!=1&&(r->type==0||r->type==4||r->type==6)){float body=f;float ceiling=reference[vpatch>=108?en_reference():(vpatch-ROLE_PATCH_BASE)/7].ceiling;while(body>ceiling)body*=.5f;while(body<ceiling*.5f)body*=2;
   voice[k].inc[1]=body*r->ratio1/EN_SR;voice[k].inc[2]=body*r->ratio2/EN_SR;
   /* Bespoke roles already define gain and low-register body. Do not bury
    * their fundamental under the legacy dark-bed attenuation as well. */
   voice[k].carrier=fm_clampf(200/f,.18f,.70f)*fm_maxf(.25f,reference[(vpatch-ROLE_PATCH_BASE)/7].carrier);
  }else voice[k].carrier=1;
  voice[k].pan=(alternate[l]&1u)?-r->width:r->width;voice[k].rev=r->send;
 }}
 if(vpatch>=122&&vpatch<=125){const vocal_bank_t *sample=&vocal_banks[vpatch-122];float pitch=f;if(vpatch!=122){while(pitch>sample->root*2)pitch*=.5f;while(pitch<sample->root*.5f)pitch*=2;}voice[k].inc[2]=pitch/sample->root*(16000.0f/(EN_SR*sample->length));}
 voice[k].held=(uint32_t)((is_drum?0.045f:duration)*EN_SR);

 if(bowl_override){voice[k].attack=1-fm_expf(-1/(0.065f*EN_SR));voice[k].release=fm_expf(-6.9f/(7.5f*EN_SR));voice[k].held=1323;voice[k].partial=0.18f;en_bowls++;}
 if(impact_voice==1){voice[k].attack=1-fm_expf(-1/(0.055f*EN_SR));voice[k].release=fm_expf(-6.9f/(6.5f*EN_SR));voice[k].held=2646;voice[k].partial=0.12f;voice[k].rev=0.45f;en_chimes++;}
 if(impact_voice==3){voice[k].attack=1-fm_expf(-1/(0.12f*EN_SR));voice[k].release=fm_expf(-6.9f/(9.0f*EN_SR));voice[k].held=8820;voice[k].partial=0.16f;voice[k].rev=0.50f;}
 if(impact_voice==2){voice[k].attack=1-fm_expf(-1/(0.14f*EN_SR));voice[k].release=fm_expf(-6.9f/(10.5f*EN_SR));voice[k].held=13230;voice[k].partial=0.13f;voice[k].rev=0.50f;}
voice_started:
 if(!is_drum&&!impact_voice&&!bowl_override)phrase_record(l,(int)(69+12*fm_log2f(f/440)+.5f),voice[k].held,velocity,manual_pitch&&!generated_strike);
 if(!is_drum&&impact_voice!=2){en_last_planet[l]=(uint8_t)planet;en_planet_flash[l][planet]=fm_maxf(en_planet_flash[l][planet],0.65f);en_freq[l]=f;en_energy[l]=1;}
 if(!manual_pitch&&!is_drum&&!impact_voice)control_event(0);if((unsigned)l<7u)en_note_velocity[l]=(uint8_t)fm_clampf(velocity*255,0,255);BARRIER();en_note_serial[l]++;if(!is_drum&&!impact_voice&&!bowl_override&&!manual_pitch)lane_last_note[l]=clock_samples;en_notes++;if(is_drum)en_hits++;
 if(live_strike||bowl_override)alternate[l]=saved_alternate;

}
/* Shared diatonic chord vocabulary for played and generated chords. */
static const uint8_t chord_steps[4][4]={{0,0,0,0},{0,2,4,0},{0,3,4,0},{0,2,4,6}};
static uint32_t strum_gesture(int layer,uint32_t h){
 int mode=par[P_STRUM_MODE];if(!mode)return h;
 uint32_t stable=gx_hash(seed^(unsigned)layer*0x9e3779b9u);
 return mode==3?gx_hash(stable^(sequence_step/128u)*193u):stable;
}
static int strum_reverse(uint32_t h){int dir=par[P_STRUM_DIR];if(dir==1)return 0;if(dir==2)return 1;if(dir==3)return (sequence_step/4u)&1u;
 return par[P_STRUM_MODE]==2?(sequence_step/64u)&1u:(h>>9)&1u;
}
static unsigned strum_spacing(unsigned gap,uint32_t h,int note){int jitter=(int)(gx_hash(h^(unsigned)note)%25u)-12;int spacing=(int)gap+jitter*par[P_STRUM_FEEL]/100;return (unsigned)bounded(spacing,80,200);}
static float strum_velocity(uint32_t h,int note){float gesture=.80f+.04f*((h>>(note*3))&7u);if(par[P_STRUM_MODE]==2&&((sequence_step/64u)&1u))gesture=1.08f-.06f*note;return 1+(gesture-1)*par[P_STRUM_DYN]*.01f;}
static unsigned chord_gesture_gap(void){return par[P_CHORD_PLAYBACK]==1?0:par[P_CHORD_PLAYBACK]==2?(en_strum_ms?en_strum_ms:120):(par[P_STYLE]==EN_NIGHTFALL||par[P_STYLE]==EN_CINEMATIC?0:en_strum_ms);}
static void generated_acoustic_chord(int layer,int degree,float velocity,uint32_t h){control_event(1);control_event(2);
 int patch=par[LP(layer,L_VOICE)],harp=patch>=28;
 int type=1+(int)((h>>4)%3u),count=type==3?4:3;
 if(par[P_STYLE]==EN_NEOCLASSICAL){type=1;count=2;} /* root/third; sustained support supplies the remaining harmony */
 /* Phrase-local choices never change the player's chord or strum controls.
  * Reference/anchor select gestures, not a physiological Hz effect. */
 uint32_t gesture=strum_gesture(layer,gx_hash(h^(uint32_t)en_reference()*97u^(uint32_t)par[P_ANCHOR]*193u));
 unsigned gap=chord_gesture_gap(),elapsed=0;
 (void)harp;
 int reverse=strum_reverse(gesture);
 for(int note=0;note<count;note++){
  int pick=reverse?count-1-note:note,d=degree+chord_steps[type][pick];
  float v=velocity*.46f*strum_velocity(gesture,note);
  if(note&&gap)elapsed+=strum_spacing(gap,gesture,note);
  if(!note||!gap){manual_pitch=1;manual_octave=(int)fm_floorf((float)d/7);generated_strike=1;start_voice(layer,d,v);generated_strike=0;manual_pitch=0;}
  else for(int q=0;q<16;q++)if(!strum_notes[q].due){strum_notes[q]=(strum_note_t){clock_samples+(uint32_t)((float)elapsed*.001f*EN_SR),v,(int8_t)d,(uint8_t)(layer|64|16),(uint16_t)en_track_revision[layer]};break;}
 }
}
typedef struct{uint32_t due,epoch;int16_t degree;float velocity;uint8_t on,chord,pad;} timed_gesture_t;
static timed_gesture_t timed_gesture[7];static uint32_t timing_origin;
static int16_t context_pending[7];static uint8_t context_mask;
static int context_index(int p){if(p==P_TEMPO)return 0;if(p==P_STRUM)return 1;if(p==P_CHORD_PLAYBACK)return 2;if(p>=P_QUANT_MODE&&p<=P_QUANT_SWING)return 3+p-P_QUANT_MODE;return -1;}
static void context_commit(void){static const uint8_t params[7]={P_TEMPO,P_STRUM,P_CHORD_PLAYBACK,P_QUANT_MODE,P_QUANT_GRID,P_QUANT_STRENGTH,P_QUANT_SWING};for(int i=0;i<7;i++)if(context_mask&(1u<<i)){par[params[i]]=context_pending[i];if(i<3){en_control_apply_samples[i]=clock_samples;control_applied_serial[i]=control_seen[i];}if(params[i]==P_STRUM)en_strum_ms=(uint16_t)context_pending[i];}context_mask=0;}

int en_control_pending(int p){int ci=context_index(p);return ci>=0&&((context_mask&(1u<<ci))||control_seen[ci]!=control_serial[ci]);}
static uint32_t musical_due(int layer,int pad){
 if(!par[P_QUANT_MODE]||pad||layer==4||layer==0)return clock_samples;
 static const uint8_t divisors[7]={1,2,4,8,3,6,1};
 uint32_t beat=(uint32_t)(EN_SR*60/en_bpm),grid=beat/divisors[par[P_QUANT_GRID]];if(par[P_QUANT_GRID]==6)grid=beat*2/3;if(!grid)return clock_samples;
 uint32_t relative=clock_samples-timing_origin,slot=relative/grid,remainder=relative%grid;
 /* A past nearest point is played now; positive attraction is scheduled once. */
 uint32_t delay=par[P_QUANT_MODE]==2?(remainder<=EN_WORKER_FRAMES?0:grid-remainder):(remainder>grid/2?grid-remainder:0);unsigned strength=par[P_QUANT_MODE]==2?100:par[P_QUANT_STRENGTH];
 if(par[P_QUANT_MODE]==1){if(par[P_STYLE]==EN_NIGHTFALL)strength/=4;else if(par[P_STYLE]==EN_NEOCLASSICAL&&layer!=1&&layer!=3)strength/=2;}
 if(((slot+(delay!=0))&1u)&&par[P_QUANT_SWING])delay+=grid*(unsigned)par[P_QUANT_SWING]/100u;
 return clock_samples+delay*strength/100u;
}
static void fire_gesture(int l,int degree,float velocity,int chord,int pad){
 if(chord)generated_acoustic_chord(l,degree,velocity,gx_hash(seed^sequence_step^(uint32_t)l));
 else{start_voice(l,degree,velocity*(pad?.65f:1));if(pad)start_voice(l,degree+4,velocity*.48f);}
}
typedef struct{uint32_t due;float velocity;uint8_t part,on;} timed_drum_t;
static timed_drum_t timed_drums[20];static uint32_t drum_last_due[5];static uint8_t drum_last_mask;
static void drum_fire(int part,float velocity){if((drum_last_mask&(1u<<part))&&(uint32_t)(clock_samples-drum_last_due[part])<=EN_WORKER_FRAMES)return;drum_last_mask|=(uint8_t)(1u<<part);drum_last_due[part]=clock_samples;if(par[P_DRUMS]&&audible(7)){drum_start=1;start_voice(7,part,velocity);drum_start=0;}}
static void drum_schedule(int part,float velocity){uint32_t due=musical_due(1,0);if(due==clock_samples){drum_fire(part,velocity);return;}int slot=-1;for(int q=0;q<20;q++){if(!timed_drums[q].on){slot=q;continue;}if(timed_drums[q].part==part&&timed_drums[q].due==due){timed_drums[q].velocity=fm_maxf(timed_drums[q].velocity,velocity);return;}}if(slot>=0)timed_drums[slot]=(timed_drum_t){due,velocity,(uint8_t)part,1};}
static void timing_service(void){for(int q=0;q<20;q++)if(timed_drums[q].on&&(int32_t)(clock_samples-timed_drums[q].due)>=0){timed_drums[q].on=0;if(en_playing&&!en_frozen)drum_fire(timed_drums[q].part,timed_drums[q].velocity);}for(int l=0;l<7;l++){timed_gesture_t *g=&timed_gesture[l];if(g->on&&(int32_t)(clock_samples-g->due)>=0){g->on=0;if(en_playing&&!en_frozen&&!en_death[l]&&g->epoch==en_track_revision[l]&&audible(l))fire_gesture(l,g->degree,g->velocity,g->chord,g->pad);}}}
static int local_keyboard;
static void start_key(int l,int key,float velocity){if((unsigned)l>=EN_NGAL)return;
 live_strike=1;manual_keyboard=local_keyboard;
 int degree=en_perfect_degree(key)+(local_keyboard?en_key_octave*7:0),type=en_chord_mode;
 if(type>=4){int pc=key%12;if(pc<0)pc+=12;unsigned whites=local_keyboard?0xad5u:0xab5u;type=(whites&(1u<<pc))?0:type-3;}
 int count=type==0?1:type==3?4:3;if(count>1){control_event(1);control_event(2);}
 unsigned live_gap=chord_gesture_gap();if(live_gap&&count>1){uint32_t h=strum_gesture(l,gx_hash(seed^clock_samples^(unsigned)key));int reverse=strum_reverse(h);unsigned elapsed=0;for(int j=0;j<count;j++){if(j)elapsed+=strum_spacing(live_gap,h,j);int pick=reverse?count-1-j:j,d=degree+chord_steps[type][pick];float v=velocity/1.8f*strum_velocity(h,j);if(!j){manual_octave=(int)fm_floorf((float)d/7);manual_pitch=1;start_voice(l,d,v);}else for(int q=0;q<16;q++)if(!strum_notes[q].due){strum_notes[q]=(strum_note_t){clock_samples+(uint32_t)((float)elapsed*.001f*EN_SR),v,(int8_t)d,(uint8_t)(l|64|(local_keyboard?32:0)),(uint16_t)en_track_revision[l],midi_tag};break;}}manual_pitch=manual_keyboard=0;live_strike=0;return;}
 for(int j=0;j<count;j++){int d=degree+chord_steps[type][j];manual_octave=(int)fm_floorf((float)d/7);manual_pitch=1;start_voice(l,d,velocity/(count==1?1:1.8f));
  if(en_live_mode==1&&!type){unsigned v=LIVE_VOICE_BASE+((live_next-1)&3u);float f=(l==1?en_bass_root():HEALING_HZ[par[P_ANCHOR]])*fm_exp2f((float)key/12+(local_keyboard?en_key_octave:0));float lo=l==1?en_bass_root():174,hi=l==1?lo*2:963;if(!local_keyboard){while(f>=hi)f*=.5f;while(f<lo)f*=2;}float ratio=f/(voice[v].inc[0]*EN_SR);for(int q=0;q<3;q++)voice[v].inc[q]*=ratio;}
 }manual_pitch=manual_keyboard=0;live_strike=0;
}
/* Source-owned live notes: USB, TRS and BLE cannot release each other or
 * generated voices. All operations are bounded by the existing voice/queue caps. */
static void midi_release_tag(uint16_t tag,int force){
 learned_release(tag);
 unsigned channel=(tag-1u)>>7;
 for(int q=0;q<16;q++)if(strum_notes[q].midi_tag==tag)strum_notes[q].due=0;
 for(int q=0;q<NV;q++)if(voice[q].midi_tag==tag){voice[q].midi_released=1;if(force||!midi_sustain[channel]){voice[q].held=0;voice[q].release=fm_minf(voice[q].release,fm_expf(-6.9f/(.45f*EN_SR)));}}
}
static void midi_drop_source(unsigned source){
 for(unsigned ch=source*16;ch<source*16+16;ch++){midi_sustain[ch]=0;midi_bend[ch]=1;}
 for(int q=0;q<16;q++)if(strum_notes[q].midi_tag&&(strum_notes[q].midi_tag-1u)/2048u==source)strum_notes[q].due=0;
 for(int q=0;q<NV;q++)if(voice[q].midi_tag&&(voice[q].midi_tag-1u)/2048u==source)midi_release_tag(voice[q].midi_tag,1);
}
static void midi_channel_message(unsigned source,unsigned st,unsigned a,unsigned v){
 unsigned channel=source*16+(st&15u);uint16_t tag=(uint16_t)(1u+channel*128u+a);unsigned kind=st&240u;
 if(kind==0x80u||(kind==0x90u&&!v)){midi_release_tag(tag,0);return;}
 if(kind==0x90u){midi_release_tag(tag,1);midi_tag=tag;learned_note(par[P_FOCUS],(int)a-60);start_key(par[P_FOCUS],(int)a-60,(float)v*(.5f/127));midi_tag=0;return;}
 if(kind==0xb0u){
  if(a==64){midi_sustain[channel]=(uint8_t)(v>=64);if(v<64)for(int q=0;q<NV;q++)if(voice[q].midi_tag&&(voice[q].midi_tag-1u)/128u==channel&&voice[q].midi_released)midi_release_tag(voice[q].midi_tag,1);}
  else if(a==120||a==123){midi_sustain[channel]=0;for(int q=0;q<16;q++)if(strum_notes[q].midi_tag&&(strum_notes[q].midi_tag-1u)/128u==channel)strum_notes[q].due=0;for(int q=0;q<NV;q++)if(voice[q].midi_tag&&(voice[q].midi_tag-1u)/128u==channel)midi_release_tag(voice[q].midi_tag,1);}
  else {int param=(a>=16&&a<=23)?(a==23?P_DRUM_LEVEL:LP(a-16,L_LEVEL)):a==1?P_MOTION:a==7?LP(par[P_FOCUS],L_LEVEL):a==10?LP(par[P_FOCUS],L_PAN):a==74?LP(par[P_FOCUS],L_TONE):a==91?P_REVERB:a==93?P_ECHO:-1;if(param>=0){const en_param_t *info=en_param_info(param);par[param]=(int16_t)(info->lo+(int)(v*(unsigned)(info->hi-info->lo)/127u));remote_rev[param]++;en_remote_serial++;}}
 }else if(kind==0xe0u){float bend=fm_exp2f(((float)(a+(v<<7))-8192)*(2.0f/(8192*12)));float ratio=bend/midi_bend[channel];midi_bend[channel]=bend;for(int q=0;q<NV;q++)if(voice[q].midi_tag&&(voice[q].midi_tag-1u)/128u==channel)for(int osc=0;osc<3;osc++)voice[q].inc[osc]*=ratio;}
}
uint8_t en_scene_mask(int scene){uint8_t mask=(uint8_t)par[P_MASK+(scene&7)];mask&=(uint8_t)~2u;if(par[P_BASS]!=7)mask|=2;mask&=((1u<<EN_NGAL)-1u);return mask;}
uint32_t en_track_flags(int l){if((unsigned)l>=8)return EN_TRACK_OFF;int solo=0;for(int i=0;i<8;i++)solo|=par[LP(i,L_SOLO)];
 uint32_t f=0;if(l==EN_DRUM_TRACK){if(!par[P_DRUMS])f|=EN_TRACK_OFF;if(!par[P_DRUM_LEVEL])f|=EN_TRACK_LEVEL_ZERO;if(!par[P_DRUM_HITS])f|=EN_TRACK_DENSITY_ZERO;}
 else{if(en_death[l]>0||(l==1&&par[P_BASS]==7))f|=EN_TRACK_OFF;if(l!=1&&!(en_scene_mask(en_scene)&(1<<l)))f|=EN_TRACK_SCENE_OUT;if(!par[LP(l,L_LEVEL)])f|=EN_TRACK_LEVEL_ZERO;if(!par[LP(l,L_DENSITY)]||!par[P_DENSITY])f|=EN_TRACK_DENSITY_ZERO;}
 if(par[LP(l,L_MUTE)])f|=EN_TRACK_MUTED;if(solo&&!par[LP(l,L_SOLO)])f|=EN_TRACK_SOLO_OUT;if(!en_playing)f|=EN_TRACK_REST;return f;
}
static int audible(int l){return !(en_track_flags(l)&15u);}
int en_drum_note(unsigned at){
  /* Nested metrical ranks: turning Hits up adds steps without losing the pulse.
   * Per-phrase hashes change the answering rhythm, never the shared clock. */
  static const uint8_t rank[5][16]={{0,12,4,10,2,14,6,8,1,13,5,11,3,15,7,9},{0,13,6,9,2,15,4,11,1,12,7,8,3,14,5,10},{0,12,4,8,2,13,5,9,1,14,6,10,3,15,7,11},{0,15,6,11,2,13,7,9,1,14,4,10,3,12,5,8},{0,14,10,12,2,15,7,11,1,13,6,9,3,8,4,5}};
  unsigned q=at&15u;int mode=par[P_DRUMS],density=par[P_DRUM_HITS];if(!mode||!density)return 0;
  int maximum=(par[P_REST]==EN_NIGHTFALL)?4:mode==1?10:16;
  int hits=1+(density-1)*maximum/100;
  int evolve=par[P_EVOLVE]&&!(par[P_REST]==EN_NIGHTFALL);
  unsigned period=par[P_EVOLVE]<34?4u:par[P_EVOLVE]<67?2u:1u;
  uint32_t h=gx_hash(seed^(uint32_t)par[P_KIT]*0x9e3779b9u^(evolve?gx_hash(drum_cycle/period+1u):0u));
  unsigned shape=(h>>8)&3u;
  if(evolve&&(drum_cycle&1u))shape=(shape+1u)&3u;
  if(en_drum_phase==2&&evolve)hits=(hits+1)/2;
  else if(en_drum_phase==1&&evolve){hits=bounded(hits+hits/4,1,maximum);shape=4;}
  if(rank[shape][q]>=hits)return 0;
  if(q==0&&at>=16u&&(at%64u)==0)return 5;
  if(q==0||q==8)return 1;
  if(q==4||q==12)return 2;
  if(evolve&&en_drum_phase==1&&q>=13)return q==15?2:4;
  if((par[P_REST]==EN_NIGHTFALL))return 3;
  if(q&1u)return (h&(1u<<(q+8u)))?4:3;
  if(evolve&&mode==2&&(q==6||q==10)&&(h&(1u<<q)))return 1;
  return 3;

}
int en_note_at(int l,unsigned at){if((unsigned)l>=EN_NGAL||en_death[l]>0||!par[LP(l,L_DENSITY)])return 0;
 if(l==1){unsigned density=(unsigned)par[LP(l,L_DENSITY)];return !(at&3u)&&gx_hash(seed^(at/4u)*193u^(unsigned)par[LP(l,L_VARIANT)])%100u<density?1:0;}
 return en_note_grid[l][at%gx_len(par[LP(l,L_STEPS)])];
}
static void step(void){
 if(!(sequence_step&15u)){context_commit();timing_origin=clock_samples;}
 int l;en_step=(uint8_t)(sequence_step&15u);bar=sequence_step/16u;
 if(!en_frozen&&!(sequence_step&63u))motif_boundary();
 if(!(sequence_step&15u)){
  if(supernova_pending&&!en_frozen){core_pending=fm_maxf(core_pending,.35f);supernova_pending=0;}
  if(bloom_chord&&en_playing){core_pending=fm_maxf(core_pending,0.25f);bloom_chord=0;}
  if(!en_frozen){drum_cycle=bar;en_drum_phase=(uint8_t)(par[P_EVOLVE]&&!(par[P_REST]==EN_NIGHTFALL)?(bar%32u>=30u?2:bar%(par[P_EVOLVE]>=50?4u:8u)==(par[P_EVOLVE]>=50?3u:7u)?1:0):0);}
  /* Pulsar echo notes and quasar pad changes are phrase-quantized and in key. */
  if(en_playing&&!en_frozen){for(int i=0;i<3;i++){int lane=en_sky[i].layer;if(bar%(i==2?8u:4u)==0&&audible(lane)){if(i==2){en_field_cycles[lane]++;field_pending|=(uint8_t)(1u<<lane);}else bloom_pending|=(uint8_t)(1u<<lane);}}}
  scene_bars++;if(en_scene_pending<8){en_scene=en_scene_pending;par[P_SCENE]=en_scene;en_scene_pending=255;scene_bars=1;}event_bar();
  if(!en_frozen)en_bass_section=(uint8_t)(par[P_BASS]==8?1:par[P_BASS]&&par[P_BASS]<7?par[P_BASS]:(1+(bar/((par[P_REST]==EN_NIGHTFALL)?16u:8u)+seed%6u)%6u));
  if(remix_pending&&en_death[en_remix_layer])remix_pending=0;
  if(remix_pending && !en_frozen){
   int g=en_remix_layer;en_field_cycles[g]++;en_births++;en_life[g]=0;
   gx_plan(g,seed^gx_hash(en_births*97u+bar+g),par[LP(g,L_DENSITY)],par[LP(g,L_STEPS)],en_note_grid[g]);
   if(en_sun_mask&(1u<<g)){bloom_fx(en_field_x[g],en_field_y[g],1,6,g);core_pending=fm_maxf(core_pending,.35f);}
   en_death[g]=1.2f;burst_at(en_field_x[g],en_field_y[g],0.72f);remix_pending=0;
  }
  if(field_pending && !en_frozen){
   for(int l=0;l<EN_NGAL;l++)if((field_pending&(1u<<l))&&!en_death[l]){
    if(field_force&(1u<<l))gx_plan(l,seed^(uint32_t)par[LP(l,L_VARIANT)]*0x9e3779b9u^gx_hash(en_field_cycles[l]+(uint32_t)l),par[LP(l,L_DENSITY)],par[LP(l,L_STEPS)],en_note_grid[l]);
    else if(par[P_EVOLVE]&&motif_active<EN_MOTIF_EVOLVE){unsigned len=gx_len(par[LP(l,L_STEPS)]),pos=len-4u+gx_hash(en_field_cycles[l]+l)%4u;en_note_grid[l][pos]=(uint8_t)(1+shared_degree(l,(int)gx_hash(bar+l)%7,0));}
   }
   field_pending=field_force=0;
  }
  if(par[P_AUTO]&&!en_frozen && scene_bars>(uint32_t)((par[P_REST]==EN_NIGHTFALL)&&par[P_BARS]<32?32:par[P_BARS])){en_scene=(en_scene+1)&7;par[P_SCENE]=en_scene;scene_bars=1;en_greats++;}
  if(!en_frozen&&motif_active>0&&motif_active<EN_MOTIF_EVOLVE&&bar&&!(bar&31u))for(int l=0;l<7;l++)if(!en_death[l])for(unsigned q=0;q<gx_len(par[LP(l,L_STEPS)]);q++)en_note_grid[l][q]=motif_memory[l][q];
  if(!en_frozen && motif_active<EN_MOTIF_EVOLVE && par[P_EVOLVE] && bar && !(bar&7u)){
   /* Alternate only one layer's answer; the harmonic foundation survives. */
   int layer=(int)(gx_hash(seed^bar)%7u),n=(int)gx_len(par[LP(layer,L_STEPS)]),pos=n-4+(int)(gx_hash(bar)%4u);
   if(!en_death[layer]&&gx_hash(seed+bar)%100u<(unsigned)par[P_EVOLVE])en_note_grid[layer][pos]=(uint8_t)(1+gx_hash(seed^bar^layer)%7u);
  }
 }
 if(par[P_DRUMS]&&audible(EN_DRUM_TRACK)){int token=en_drum_note(sequence_step);if(token){if(token==5)drum_schedule(0,.34f);drum_schedule(token-1,.40f*phrasing[en_reference()].drums*(sequence_step&1u?.65f:1));}}
 int scan_start=phrase_cursor;for(int turn=0;turn<EN_NGAL;turn++){l=(scan_start+turn)%EN_NGAL;if(timed_gesture[l].on)continue;
  unsigned rate=1u<<par[LP(l,L_RATE)],len=gx_len(par[LP(l,L_STEPS)]),at=(sequence_step/rate)%len;
  en_layer_step[l]=(uint8_t)at;
  int enabled=!(en_track_flags(l)&~EN_TRACK_REST);
  int melodic=l==2||l==3||l==5;
  float phrase_limit=l==2?12:l==3?20:28;if(par[P_REST]==EN_NIGHTFALL)phrase_limit*=1.6f;
  int reentry=melodic&&par[LP(l,L_DENSITY)]&&((sequence_step&7u)==(unsigned)(l==2?0:l==3?2:4))&&(uint32_t)(clock_samples-lane_last_note[l])>(uint32_t)(phrase_limit*EN_SR);
  int entry=enabled&&((reference_entry&(1u<<l)) || reentry || (l==0&&(int32_t)(clock_samples-lane_next_sample[l])>=0));
  if(sequence_step%rate&&!entry)continue;
  int token=entry?1:en_note_at(l,at);
  unsigned chamber_arc=(bar/8u)&3u;
  if(par[P_STYLE]==EN_NEOCLASSICAL&&!entry&&(l==2||l==5)){
   int slot=chamber_phrase_slot(sequence_step,chamber_arc,l);if(slot<0)continue;
   /* Keep the five motif engines authoritative; only the familiar/off
    * theme uses the chamber seed. Learned/evolving/answering templates remain live. */
   if(motif_active<=EN_MOTIF_REMEMBER)token=1+chamber_phrase_degree(seed,sequence_step/64u,(unsigned)slot);
  }
  int converse=motif_active==EN_MOTIF_CONVERSE&&motif_caller>=0&&motif_responder>=0;
  unsigned call_at=sequence_step&63u;
  if(converse&&l!=0&&l!=1&&l!=4){
   if(call_at<32u){if(l!=motif_caller)continue;}
   else{if(l!=motif_responder||!audible(motif_caller)||!par[LP(motif_caller,L_LEVEL)]||en_death[motif_caller]||en_track_revision[motif_caller]!=motif_caller_epoch||en_track_revision[motif_responder]!=motif_responder_epoch||motif_call[call_at-32u]<0)continue;token=1;}
  }
  en_trigger[l]&=(uint16_t)~(1u<<(sequence_step&15u));
  if(!token||!audible(l)||!par[P_DENSITY]||!par[LP(l,L_DENSITY)])continue;
  uint32_t h=gx_hash(seed^sequence_step*97u^(unsigned)l);
  int attraction_density=(int)((float)par[P_DENSITY]*(0.82f+0.18f*(1-en_field_r[l]))*(0.92f+en_star*0.12f));
  if(en_death[l]>0||en_life[l]<.35f)continue;
  
  attraction_density=(int)((float)attraction_density*(1-en_shock[l]*0.3f));
  if(en_event==4 && l>0 && l<7 && (at&7u)==7u)token=1+(int)(gx_hash(seed^bar^l)%7u);
  if((par[P_REST]==EN_NIGHTFALL)){
   attraction_density=(int)((float)attraction_density*(l==5?.35f:.68f)*(1-.25f*wind_depth));
   if(!entry&&(l==2||l==3||l==5) && (at&3u))continue;
  }
  if(en_event==3)attraction_density=(int)((float)attraction_density*(1-0.65f*en_event_mix));
  /* Alignment alternates answering groups without overwriting their motifs. */
  if(en_event==1 && l>0 && l<7 && ((l+(int)(sequence_step/8u))&1))
   attraction_density=(int)((float)attraction_density*(1-0.55f*en_event_mix));
  if(!entry&&l!=1&&!(par[P_STYLE]==EN_NEOCLASSICAL&&(l==2||l==5))&&(int)(h%100u)>=attraction_density)continue;
  int chord=current_harmony();
  /* Impact keeps the established harmony; it shapes phrasing, not key. */
  /* One answering note responds to angular sector; anchors remain coherent. */
  if(l>1 && l<7 && (at&15u)>=12u)token=1+safe_degree(token-1+((bar/8u)&1u));
  if(l>0 && l<7 && en_comet>0.3f && (at&15u)==15u)chord+=2;
  const phrasing_t *shape=&phrasing[en_reference()];
  /* Gentle phrase accents, stable for a repeated motif; no random loud spikes. */
  float accent=1-shape->accent*.5f+shape->accent*(at%8u==0?1.0f:at%4u==0?.55f:.1f);
  float velocity=(l==1?.34f:.34f)*accent;
  if(en_event==3)velocity*=1-0.25f*en_event_mix;
  velocity*=(l==1?1:en_life[l])*(1-0.12f*en_fog)*(1-0.08f*en_shock[l]);
  int degree=l==0?0:token-1+chord;
  if(l==1){static const uint8_t phrase[6][8]={{0,0,4,0,0,2,4,0},{0,4,0,2,0,4,1,0},{0,2,4,2,0,4,5,4},{0,3,4,3,0,1,4,0},{6,4,2,0,4,2,1,0},{0,0,0,4,0,0,2,0}};int section=en_bass_section?en_bass_section-1:0;degree=phrase[section][(at/2)&7]+chord;if(section==5&&(at&7u))continue;}
  if(par[P_STYLE]==EN_CINEMATIC){
   static const uint8_t theme[4][8]={{0,2,4,3,2,0,1,4},{0,1,4,3,1,0,6,4},{4,2,1,0,2,3,1,0},{0,4,2,5,4,2,1,0}};const uint8_t *motif=theme[(mix_intention+en_reference())&3];
   static const uint8_t bassline[4][8]={{0,0,4,0,0,2,4,0},{0,4,0,2,0,4,5,4},{0,4,2,4,0,4,5,4},{0,0,0,4,0,0,2,0}};
   unsigned form=(sequence_step-cinematic_start_step)/128u&3u;
   if(l==1)degree=bassline[form][(at/2)&7]+chord;
   else if(l!=0&&l!=4&&l!=6)degree=motif[(at+(l==5?4:0))&7]+chord+((bar&7u)>=4u?2:0);
  }
  if(par[P_STYLE]==EN_NEOCLASSICAL){
   if(l==1)degree=chord+((bar&3u)==2u?4:0);else if(l==0||l==4)degree=chord;else if(par[P_MEMORY]==EN_MOTIF_OFF){static const uint8_t cell[7]={0,2,4,3,2,1,0};degree=chord+cell[(at/2u+(unsigned)l)%7u];}else degree=chord+token-1;
   if(!entry&&(l==3||l==5)&&((sequence_step&63u)>=48u||((at+(unsigned)l)&3u)==3u))continue;
   float arc=.78f+.18f*fm_sin_turns((float)(bar&31u)/32-.25f);velocity*=arc*(l==2?1:l==5?.65f:.82f)*(1+.06f*fm_sin_turns((float)(at&15u)/16));
  }
  if(converse&&call_at>=32u&&l==motif_responder)
   degree=safe_degree(motif_call[call_at-32u]+(call_at>=56u?-2:0))+chord;
  int vt=patch_type(par[LP(l,L_VOICE)]);
  if(!entry&&(int32_t)(clock_samples-lane_next_sample[l])<0)continue;

  int pad=l!=1&&(vt==0||vt==4||vt==6);
  int is_strum=l!=1&&patch_strum(par[LP(l,L_VOICE)]);
  uint32_t due=musical_due(l,pad);
  if(due!=clock_samples){if(!timed_gesture[l].on)timed_gesture[l]=(timed_gesture_t){due,en_track_revision[l],(int16_t)degree,velocity,1,(uint8_t)is_strum,(uint8_t)pad};}
  else fire_gesture(l,degree,velocity,is_strum,pad);
  if(is_strum)lane_next_sample[l]=clock_samples+(uint32_t)(reference[en_reference()].strum*EN_SR);
  if(l==1&&par[P_BASS_STRUM]>0){
   int mode=par[P_BASS_STRUM];unsigned gap=par[P_STRUM]?par[P_STRUM]:80u+gx_hash(seed^bar)%121u;
   int answer=degree+(mode==2?4:0);
   for(int q=0;q<16;q++)if(!strum_notes[q].due){strum_notes[q]=(strum_note_t){clock_samples+(uint32_t)((float)gap*.001f*EN_SR),velocity*.60f,(int8_t)answer,(uint8_t)(1|16|(mode==1?8:0)),(uint16_t)en_track_revision[1]};break;}
  }
  if(l==1)lane_next_sample[l]=clock_samples+(uint32_t)(shape->bass_gap*EN_SR);else if(pad)lane_next_sample[l]=clock_samples+(uint32_t)(reference[en_reference()].retrigger*EN_SR);
  /* All generated patch families share reference-paced onset spacing.
   * Notes within an intentional chord/strum remain one musical gesture. */
  const role_patch_t *role=role_patch(par[LP(l,L_VOICE)]);
  float lane_gap=(is_strum?reference[en_reference()].strum:pad?reference[en_reference()].retrigger:l==1?shape->bass_gap:shape->melody_gap)*(1+.45f*wind_depth);
  if(role)lane_gap=role->gap*(1+.45f*wind_depth);
  if(par[P_STYLE]==EN_NEOCLASSICAL){
   if(l==2)lane_gap=(60.0f/en_bpm)*.40f;
   if(l==5)lane_gap=(60.0f/en_bpm)*(chamber_arc==2?.70f:1.6f);
   if(l==3)lane_gap=(60.0f/en_bpm)*(chamber_arc==2?1.5f:3.0f);
   lane_gap*=.90f+(float)(gx_hash(seed^(sequence_step/8u)^(unsigned)l)&31u)*.006f;lane_next_sample[l]=clock_samples;}
  if(par[P_STYLE]==EN_CINEMATIC){float e=en_cinematic_energy;lane_gap*=pad?.70f+.15f*(1-e):l==1?.40f+.15f*(1-e):.42f+.16f*(1-e);lane_next_sample[l]=clock_samples;}
  uint32_t next=clock_samples+(uint32_t)(lane_gap*EN_SR);if((int32_t)(next-lane_next_sample[l])>0)lane_next_sample[l]=next;
  if(converse&&call_at<32u&&l==motif_caller)motif_call[call_at]=(int8_t)safe_degree(degree-chord);
  reference_entry&=(uint8_t)~(1u<<l);
  phrase_cursor=(uint8_t)((l+1)%EN_NGAL);
  en_trigger[l]|=(uint16_t)(1u<<(sequence_step&15u));
 }
 sequence_step++;
 en_scene_phase=fm_clampf((float)scene_bars/(float)par[P_BARS],0,1);
}
#define ECHO_N 11025u
/* Q15 delay storage frees44100B for native DMA overlap alongside BLE.
 * Dry synthesis and floating-point reverb remain unchanged. */
static int16_t echo_l[ECHO_N] BIG,echo_r[ECHO_N] BIG;
static float drum_room[2][704] BIG;static uint32_t drum_pos[2],drum_noise=0x871ab31u;static float drum_wet;
/* Move the16KiB room bank to ordinary SRAM to retain8KiB pool reserve
 * with two full native framebuffers. The audio layout/values are unchanged. */
#if EN_BLE
static float room[4][1024] BIG;
#else
static float room[4][1024];
#endif
static uint32_t ep,rp[4];
static const uint32_t ROOM_LEN[4]={779,809,745,711};
static float dc_l,dc_r,master_s;
static float speaker_mix,speaker_low,speaker_mid,speaker_high;
#include "response_mapping.h"
#include "space_runtime.h"
/* Conservative small-speaker profile, not a measured FM1 response correction.
 * The crossfade keeps switching quiet; it never changes pitch or oscillator tuning. */
static void output_profile(float *l,float *r){
 float mono=0.5f*(*l+*r);
 speaker_low+=0.0113f*(mono-speaker_low); /* ~80Hz sub protection; retain bass body */
 speaker_mid+=0.0688f*(mono-speaker_mid); /* ~500 Hz */
 speaker_high+=0.203f*(mono-speaker_high); /* ~1.6 kHz */
 float shaped=((mono-speaker_low)+.36f*(speaker_mid-speaker_low)+.26f*(speaker_high-speaker_mid))*0.94f;
 speaker_mix+=((en_output_route?1.0f:0.0f)-speaker_mix)*0.00008f;
 *l+=speaker_mix*(shaped-*l);*r+=speaker_mix*(shaped-*r);
}
static void bridge_reset(void);static void bridge_cold(void);
static void soundscape_reset(void);
void en_init(void){air_filter_readout=0;air_smoothing=0;air_manual_track=air_manual_cut=air_manual_res=0;air_filter_mix=0;air_filter_track=0;for(unsigned c=0;c<2;c++)air_filter_low[c]=air_filter_band[c]=0;band_low=band_mid=0;for(unsigned i=0;i<3;i++)en_audio_band[i]=band_sum[i]=0;for(unsigned a=0;a<8;a++)for(unsigned b=0;b<4;b++)air_current[a][b]=0;air_target=air_target_track=air_target_dest=0;en_queue_drops=en_clock_coalesced=0;for(unsigned q=0;q<4;q++)en_dsp_stage_us[q]=en_dsp_stage_max_us[q]=0;for(unsigned q=0;q<3;q++)control_applied_serial[q]=control_event_serial[q]=0;for(int q=0;q<7;q++)control_seen[q]=control_serial[q]=0;drum_last_mask=0;for(int q=0;q<20;q++)timed_drums[q].on=0;context_mask=0;soundscape_reset();timing_origin=0;for(int l=0;l<7;l++)timed_gesture[l].on=0;en_patch_locks=0;en_core_turn=0;en_core_evolution=0;for(int l=0;l<EN_NGAL;l++)phrase_written[l]=0;motif_active=EN_MOTIF_REMEMBER;motif_variation_active=1;motif_caller=motif_responder=-1;for(int q=0;q<32;q++)motif_call[q]=-1;for(int kit=0;kit<EN_KITS;kit++)for(int kind=0;kind<5;kind++)drum_release[kit][kind]=fm_expf(-6.9f/(decay[kit][kind]*1.2f*EN_SR));learned_reset();learned_answers=0;cinematic_start_step=0;en_cinematic_stage=0;en_cinematic_energy=.28f;output_external=0;en_output_route=1;en_remote_serial=0;for(int q=0;q<P_NPARAMS;q++)remote_rev[q]=0;midi_tag=0;for(int q=0;q<64;q++){midi_sustain[q]=0;midi_bend[q]=1;}auto_reference=-1;en_sample_rate=44100;en_drum_parts_mask=31;en_grb_victim=-1;bridge_reset();bridge_cold();
 unsigned i,k;int16_t p[P_NPARAMS];uint16_t pool0;
 for(i=0;i<=SN;i++)sint[i]=fm_sinf((float)i*FM_TWO_PI/(float)SN);
 for(i=0;i<ECHO_N;i++)echo_l[i]=echo_r[i]=0;
 drum_pos[0]=drum_pos[1]=0;drum_wet=0;drum_noise=0x871ab31u;for(k=0;k<2;k++)for(i=0;i<704;i++)drum_room[k][i]=0;
 for(k=0;k<4;k++){rp[k]=0;for(i=0;i<1024;i++)room[k][i]=0;}
 for(i=0;i<NV;i++)voice[i].on=0;en_shed_count=0;en_voice_budget=NV;en_active_voices=0;en_graphics_throttle=pressure_history=0;en_audio_max_us=en_pressure_events=0;quiet_halves=0;
 en_piece(5,p,&pool0);for(i=0;i<P_NPARAMS;i++)par[i]=p[i];
 en_scene_pending=255;for(i=0;i<8;i++){lane_last_note[i]=0;en_lane_rms[i]=en_lane_peak[i]=0;en_meter[i]=en_visual[i]=0;en_trigger[i]=0;en_energy[i]=en_freq[i]=0;alternate[i]=0;rebuild(i);}
 en_bloom_mode=en_bloom_eternal=0;
 en_key_octave=0;manual_keyboard=0;mix_intention=5;generated_strike=0;space_reset();field_samples=0;en_diagnostic_mode=0;runtime_jobs=runtime_fallbacks=0;en_mix_limit=1;en_master_clip=0;en_drum_phase=0;drum_cycle=0;for(i=0;i<8;i++){en_balance[i]=i==1?0.85f:1;balance_peak[i]=duck_peak[i]=0;en_peak[i]=0;en_clip[i]=0;en_duck[i]=1;}
 object_bowl_mask=object_bowl_play=0;drum_start=0;drum_alternate=0;en_live_mode=2;en_strum_ms=0;en_chord_mode=live_next=discharge_next=0;thunder_env=thunder_low=0;for(int q=0;q<4;q++)en_discharge[q].age=0;for(int q=0;q<8;q++)en_cue_age[q]=0;
 en_sync_role=clock_seen=master_was_playing=master_has_started=0;midi_clock_phase=0;
 qr=qw=0;sequence_step=bar=scene_bars=clock_samples=last_clock_sample=clock_count=ep=0;ext_ticks=local_ticks=clock_interval=0;step_phase=0;
 en_playing=1;en_frozen=en_ext=0;en_scene=0;en_notes=en_hits=en_greats=en_years=universe_samples=0;
 gate=wetrev=wetecho=width_s=master_s=dc_l=dc_r=0;
 speaker_mix=1;speaker_low=speaker_mid=speaker_high=0;field_init();
 en_sleep_expired=0;sleep_elapsed=en_sleep_remaining=0;wind_depth=wind_l=wind_r=0;night_mix=0;night_gain=1;mute_gain=1;muted=0;
}
static void galaxy_clock(int p){
 if(en_sync_role==1)return;
 if(p==EN_CLK_TICK){uint32_t delta=clock_samples-last_clock_sample;last_clock_sample=clock_samples;if(clock_seen&&delta>400&&delta<12000){clock_interval=clock_interval?(clock_interval*7u+delta)/8u:delta;en_ext=1;}clock_seen=1;ext_ticks++;}
 else if(p==EN_CLK_START){cinematic_start_step=sequence_step=bar=scene_bars=0;ext_ticks=local_ticks=0;en_playing=1;step_phase=0;clock_seen=0;clock_interval=0;last_clock_sample=clock_samples;}
 else if(p==EN_CLK_STOP)en_playing=0;else if(p==EN_CLK_CONTINUE){en_playing=1;last_clock_sample=clock_samples;clock_seen=0;}
}
static uint32_t rate_scale(uint32_t x,float ratio){float q=(float)x*ratio;if(q>=4294967296.0f)q-=4294967296.0f;return (uint32_t)q;}
static void commands(void){
 static const uint8_t control_params[7]={P_TEMPO,P_STRUM,P_CHORD_PLAYBACK,P_QUANT_MODE,P_QUANT_GRID,P_QUANT_STRENGTH,P_QUANT_SWING};
 for(int ci=0;ci<7;ci++)if(control_seen[ci]!=control_serial[ci]){int p=control_params[ci],v=control_chosen[ci];control_seen[ci]=control_serial[ci];if(en_playing&&clock_samples){context_pending[ci]=(int16_t)v;context_mask|=(uint8_t)(1u<<ci);}else{par[p]=(int16_t)v;if(p==P_STRUM)en_strum_ms=(uint16_t)v;if(ci<3){en_control_apply_samples[ci]=clock_samples;control_applied_serial[ci]=control_seen[ci];}}}
 uint32_t stop=qw; /* Bounded snapshot: IRQ producers cannot prolong this pass. */
 while(qr!=stop){command_t c=cq[qr++%QN];int p=c.a,v=c.b;
  if(c.c==C_SET){const en_param_t *info=en_param_info(p);v=bounded(v,info->lo,info->hi);int ci=context_index(p);if(ci>=0&&en_playing&&clock_samples){context_pending[ci]=(int16_t)v;context_mask|=(uint8_t)(1u<<ci);continue;}if((p==P_REST||p==P_SLEEP_TIMER) && par[p]!=v){sleep_elapsed=0;en_sleep_expired=0;}
   if(p==P_STRUM&&v>0)v=bounded(v,80,200);if(p==P_SLEEP_TIMER)v=0;if(p==P_POWER&&(v==0||v==1||v==3))v=4;int old=par[p];if(p==P_TUNING&&old!=v){for(int k=0;k<NV;k++)if(voice[k].on&&!voice[k].drum&&!voice[k].midi_tag)voice[k].held=0;}par[p]=(int16_t)v;
   if(p==P_STYLE&&old!=v){cinematic_start_step=sequence_step;en_cinematic_stage=0;en_cinematic_energy=.28f;if(old==EN_CINEMATIC||v==EN_CINEMATIC||old==EN_NEOCLASSICAL||v==EN_NEOCLASSICAL){reference_entry=127u;for(int l=0;l<EN_NGAL;l++)lane_next_sample[l]=clock_samples;}}
   if(p==P_OUTPUT)en_output_detect(output_external);if(p==P_AUTO_LEVEL&&!v)for(int q=0;q<8;q++)en_balance[q]=1;if(p==P_AUTO_DUCK&&!v)for(int q=0;q<8;q++)en_duck[q]=1;
   if(p>=P_LAYER&&p<P_LAYER+7*16&&(p-P_LAYER)%16==L_VOICE){int l=(p-P_LAYER)/16;for(int q=0;q<EN_NGAL;q++)if(q!=l&&par[LP(q,L_VOICE)]==v){par[LP(q,L_VOICE)]=(int16_t)old;patch_publish(q);}patch_publish(l);}
   if(p==P_REFERENCE&&old!=v){if(v==0)auto_reference=-1;reference_entry=127u;for(int l=0;l<EN_NGAL;l++)lane_next_sample[l]=clock_samples;for(int k=0;k<NV;k++)if(voice[k].on&&!voice[k].drum&&!voice[k].midi_tag)voice[k].held=0;}
   if(p==P_CHORD){en_chord_mode=(uint8_t)v;en_live_mode=2;}if(p==P_STRUM)en_strum_ms=(uint16_t)v;
   if(p==P_ANCHOR&&old!=v){auto_reference=-1;float ratio=HEALING_HZ[v]/HEALING_HZ[old],lo=en_bass_root(),hi=lo*2;for(int k=0;k<NV;k++)if(voice[k].on&&voice[k].layer==1&&!voice[k].drum&&!voice[k].sfx){float before=voice[k].inc[0]*EN_SR,f=before*ratio;while(f>=hi)f*=.5f;while(f<lo)f*=2;for(int q=0;q<3;q++)voice[k].inc[q]*=f/before;}}
   if(p>=P_LAYER && p<P_LAYER+7*16 && ((p-P_LAYER)%16==L_DENSITY||(p-P_LAYER)%16==L_STEPS||(p-P_LAYER)%16==L_VARIANT))rebuild((p-P_LAYER)/16);
   if(p==P_SCENE){if(en_playing)en_scene_pending=(uint8_t)v;else{en_scene=(uint8_t)v;en_scene_pending=255;}scene_bars=0;}
  }else if(c.c==C_STRUM){en_strum_ms=(uint16_t)(v<=0?0:bounded(v,80,200));par[P_STRUM]=(int16_t)en_strum_ms;
  }else if(c.c==C_ASSIST){if(p>=0)en_live_mode=(uint8_t)bounded(p,0,2);if(v>=0)en_chord_mode=(uint8_t)bounded(v,0,6);
  }else if(c.c==C_SYNC){
   if(en_sync_role==1&&master_was_playing)en_midi_out(0xfc,0,0);
   en_sync_role=(uint8_t)v;master_was_playing=master_has_started=0;midi_clock_phase=0;clock_seen=0;en_ext=0;clock_interval=0;ext_ticks=local_ticks=0;
   if(v==1){cinematic_start_step=sequence_step=bar=scene_bars=0;step_phase=0;if(en_playing){en_midi_out(0xfa,0,0);master_was_playing=master_has_started=1;}}else if(v>=2)en_playing=0;
  }else if(c.c==C_BLOOM){
   if(p==0){uint8_t oldmode=en_bloom_mode;uint32_t tag=(uint32_t)v;en_bloom_mode=0;en_bloom_eternal=(uint8_t)((tag&0xff00u)==0xb100u&&((tag>>2)&1u));if(oldmode!=en_bloom_mode)bloom_launch_pending=0;}
   else if(p==1){(void)v;}
   else if(p==2){(void)v;}
  }else if(c.c==C_SEED){seed=(uint32_t)c.b;if(!seed)seed=1;auto_reference=-1;}
  else if(c.c==C_REGEN){learned_reset();patches_unique(par,seed);for(int l=0;l<EN_NGAL;l++)patch_publish(l);en_scene=(uint8_t)par[P_SCENE];en_scene_pending=255;for(p=0;p<8;p++)rebuild(p);field_init();}
  else if(c.c==C_PLAY){if(v)en_sleep_expired=0;en_frozen=0;if(v&&clock_samples-last_clock_sample>22050u){en_ext=0;ext_ticks=local_ticks=0;}en_playing=(uint8_t)(v!=0);}
  else if(c.c==C_FREEZE)en_frozen=(uint8_t)(v!=0);
  else if(c.c==C_AIR_FILTER){air_manual_track=(unsigned)p;air_manual_cut=(unsigned)v/128u;air_manual_res=(unsigned)v%128u;}
  else if(c.c==C_AIR){air_target_track=(unsigned)p/6u;air_target_dest=(unsigned)p%6u;air_target=(unsigned)bounded(v,0,32767);}
  else if(c.c==C_TOUCH){int l=par[P_FOCUS];par[LP(l,L_VARIANT)]=(par[LP(l,L_VARIANT)]+1)&127;rebuild(l);}
  else if(c.c==C_PANIC){learned_reset();for(int q=0;q<16;q++)strum_notes[q].due=0;for(p=0;p<NV;p++)voice[p].on=0;}
  else if(c.c==C_MUTE)muted=(uint8_t)(v!=0);
  else if(c.c==C_PLANET){int l=p/32,i=(p/4)&7,f=p&3;if(i<en_planets.count[l]){uint8_t *q=(uint8_t *)&en_planets.planet[l][i];q[f]=(uint8_t)bounded(v,0,f?100:6);}}
  else if(c.c==C_PLANETS_RESTORE){for(int l=0;l<8;l++){en_planets.count[l]=(uint8_t)bounded(restore_planets.count[l],3,8);for(int i=0;i<8;i++)en_planets.planet[l][i]=restore_planets.planet[l][i];}}
  else if(c.c==C_COSMIC){en_event_pending=(uint8_t)p;en_remix_layer=(uint8_t)v;}
  else if(c.c==C_KEY_OCTAVE){en_key_octave=(int8_t)v;}
  else if(c.c==C_INTENTION){if(mix_intention!=(uint8_t)v)auto_reference=-1;mix_intention=(uint8_t)v;}
  else if(c.c==C_DRUMPARTS){en_drum_parts_mask=(uint8_t)v;for(int k=0;k<NV;k++)if(voice[k].drum&&!(v&(1u<<(voice[k].drum-1)))){voice[k].held=0;voice[k].release=.96f;}}
  else if(c.c==C_QUALITY){if((unsigned)v!=(unsigned)EN_SR){float ratio=(float)v/EN_SR,inverse=1/ratio;
   for(int k=0;k<NV;k++){for(int h=0;h<3;h++)voice[k].inc[h]*=inverse;voice[k].held=(uint32_t)(voice[k].held*ratio);voice[k].attack*=inverse;voice[k].release=1-(1-voice[k].release)*inverse;}
   for(int q=0;q<16;q++)if(strum_notes[q].due)strum_notes[q].due=rate_scale(strum_notes[q].due,ratio);
   for(int q=0;q<8;q++)lane_next_sample[q]=rate_scale(lane_next_sample[q],ratio);
   clock_samples=rate_scale(clock_samples,ratio);last_clock_sample=rate_scale(last_clock_sample,ratio);clock_interval=rate_scale(clock_interval,ratio);sleep_elapsed=rate_scale(sleep_elapsed,ratio);universe_samples=rate_scale(universe_samples,ratio);field_samples=rate_scale(field_samples,ratio);en_sample_rate=(float)v;bridge_reset();
  }}
  else if(c.c==C_DIAGNOSTIC){en_diagnostic_mode=(uint8_t)v;}
  else if(c.c==C_STRIKE){if(v>=-12&&v<=14){midi_tag=(uint16_t)(6145+v+12);midi_release_tag(midi_tag,1);local_keyboard=1;learned_note(p,v);start_key(p,v,0.5f);local_keyboard=0;midi_tag=0;}}
  else if(c.c==C_CORE_STAGE){if((uint32_t)v>en_core_evolution)en_core_evolution=(uint32_t)v;}
  else if(c.c==C_KEY_RELEASE){midi_release_tag((uint16_t)(6145+v+12),1);}
  else if(c.c==C_CLOCK)galaxy_clock(p);
 }
}
#include "soundscape_runtime.h"
volatile uint32_t en_dsp_stage_us[4],en_dsp_stage_max_us[4];
#ifndef EN_HOST
static uint32_t dsp_stamp(void){return plat_profile_ticks();}
static uint32_t dsp_elapsed(uint32_t t){return plat_profile_elapsed(t);}
#else
static uint32_t dsp_stamp(void){return 0;}
static uint32_t dsp_elapsed(uint32_t t){(void)t;return 0;}
#endif
static void dsp_stage(unsigned i,uint32_t us){en_dsp_stage_us[i]=us;if(us>en_dsp_stage_max_us[i])en_dsp_stage_max_us[i]=us;}
void en_render(int32_t *out,uint32_t n,uint32_t gain){
 uint32_t stage_start=dsp_stamp(),voice_us=0,loop_start;


 int core_allowed=0;float lane_square[8]={0},lane_peak[8]={0},dry_square=0,wet_square=0;float layer_level[8];uint32_t routing_step=0;int routing_dirty=1;float block_peak[8]={0},reactive_peak[8]={0},block_pan[8],block_carrier[8],block_partial[8];uint32_t j,k,pkt;extern int plat_midi_in(uint32_t *pkt);
 commands();
 uint32_t active=air_smoothing;
 if(air_target&&air_target_dest<4)active|=1u<<(air_target_track*4+air_target_dest);
 if(active){
  float step=fm_minf(1,(float)n/(EN_SR*.15f));
  air_smoothing=active;
  while(active){
   unsigned index=(unsigned)__builtin_ctz(active),q=index/4,d=index%4;
   uint32_t bit=1u<<index;
   active&=~bit;
   float target=q==air_target_track&&d==air_target_dest?air_target*(1.0f/32767):0;
   float value=air_current[q][d]+(target-air_current[q][d])*step;
   if(!target&&value<.00001f){value=0;air_smoothing&=~bit;}
   air_current[q][d]=value;
  }
 }
 for(unsigned i=0;i<3;i++){en_audio_band[i]=band_sum[i]/(float)n;band_sum[i]=0;}
 learned_answer();soundscape_control();soundscape_prepare(n);dsp_stage(0,dsp_elapsed(stage_start));stage_start=dsp_stamp();
 if(!en_playing){for(int q=0;q<20;q++)timed_drums[q].on=0;for(int q=0;q<7;q++)timed_gesture[q].on=0;if(context_mask)context_commit();}
 if(en_diagnostic_mode==2){for(uint32_t q=0;q<n*2;q++)out[q]=0;field_advance((float)n/EN_SR);return;}
 float meter_peak[8];uint16_t meter_clip[8];en_master_clip=en_master_clip>n?en_master_clip-n:0;float meter_decay=fm_powf(.9999f,(float)n);for(unsigned q=0;q<8;q++){meter_peak[q]=en_peak[q]*meter_decay;duck_peak[q]*=meter_decay;meter_clip[q]=en_clip[q]>n?en_clip[q]-n:0;}
 float drum_damping=(float)par[P_DRUM_DAMP]*.01f,drum_echo_gain=(float)par[P_DRUM_ECHO]*.007f,drum_room_target=(float)par[P_DRUM_ROOM]*.009f;
 if(par[P_DRUM_AUTO]){int ref=en_reference();drum_damping=fm_clampf(drum_damping+.03f*(ref-3)+.002f*par[P_ANCHOR],0,1);float space=.80f+.06f*ref+.004f*par[P_ANCHOR];drum_echo_gain*=space;drum_room_target*=space;}
 drum_echo_gain+=air_current[EN_DRUM_TRACK][1]*fm_maxf(0,.7f-drum_echo_gain)*.4f;
 drum_room_target+=air_current[EN_DRUM_TRACK][0]*fm_maxf(0,.9f-drum_room_target)*.4f;
 drum_damping=fm_minf(1,drum_damping+air_current[EN_DRUM_TRACK][2]*.65f);
 float drum_filter=.025f+(1-drum_damping)*.28f;
 if(en_playing&&!en_frozen){universe_samples+=n;while(universe_samples>=(uint32_t)EN_SR){universe_samples-=(uint32_t)EN_SR;en_years++;}}
 for(int q=0;q<16;q++)if(strum_notes[q].due&&(int32_t)(clock_samples-strum_notes[q].due)>=0){strum_note_t v=strum_notes[q];strum_notes[q].due=0;int l=v.layer&7;generated_strike=(v.layer&16u)?((v.layer&8u)?2:1):0;strum_shared=(uint8_t)(v.layer>>7);if(v.layer&64u){manual_pitch=1;manual_keyboard=(v.layer&32u)!=0;manual_octave=(int)fm_floorf((float)v.degree/7);live_strike=1;}midi_tag=v.midi_tag;if(!en_death[l]&&(uint16_t)en_track_revision[l]==v.epoch&&audible(l))start_voice(l,v.degree,v.velocity);midi_tag=0;strum_shared=0;manual_pitch=manual_keyboard=live_strike=generated_strike=0;}
 for(k=0;k<64 && plat_midi_in(&pkt);k++){
  unsigned st=(pkt>>8)&255u,a=(pkt>>16)&127u,v=(pkt>>24)&127u;
  if(st>=0xf8u){if(en_sync_role<2||((pkt>>4)&15u)==(en_sync_role==2?2u:0u)){if(st==0xF8)galaxy_clock(EN_CLK_TICK);else if(st==0xFA)galaxy_clock(EN_CLK_START);else if(st==0xFB)galaxy_clock(EN_CLK_CONTINUE);else if(st==0xFC)galaxy_clock(EN_CLK_STOP);}}
  else {unsigned source=(pkt>>4)&15u;if(source<3)midi_channel_message(source,st,a,v);}
 }
 if(en_sync_role==1&&master_was_playing!=en_playing){en_midi_out(en_playing?(master_has_started?0xfb:0xfa):0xfc,0,0);master_was_playing=en_playing;if(en_playing)master_has_started=1;}
 for(int l=0;l<EN_NGAL;l++)core_allowed|=!par[LP(l,L_MUTE)];
 if(clock_samples-last_clock_sample>22050u){en_ext=0;ext_ticks=local_ticks=0;clock_seen=0;if(en_sync_role>=2)en_playing=0;}
 en_bpm=en_ext&&clock_interval?60.0f*EN_SR/(24.0f*(float)clock_interval):(float)((par[P_REST]==EN_NIGHTFALL)&&par[P_TEMPO]>48?48:par[P_TEMPO]);
 if(par[P_STYLE]==EN_CINEMATIC&&en_playing&&!en_frozen){uint32_t steps=sequence_step-cinematic_start_step;en_cinematic_stage=(steps/128u)&3u;float dt=(float)n/EN_SR;en_cinematic_energy+=(cinematic_form_target(steps)-en_cinematic_energy)*dt/(2+dt);}

 if(en_playing&&!en_frozen){float dt=(float)n/EN_SR;wind_depth=(par[P_REST]==EN_NIGHTFALL)?fm_minf(1,wind_depth+dt/180):fm_maxf(0,wind_depth-dt/12);}
 float wind_tone=.20f-.08f*wind_depth,wind_blend=.30f*wind_depth;

 field_samples+=n;while(field_samples>=(uint32_t)(EN_SR/60)){field_samples-=(uint32_t)(EN_SR/60);field_advance(1.0f/60.0f);bloom_advance(1.0f/60.0f);}
 if(en_playing&&!en_frozen){for(int q=0;q<4;q++)en_discharge[q].age=fm_maxf(0,en_discharge[q].age-(float)n/EN_SR);for(int q=0;q<8;q++)en_cue_age[q]=fm_maxf(0,en_cue_age[q]-(float)n/EN_SR);}
 if(bowl_clock>n)bowl_clock-=n;else bowl_clock=0;
 if(impact_clock>n)impact_clock-=n;else impact_clock=0;if(core_clock>n)core_clock-=n;else core_clock=0;
 /* Physical events influence the next phrase, never enqueue immediate voices.
  * Coalesce pending bits; stale events cannot drain as a rapid note backlog. */
 if(en_playing&&!en_frozen){field_pending|=(uint8_t)(bowl_pending|chime_pending|bloom_pending);if(core_pending>0)field_pending|=1u;}
 bowl_pending=chime_pending=bloom_pending=object_bowl_mask=0;core_pending=0;
 if(en_playing&&!en_frozen)for(k=0;k<8;k++)en_energy[k]*=0.98f;
 float cosmic_dt=(float)n/EN_SR;if(en_playing&&!en_frozen){cosmic_bloom=fm_maxf(0,cosmic_bloom-cosmic_dt/10);for(int q=0;q<7;q++){
  float target=1;if(en_grb_phase==2&&q==grb_target)target=1-.75f*ease(fm_minf(1,grb_feed_age/3.8f));
  if(cosmic_tail[q]>0){cosmic_tail[q]=fm_maxf(0,cosmic_tail[q]-cosmic_dt);target=.25f*(cosmic_tail[q]/8);}
  else if(en_death[q]>0&&(en_grb_mask&(1u<<q)))target=0;
  cosmic_gain[q]+=(target-cosmic_gain[q])*fm_minf(1,cosmic_dt*8);
 }}
 /* Slowly independent spectral swells, computed once per block. No pitch modulation. */
 int ref=en_reference();float swell[8];
 for(int l=0;l<8;l++)swell[l]=fm_sin_turns((float)clock_samples/EN_SR*reference[ref].swell+(float)l*.137f);
 for(int l=0;l<8;l++){
  block_pan[l]=(float)par[LP(l,L_PAN)]*.01f+en_field_x[l]*(.25f+en_nebula*.12f)+shock_s[l]*fm_sin_turns(en_field_time*.07f+(float)l*.13f)*.28f;
  block_carrier[l]=1+swell[l]*(ref==1?.24f:.12f);
  block_partial[l]=1-swell[l]*(ref==1?.30f:.18f);
  if(ref==1&&l!=1){float breath=.70f+.30f*fm_sin_turns((float)clock_samples/EN_SR*.019f);block_carrier[l]*=breath;block_partial[l]*=breath;}
 }
 const float shock_stride_k=EN_WORKER_FRAMES==64u?.0047886775f:1-fm_powf(.999925f,(float)EN_WORKER_FRAMES);
 const float reverb_target=(float)par[P_REVERB]*0.0038f;
 const float echo_target=(float)par[P_ECHO]*0.0030f;
 const float midi_increment=en_bpm*(24.0f/(60.0f*EN_SR)),step_increment=en_bpm*(4.0f/(60.0f*EN_SR));
 const uint32_t echo_delay=en_echo_delay(en_bpm);
 dsp_stage(1,dsp_elapsed(stage_start));loop_start=dsp_stamp();
 for(j=0;j<n;j++){

  float l=0,r=0,rs=0,drum_send=0,esl=0,esr=0;
#ifdef EN_HOST
  float buses[6]={0};
#endif
  clock_samples++;
  if(!(j&(EN_WORKER_FRAMES-1u)))timing_service();
  if(en_sync_role==1&&en_playing){midi_clock_phase+=midi_increment;if(midi_clock_phase>=1){midi_clock_phase-=1;en_midi_out(0xf8,0,0);}}
  /* Shock is consumed only at worker boundaries. Advance the identical
   * one-pole target in worker-sized chunks instead of8 updates per sample. */
  if(!(j&(EN_WORKER_FRAMES-1u))){float k=j?shock_stride_k:.000075f;for(int g=0;g<8;g++)shock_s[g]+=(en_shock[g]*((par[P_REST]==EN_NIGHTFALL)?.65f:1)-shock_s[g])*k;}
  gate+=((en_playing?1.0f:0.0f)-gate)*(en_playing?0.0001f:0.001f);
  mute_gain+=((muted?0.0f:1.0f)-mute_gain)*0.001f;
  master_s+=((float)gain/4096.0f-master_s)*0.0005f;
  if(en_playing){if(en_ext){if(!(j&(EN_WORKER_FRAMES-1u))&&ext_ticks-local_ticks>=6u){uint32_t due=(ext_ticks-local_ticks)/6u;if(due>1u){local_ticks+=(due-1u)*6u;en_clock_coalesced+=due-1u;}local_ticks+=6u;step();}}else{step_phase+=step_increment;if(!(j&(EN_WORKER_FRAMES-1u))&&step_phase>=1){step_phase-=1;step();}}}
  /* Routing changes at command/block boundaries and sequencer steps, not per
   * oscillator sample. Refresh after step() so scene/mute changes stay exact. */
  if(routing_dirty||routing_step!=sequence_step){for(int l=0;l<8;l++)layer_level[l]=((cosmic_tail[l]>0?!(en_track_flags(l)&(EN_TRACK_MUTED|EN_TRACK_SOLO_OUT|EN_TRACK_SCENE_OUT)):audible(l)))?(float)par[LP(l,L_LEVEL)]*0.01f:0;routing_step=sequence_step;routing_dirty=0;}
  if(!(j&(EN_WORKER_FRAMES-1u))){
   runtime_ctx.audition=0;
   runtime_ctx.n=n-j<EN_WORKER_FRAMES?n-j:EN_WORKER_FRAMES;
   runtime_ctx.vib_depth=(float)par[P_VIBRATO]*(.69314718056f/1200);runtime_ctx.vib_rate=(float)par[P_VIB_RATE]/EN_SR;runtime_ctx.vib_delay=(uint32_t)(par[P_VIB_DELAY]*.01f*EN_SR);runtime_ctx.night=night_mix;runtime_ctx.breath=en_breath;
   runtime_ctx.drum_filter=drum_filter;runtime_ctx.drum_pan=(float)par[LP(EN_DRUM_TRACK,L_PAN)]*.01f;runtime_ctx.drum_level=(par[P_DRUMS]&&audible(EN_DRUM_TRACK))?(float)par[P_DRUM_LEVEL]*.01f:0;
   runtime_ctx.speaker=speaker_mix;runtime_ctx.core_allowed=core_allowed;runtime_ctx.ref=ref;runtime_ctx.drum_echo=drum_echo_gain;
   float width=width_s,target_width=(float)par[P_WIDTH]*.01f;for(unsigned s=0;s<runtime_ctx.n;s++){runtime_ctx.width[s]=width;width+=(target_width-width)*.00004f;}width_s=width;
   for(int q=0;q<8;q++){runtime_ctx.level[q]=layer_level[q];runtime_ctx.live_level[q]=(en_track_flags(q)&(EN_TRACK_MUTED|EN_TRACK_SOLO_OUT))?0:(float)par[LP(q,L_LEVEL)]*.01f;runtime_ctx.presence[q]=cosmic_tail[q]>0?1:en_presence[q];runtime_ctx.cosmic[q]=cosmic_gain[q];runtime_ctx.bloom[q]=(q==cosmic_last?cosmic_bloom:0);runtime_ctx.shock[q]=shock_s[q];runtime_ctx.pan[q]=block_pan[q];runtime_ctx.carrier[q]=block_carrier[q];runtime_ctx.partial[q]=block_partial[q];runtime_ctx.balance[q]=en_balance[q]*en_duck[q];if(par[P_STYLE]==EN_CINEMATIC){float e=en_cinematic_energy;runtime_ctx.balance[q]*=q==1?.75f+.25f*e:q==7?.55f+.35f*e:(q==0||q==4||q==6)?.70f+.30f*e:.55f+.45f*e;}runtime_ctx.swell[q]=swell[q];runtime_ctx.reverb[q]=(float)par[LP(q,L_REVERB)];runtime_ctx.echo[q]=(float)par[LP(q,L_ECHO)];if(q<7){runtime_ctx.reverb[q]=fm_minf(100,runtime_ctx.reverb[q]+air_current[q][0]*45);runtime_ctx.echo[q]=fm_minf(100,runtime_ctx.echo[q]+air_current[q][1]*40);}if(!runtime_ctx.attack[q]||runtime_attack[q]!=par[LP(q,L_ATTACK)]){runtime_attack[q]=par[LP(q,L_ATTACK)];runtime_ctx.attack[q]=1-fm_expf(-1/((.005f+(float)runtime_attack[q]*.02f)*EN_SR));}if(!runtime_ctx.release[q]||runtime_release[q]!=par[LP(q,L_RELEASE)]){runtime_release[q]=par[LP(q,L_RELEASE)];runtime_ctx.release[q]=fm_expf(-6.9f/((.02f+(float)runtime_release[q]*.03f)*EN_SR));}}
   unsigned modulation=air_target_dest>=4&&air_target>0;
   unsigned filter_on=modulation||air_manual_cut||air_manual_res;
   if(air_filter_mix==0&&filter_on){
    air_filter_track=modulation?air_target_track:air_manual_track;
    for(unsigned c=0;c<2;c++)air_filter_low[c]=air_filter_band[c]=0;
   }
   runtime_ctx.filter_track=air_filter_mix>0||filter_on?(int)air_filter_track:-1;
   {uint32_t t=dsp_stamp();runtime_prepare();voice_us+=dsp_elapsed(t);}
   for(unsigned q=0;q<8;q++)for(unsigned u=0;u<runtime_ctx.n;u++){float a=runtime_lane[0][u][q]+runtime_lane[1][u][q];lane_square[q]+=a*a;lane_peak[q]=fm_maxf(lane_peak[q],fm_fabsf(a));}
   for(unsigned q=0;q<8;q++){
    block_peak[q]=fm_maxf(block_peak[q],runtime_peak[0][q]+runtime_peak[1][q]);
    float bound=runtime_meter[0][q]+runtime_meter[1][q];reactive_peak[q]=fm_maxf(reactive_peak[q],bound);
    /* Track meters are post-fader/pan/auto-mix, pre-master and before the
     * stereo peak guard. Include the real4x output gain; don't hide overload. */
    duck_peak[q]=fm_maxf(duck_peak[q],bound);
    float output_bound=bound*4*gate*mute_gain*night_gain;meter_peak[q]=fm_maxf(meter_peak[q],output_bound);if(output_bound>.86f)meter_clip[q]=44100;
   }
  }
  unsigned sample=j&(EN_WORKER_FRAMES-1u);
  float *m0=runtime_mix[0][sample],*m1=runtime_mix[1][sample];
  float environment=soundscape_tick();l=m0[0]+m1[0]+environment;r=m0[1]+m1[1]+environment*.97f;rs=m0[2]+m1[2];esl=m0[3]+m1[3];esr=m0[4]+m1[4];drum_send=m0[5]+m1[5];
#ifdef EN_HOST
  for(unsigned q=0;q<6;q++)buses[q]=runtime_bus[0][sample][q]+runtime_bus[1][sample][q];
#endif
  if(j+1==n&&runtime_ctx.filter_track<0)air_filter_readout=0;
  if(runtime_ctx.filter_track>=0){
   int selected=air_filter_track==air_target_track&&air_target_dest>=4;
   float amount=selected?(float)air_target/32767.0f:0;
   int manual=air_filter_track==air_manual_track&&(!air_target||air_target_dest<4||selected);
   float cut=manual?air_manual_cut*.01f:0,res=manual?air_manual_res*.01f:0;
   float desired=amount>0||cut>0||res>0?1.0f:0.0f;
   air_filter_mix+=(desired-air_filter_mix)*.002f;
   if(!desired&&air_filter_mix<.0001f)air_filter_mix=0;
   float coefficient=.32f-.27f*fm_minf(1,cut+(air_target_dest==4?amount:0));
   float damping=1.25f-.50f*fm_minf(1,res+(air_target_dest==5?amount:0));
   if(j+1==n){
    unsigned cutoff=(unsigned)(fm_minf(1,cut+(air_target_dest==4?amount:0))*100+.5f);
    unsigned resonance=(unsigned)(fm_minf(1,res+(air_target_dest==5?amount:0))*100+.5f);
    air_filter_readout=air_filter_mix>0?((air_filter_track+1)<<16)|(resonance<<8)|cutoff:0;
   }
   float correction[2];
   for(unsigned c=0;c<2;c++){
    float dry=runtime_filter_dry[0][sample][c]+runtime_filter_dry[1][sample][c];
    air_filter_low[c]+=coefficient*air_filter_band[c];
    air_filter_band[c]+=coefficient*(dry-air_filter_low[c]-damping*air_filter_band[c]);
    correction[c]=(air_filter_low[c]-dry)*air_filter_mix;
   }
   l+=correction[0];r+=correction[1];
  }
  /* Display meters are published at worker boundaries, outside this hot loop. */

  wetrev+=(reverb_target-wetrev)*0.00004f;
  wetecho+=(echo_target-wetecho)*0.00004f;
  float fx[6];space_tick(esl,esr,rs,drum_send,echo_delay,fx);
  float el=fx[0],er=fx[1],rl=fx[2],rr=fx[3],da=fx[4],db=fx[5];
  drum_wet+=(drum_room_target-drum_wet)*.00015f;float dw=drum_wet;
  float wl=el*wetecho+rl*(wetrev+cosmic_bloom*.15f)+da*dw,wr=er*wetecho+rr*(wetrev+cosmic_bloom*.15f)+db*dw;
  dry_square+=(l*l+r*r)*.5f*gate*gate;wet_square+=(wl*wl+wr*wr)*.5f*gate*gate;
  l=(l+wl)*gate+(runtime_ctx.audition?(m0[0]+m1[0])*(1-gate):0);r=(r+wr)*gate+(runtime_ctx.audition?(m0[1]+m1[1])*(1-gate):0);
  {float sample=(l+r)*.5f;band_low+=.014f*(sample-band_low);band_mid+=.18f*(sample-band_mid);band_sum[0]+=fm_fabsf(band_low);band_sum[1]+=fm_fabsf(band_mid-band_low);band_sum[2]+=fm_fabsf(sample-band_mid);}
  dc_l+=0.0005f*(l-dc_l);dc_r+=0.0005f*(r-dc_r);l-=dc_l;r-=dc_r;
  night_mix+=(((par[P_REST]==EN_NIGHTFALL)?1.0f:0.0f)-night_mix)*0.00002f;
  float rest_target=1;

  night_gain+=(rest_target-night_gain)*0.00002f;
  l*=night_gain;r*=night_gain;
  output_profile(&l,&r);
  if(wind_depth>0){wind_l+=(l-wind_l)*wind_tone;wind_r+=(r-wind_r)*wind_tone;l+=(wind_l-l)*wind_blend;r+=(wind_r-r)*wind_blend;}
  else{wind_l=l;wind_r=r;}

  l*=master_s*mute_gain*4.0f;r*=master_s*mute_gain*4.0f;
  /* Stereo-linked peak guard: no waveshaping, no pitch changes, slow recovery. */
  float peak=fm_maxf(fm_fabsf(l),fm_fabsf(r)),limit=peak>0.86f?0.86f/peak:1;
  if(limit<1)en_master_clip=44100;
  if(limit<en_mix_limit)en_mix_limit=limit;else en_mix_limit+=(limit-en_mix_limit)*0.00003f;
  l*=en_mix_limit;r*=en_mix_limit;
  out[2*j]=(int32_t)(l*8388607.0f);out[2*j+1]=(int32_t)(r*8388607.0f);
#ifdef EN_HOST
  if(en_bus_on&&j<1024){for(k=0;k<6;k++)en_bus[j*6+k]=buses[k];en_bus[j*6+5]=(rl+rr)*wetrev+(el+er)*wetecho;}
#endif
 }
 {uint32_t total=dsp_elapsed(loop_start);dsp_stage(2,voice_us);dsp_stage(3,total>voice_us?total-voice_us:0);}
 unsigned shock_tail=n?((n-1u)&(EN_WORKER_FRAMES-1u)):0;if(shock_tail){float k=shock_tail==63u?.004714031f:1-fm_powf(.999925f,(float)shock_tail);for(int g=0;g<8;g++)shock_s[g]+=(en_shock[g]*((par[P_REST]==EN_NIGHTFALL)?.65f:1)-shock_s[g])*k;}
 /* Publish display meters once per audio block; envelope arithmetic above is unchanged. */
 for(unsigned q=0;q<8;q++){en_peak[q]=meter_peak[q];en_clip[q]=meter_clip[q];en_lane_rms[q]=n?fm_sqrtf(lane_square[q]/n)*4*gate*mute_gain*night_gain:0;en_lane_peak[q]=lane_peak[q]*4*gate*mute_gain*night_gain;}
 en_dry_rms=n?fm_sqrtf(dry_square/n)*4:0;en_wet_rms=n?fm_sqrtf(wet_square/n)*4:0;
 unsigned active_voices=0;
 uint8_t held_lanes=0;
 for(unsigned q=0;q<NV;q++)if(voice[q].on){
  active_voices++;
  if(voice[q].held&&voice[q].layer<7)held_lanes|=(uint8_t)(1u<<voice[q].layer);
 }
 en_active_voices=active_voices;
 en_held_lanes=held_lanes;
 for(k=0;k<8;k++){
  float dt=(float)n/EN_SR,level=(float)par[k==EN_DRUM_TRACK?P_DRUM_LEVEL:LP(k,L_LEVEL)]*0.01f;
  balance_peak[k]=fm_maxf(block_peak[k],balance_peak[k]*fm_maxf(0,1-dt*0.25f));
  /* Slow bounded compensation lifts naturally quiet timbres without undoing faders.
   * Never chase silence, auto-unmute, or boost bass to match a bell's peak. */
  /* Anchor and intention shape headroom, never oscillator pitch or faders. */
  static const float intent_headroom[10]={.80f,1.0f,.88f,.96f,1.02f,.90f,.92f,.88f,1.0f,.82f};
  float calm=intent_headroom[mix_intention]*((par[P_REST]==EN_NIGHTFALL)?.78f:par[P_ANCHOR]>=8?.90f:1.0f);
  float desired=1;if(par[P_AUTO_LEVEL]){if(balance_peak[k]>0.002f&&level>0)desired=fm_clampf((k==7?0.15f:0.085f)*calm*level/balance_peak[k],0.15f,(k==1||k==EN_DRUM_TRACK)?1.0f:1.8f);if(k==1)desired=fm_minf(0.90f,desired);if(en_grb_phase==2&&k==grb_target)desired=fm_minf(1,desired);}
  float duck=1;if(par[P_AUTO_DUCK]&&k!=1&&k!=EN_DRUM_TRACK){float focus=fm_maxf(duck_peak[1],duck_peak[7]*.6f);duck=1-fm_clampf(focus*(1.15f-intent_headroom[mix_intention]*.45f)*((par[P_REST]==EN_NIGHTFALL)?1.0f:.85f)*(par[P_ANCHOR]>=8?.85f:1),0,.28f);}
  if(par[P_AUTO_DUCK])en_duck[k]+=(duck-en_duck[k])*fm_minf(1,dt*(duck<en_duck[k]?12:2));else en_duck[k]=1;
  if(par[P_AUTO_LEVEL])en_balance[k]+=(desired-en_balance[k])*fm_minf(1,dt*(desired<en_balance[k]?20.0f:.25f));else en_balance[k]=1;
  if(en_playing&&!en_frozen)en_visual[k]=fm_maxf(en_visual[k]*.92f,reactive_peak[k]*gate);
  en_meter[k]=fm_maxf(en_meter[k]*0.92f,block_peak[k]*en_balance[k]*gate*master_s*mute_gain*night_gain);
 }
}

/* Output adapter: real44.1/48kHz synthesis; codec transport stays44.1kHz.
 * 48kHz is explicitly a processing mode, not a false native-codec claim.
 * Rational160/147 conversion preserves physical pitch and tempo. */
static int32_t bridge_pcm[128],bridge_a[2],bridge_b[2],bridge_last[2];
static unsigned bridge_read=64,bridge_fraction,bridge_primed,bridge_fade;
static void bridge_cold(void){bridge_last[0]=bridge_last[1]=0;}
static void bridge_reset(void){bridge_read=64;bridge_fraction=bridge_primed=0;bridge_fade=64;}
static void bridge_next(int32_t *out,uint32_t gain){if(bridge_read>=64){en_render(bridge_pcm,64,gain);bridge_read=0;}out[0]=bridge_pcm[bridge_read*2];out[1]=bridge_pcm[bridge_read*2+1];bridge_read++;}
void en_output_render(int32_t *out,uint32_t n,uint32_t gain){if(!n)return;commands();
 if(EN_SR==44100){en_render(out,n,gain);}else{
  if(!bridge_primed){bridge_next(bridge_a,gain);bridge_next(bridge_b,gain);bridge_primed=1;}
  for(unsigned i=0;i<n;i++){for(unsigned c=0;c<2;c++)out[2*i+c]=(int32_t)((float)bridge_a[c]+((float)bridge_b[c]-bridge_a[c])*(float)bridge_fraction*(1.0f/147));
   bridge_fraction+=160;while(bridge_fraction>=147){bridge_fraction-=147;bridge_a[0]=bridge_b[0];bridge_a[1]=bridge_b[1];bridge_next(bridge_b,gain);}
  }
 }
 for(unsigned i=0;i<n;i++)for(unsigned c=0;c<2;c++){if(bridge_fade){float q=(float)bridge_fade*(1.0f/64);out[2*i+c]=(int32_t)((float)out[2*i+c]*(1-q)+(float)bridge_last[c]*q);}if(c==1&&bridge_fade)bridge_fade--;}
 bridge_last[0]=out[2*n-2];bridge_last[1]=out[2*n-1];
}

void en_response(int layer,en_response_t *r){
 *r=(en_response_t){0};r->echo_seconds=space_delay_samples/(EN_SR*.5f);
 r->dry_rms=en_dry_rms;r->wet_rms=en_wet_rms;r->echo_mix=wetecho;r->room_mix=wetrev+cosmic_bloom*.15f;r->echo_feedback=space_echo_feedback;r->room_feedback=space_feedback;r->room_predelay=space_predelay_samples/(EN_SR*.5f);r->room_damping=space_damping;r->room_mod=(float)par[P_ROOM_MOD]*.01f;r->room_cycle=(float)(ROOM_LEN[0]+ROOM_LEN[1]+ROOM_LEN[2]+ROOM_LEN[3])/(4*EN_SR*.5f);r->width=width_s;r->lane_send=(float)par[LP(layer,L_REVERB)]*.007f;r->lane_echo=(float)par[LP(layer,L_ECHO)]*.007f;r->glide_ms=(float)par[P_GLIDE];r->vibrato_delay=(float)par[P_VIB_DELAY]*.01f;const role_patch_t *recipe=role_patch(par[LP(layer,L_VOICE)]);if(recipe)r->lane_send*=recipe->send;
 r->vibrato_hz=runtime_ctx.vib_rate*EN_SR;r->vibrato_cents=runtime_ctx.vib_depth*(1200/.69314718056f);
 for(int i=0;i<NV;i++)if(voice[i].on&&voice[i].layer==layer&&!voice[i].drum&&!voice[i].sfx&&patch_expressive(voice[i].patch,layer)){
  r->active=1;r->glide_ratio=voice[i].pitch_ratio;r->lane_send=(float)par[LP(layer,L_REVERB)]*.007f*voice[i].rev*(voice[i].strummed?1.1f:1);r->lane_echo*=voice[i].strummed?1.05f:1;r->phase=voice[i].vib_phase;r->vibrato_ramp=voice[i].age>runtime_ctx.vib_delay?fm_minf(1,(float)(voice[i].age-runtime_ctx.vib_delay)/(EN_SR*.4f)):0;break;}
}

unsigned en_phrase_snapshot(int layer,en_phrase_event_t out[16],uint32_t *now){*now=clock_samples;if((unsigned)layer>=EN_NGAL)return 0;uint32_t w=phrase_written[layer],n=w<16?w:16;for(unsigned i=0;i<n;i++)out[i]=phrase_events[layer][(w-n+i)&15u];BARRIER();return w==phrase_written[layer]?n:0;}
unsigned en_motif_template(int layer,uint8_t out[64]){if((unsigned)layer>=EN_NGAL)return 0;unsigned n=gx_len(par[LP(layer,L_STEPS)]);for(unsigned i=0;i<n;i++)out[i]=motif_memory[layer][i];return n;}
void en_evolution_snapshot(en_evolution_t *out){float e=0;for(int l=0;l<7;l++)e+=en_visual[l];*out=(en_evolution_t){clock_samples,sequence_step,par[P_EVOLVE],par[P_BARS],par[P_AUTO],par[P_DENSITY],en_scene,current_harmony(),fm_minf(1,e*2)};}
