/* SPDX-License-Identifier: MPL-2.0
 * Original Sound Healing application. Audio owns engine state; UI produces a bounded queue. */
#include "serenity.h"
#include "fastmath.h"
static sr_engine engine PLAT_POOL;
static int16_t pcm[APP_BLOCK*2] PLAT_POOL;
typedef struct {int kind,value; unsigned track;} command;
static command queue[64];
static volatile unsigned qread,qwrite,state_revision;
static unsigned displayed_revision;
static unsigned dropped,page,selected,group,oldkeys,oldbuttons,lastdraw,dirty=1,debug;
static float output_gain;
static unsigned audio_max,audio_count,audio_sum,audio_hist[64],p95,p99,mean_us,window_ms;
#include "visual_worker.c"
static int bounded(int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;}
static float adjust(float x,int d,float step,float hi){return fm_clampf(x+d*step,0,hi);}
static void send(int kind,int value){unsigned next=(qwrite+1)&63;if(next==qread){dropped++;return;}queue[qwrite]=(command){kind,value,selected};__asm__ volatile("":::"memory");qwrite=next;}
static void consume(void){unsigned count=0;while(qread!=qwrite&&count++<64){command c=queue[qread];__asm__ volatile("":::"memory");qread=(qread+1)&63;sr_track *t=&engine.tracks[c.track];int d=c.value;
 state_revision++;
 switch(c.kind){
 case 0:if(engine.running)sr_stop(&engine);else sr_start(&engine);break;
 case 1:sr_set_scene(&engine,bounded(engine.scene+d,0,SR_SCENES-1));break;
 case 2:sr_set_track_patch(&engine,c.track,bounded(t->patch+d,0,SR_PATCHES-1));break;
 case 3:sr_note_on(&engine,c.track,d,96,1.f);break;
 case 4:sr_set_frequency(&engine,bounded(engine.frequency+d,0,SR_FREQUENCIES-1));break;
 case 5:engine.bed_level=adjust(engine.bed_level,d,.02f,1);break;
 case 6:sr_set_binaural(&engine,!engine.binaural_enabled);break;
 case 7:sr_set_binaural_rate(&engine,fm_clampf(engine.binaural_rate+d*.5f,.5f,40));break;
 case 8:sr_set_binaural_carrier(&engine,fm_clampf(engine.binaural_carrier+d*5.f,80,1000));break;
 case 9:sr_set_drums(&engine,!engine.drums_enabled);break;
 case 10:sr_set_drum_density(&engine,bounded(engine.drum_density+d,0,3));break;
 case 11:engine.reverb_amount=adjust(engine.reverb_amount,d,.02f,.8f);break;
 case 12:engine.delay_amount=adjust(engine.delay_amount,d,.02f,.7f);break;
 case 13:t->volume=adjust(t->volume,d,.03f,1);break;
 case 14:sr_set_tuning_432(&engine,!engine.tuning_432);break;
 case 15:t->mute=!t->mute;break;
 case 16:sr_set_drum_kit(&engine,bounded(engine.drum_kit+d,0,SR_DRUM_KITS-1));break;
 case 17:sr_set_drum_level(&engine,engine.drum_level+d*.02f);break;
 case 18:engine.energy=adjust(engine.energy,d,.05f,1);break;
 case 19:engine.variation=adjust(engine.variation,d,.05f,1);break;
 case 20:engine.freeze=!engine.freeze;break;
 case 21:engine.bed_type=bounded(engine.bed_type+d,0,7);break;
 case 22:engine.tone_level=adjust(engine.tone_level,d,.005f,.10f);break;
 case 23:engine.binaural_level=adjust(engine.binaural_level,d,.005f,.10f);break;
 case 24:t->pan=fm_clampf(t->pan+d*.05f,-1,1);break;
 case 25:engine.tick_length=bounded((int)engine.tick_length-d*80,5512,22050);break;
 }
}}
void app_init(void){sr_init(&engine,7301);plat_cpu2_set(1);output_gain=0;qread=qwrite=0;page=selected=group=debug=0;oldkeys=oldbuttons=0;dirty=1;}
void app_panic(void){engine.running=0;engine.master=0;output_gain=0;}
void app_render(int32_t *lr,uint32_t n,uint32_t master){unsigned begin=plat_ticks24();consume();if(n>APP_BLOCK)n=APP_BLOCK;sr_render(&engine,pcm,n);for(unsigned i=0;i<n;i++){output_gain+=((float)master/4096.f-output_gain)*.002f;if(!master)output_gain=0;lr[2*i]=(int32_t)(pcm[2*i]*256.f*output_gain);lr[2*i+1]=(int32_t)(pcm[2*i+1]*256.f*output_gain);}unsigned us=(plat_ticks24()-begin)/24;if(us>audio_max)audio_max=us;audio_sum+=us;audio_count++;audio_hist[us/100<63?us/100:63]++;}
void app_say(const char*a,const char*b){(void)a;(void)b;dirty=1;}
void app_input_only(void){unsigned b=plat_buttons(),k=plat_keys(),press=b&~oldbuttons;oldbuttons=b;
 if(press&(1u<<B_PLAY))send(0,0);
 if(press&(1u<<B_HOME)){page=0;debug=0;}
 if(press&(1u<<B_SEL))send(15,0);
 if(press&(1u<<B_ARP))send(9,0);
 if(press&(1u<<B_GLO))debug=!debug;
 if(press&(1u<<B_REC))plat_cpu2_set(0);
 if(press&(1u<<B_ENV))send(14,0);
 if(press&(1u<<B_FX))send(6,0);
 if(press&(1u<<B_SEQ))send(20,0);
 if(press&(1u<<B_EDIT))group=(group+1)%5;
 for(unsigned i=0;i<NKEYS;i++){if((k&~oldkeys)&(1u<<i))send(3,53+i);}oldkeys=k;
 int d=plat_enc(EN_ALGO);if(d){page=bounded((int)page+d,0,3);dirty=1;}
 d=plat_enc(EN_SELECT);if(d){selected=bounded((int)selected+d,0,7);dirty=1;}
 d=plat_enc(EN_PRESET);if(d){send(page==0?1:2,d>0?1:-1);dirty=1;}
 static const int sound[5][4]={{2,13,24,11},{21,5,11,12},{4,22,14,18},{8,7,23,6},{16,17,10,9}};
 static const int controls[4][4]={{1,18,22,5},{0,0,0,0},{19,20,25,18},{13,24,11,12}};
 for(int i=0;i<4;i++){d=plat_enc(EN_K1+i);if(d){send(page==1?sound[group][i]:controls[page][i],bounded(d,-8,8));dirty=1;}}
 if(press)dirty=1;
 plat_leds((engine.running?(1u<<B_PLAY):0)|(engine.freeze?(1u<<B_SEQ):0),k);
}
#include "ui.c"
void app_ui_init(void){draw();}
void app_frame(void){app_input_only();unsigned now=plat_ms();if(now-window_ms>=2000){unsigned hist[64],count,sum,flags=plat_lock();count=audio_count;sum=audio_sum;for(unsigned i=0;i<64;i++){hist[i]=audio_hist[i];audio_hist[i]=0;}audio_count=audio_sum=0;plat_unlock(flags);if(count){unsigned acc=0;mean_us=sum/count;p95=p99=0;for(unsigned i=0;i<64;i++){acc+=hist[i];if(!p95&&acc*100>=count*95)p95=(i+1)*100;if(!p99&&acc*100>=count*99)p99=(i+1)*100;}}window_ms=now;}if(now-lastdraw>=(plat_cpu_pct()>60?100u:34u)){lastdraw=now;if(displayed_revision!=state_revision)dirty=1;prepare_geometry();draw();}}
#ifdef PLAT_HOST
int app_expect(const char *what,int *got){if(!strcmp(what,"page"))*got=page;else if(!strcmp(what,"running"))*got=engine.running;else if(!strcmp(what,"binaural"))*got=engine.binaural_enabled;else if(!strcmp(what,"track"))*got=selected;else if(!strcmp(what,"drops"))*got=dropped;else return 0;return 1;}
#endif
