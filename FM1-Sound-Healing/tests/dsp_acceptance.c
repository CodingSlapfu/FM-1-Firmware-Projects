#include "serenity.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static sr_engine e;
static int16_t out[512];
static void quiet(void){sr_init(&e,731);for(int t=0;t<SR_TRACKS;t++)e.tracks[t].mute=1;e.drums_enabled=0;e.bed_level=0;e.reverb_amount=e.delay_amount=0;e.tone_level=0;}
static void stop_test(void){sr_init(&e,1);sr_start(&e);e.binaural_enabled=1;for(int b=0;b<1000;b++)sr_render(&e,out,256);sr_stop(&e);for(int b=0;b<8000;b++)sr_render(&e,out,256);int peak=0;for(int i=0;i<512;i++)if(abs(out[i])>peak)peak=abs(out[i]);assert(peak<=1);printf("Stop residual peak %d/32768\n",peak);}
static void frequencies(void){for(int f=0;f<SR_FREQUENCIES;f++){quiet();e.frequency=f;e.tone_level=.1f;sr_start(&e);for(int b=0;b<200;b++)sr_render(&e,out,256);double re=0,im=0;double hz=sr_frequencies[f].hz;for(int n=0;n<SR_RATE;n++){sr_render(&e,out,1);double a=6.283185307179586*hz*n/SR_RATE;re+=out[0]*cos(a);im+=out[0]*sin(a);}assert(hypot(re,im)>SR_RATE*800.);printf("Tone %.1f Hz: spectral component verified\n",hz);}}
int main(void){frequencies();stop_test();sr_init(&e,987);sr_start(&e);e.energy=1;e.variation=1;e.drums_enabled=1;e.drum_density=3;e.drum_level=1;e.binaural_enabled=1;e.binaural_level=.1f;e.bed_level=1;e.reverb_amount=.8f;e.delay_amount=.7f;clock_t begin=clock();int peak=0;for(int b=0;b<10336;b++){if(b%32==0){for(int t=0;t<8;t++){sr_set_track_patch(&e,t,(b/32+t)%SR_PATCHES);sr_note_on(&e,t,36+t*5,127,3);}}e.bed_type=1+(b/128)%7;sr_render(&e,out,256);for(int i=0;i<512;i++){int v=abs(out[i]);if(v>peak)peak=v;assert(v<32767);}for(int j=0;j<SR_VOICES;j++){assert(isfinite(e.voices[j].env));assert(isfinite(e.voices[j].phase));}}printf("Worst combination: 60s host render %.3fs CPU, peak %d, engine %zu bytes; NOT FM1 timing\n",(double)(clock()-begin)/CLOCKS_PER_SEC,peak,sizeof(e));puts("PASS DSP acceptance");return 0;}
