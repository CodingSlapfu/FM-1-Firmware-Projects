#ifndef EN_VERSION
#define EN_VERSION "0.23.1"
#endif
/* SPDX-License-Identifier: GPL-3.0-only
 * EIN-1 GALAXY MATRIX: new pearl/teal sci-fi anime observatory UI.
 * Original instrument: live field, sound worlds and an explicit rhythm desk. Created by Ziao. */
#include "../dsp/fastmath.h"

enum{V_MATRIX,V_VOICE,V_MOTIF,V_DRIFT,V_SCENES,V_SPACE,V_INTENT,V_MIXER,V_OBSERVATORY,V_OUTPUT,V_REST,V_TEXT,V_PLANETS,V_DRUMS,V_BASS,V_BLUETOOTH,V_DRUMFX,V_MIXAUTO,V_DEBUG,V_DRUMPARTS,V_EXPRESSION,V_STRUM,V_MOTIF_MODE,V_REVERB_DETAIL,V_ECHO_DETAIL,V_TIMING,V_SOUNDSCAPES,V_COMPOSITION,V_USB_SCREEN,NVIEWS};
static struct{uint8_t view,sel,dirty,full,lane,planet,idle,drum_part,debug_detail;uint32_t activity,ignore_b,ignore_k,master_prev,wake_ms;uint8_t master_samples,wake_reason,sel_expression,timer_view;uint32_t frame,last_b,last_k,raw_last_b,raw_last_k,pressed[NB],msg_until;char msg[32];}ui;
/* Five direct destinations; neither local editors nor fullscreen own ALGORITHM. */
static uint8_t ui_hub,ui_area,ui_topic,ui_edit,ui_mixer_sends,ui_track_controls,ui_bass_controls;
static const char *const UI_AREA[5]={"PLAY","SHAPE","EVOLVE","MIX","SETTINGS"};
static const uint8_t UI_PAGES[5][12]={
 {V_OBSERVATORY},
 {V_VOICE,V_BASS,V_MATRIX,V_PLANETS,V_DRUMS,V_DRUMPARTS,V_EXPRESSION,V_STRUM},
 {V_MOTIF_MODE,V_MOTIF,V_DRIFT,V_SCENES,V_TIMING,V_COMPOSITION},
 {V_MIXER,V_MIXAUTO,V_DRUMFX},
 {V_SPACE,V_REVERB_DETAIL,V_ECHO_DETAIL,V_SOUNDSCAPES,V_TEXT,V_OUTPUT,V_BLUETOOTH,V_USB_SCREEN,V_DEBUG,V_INTENT,V_REST}};
static const uint8_t UI_PAGE_COUNT[5]={1,8,6,3,11};
static void ui_open_area(unsigned area){ui_area=area%5;ui_topic=0;ui.view=UI_PAGES[ui_area][0];ui_hub=ui_edit=ui.idle=0;}
static void ui_locate_page(void){if(ui_area<5)for(unsigned p=0;p<UI_PAGE_COUNT[ui_area];p++)if(UI_PAGES[ui_area][p]==ui.view){ui_topic=p;return;}for(unsigned a=0;a<5;a++)for(unsigned p=0;p<UI_PAGE_COUNT[a];p++)if(UI_PAGES[a][p]==ui.view){ui_area=a;ui_topic=p;return;}ui_open_area(0);}
static void ui_topic_step(int delta){ui_locate_page();ui_topic=(uint8_t)((ui_topic+delta%UI_PAGE_COUNT[ui_area]+UI_PAGE_COUNT[ui_area])%UI_PAGE_COUNT[ui_area]);ui.view=UI_PAGES[ui_area][ui_topic];ui_edit=0;}
static int gx_bound(int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;}
static uint32_t crystal_last_frame;static uint8_t crystal_has_frame;
static int accel_off;static unsigned bt_pick;static uint8_t air_page,air_cutoff,air_resonance;
/* Transport-owned visual clock: wall-clock UI timers remain responsive. */
static uint32_t gx_visual_ms,gx_visual_wall;
static uint32_t gx_animation_ms(void){return gx_visual_ms;}
static uint32_t ui_splash_ms=4800,boot_ms;
/* Intro motion is visual only; never alter the music/physics clock. */
static float gx_intro_turn,gx_intro_object[8],gx_intro_scene_rate,gx_intro_age;
static uint32_t gx_intro_rng;
static float gx_intro_rate(void){gx_intro_rng^=gx_intro_rng<<13;gx_intro_rng^=gx_intro_rng>>17;gx_intro_rng^=gx_intro_rng<<5;return (gx_intro_rng&1u?1.0f:-1.0f)*(0.012f+(float)((gx_intro_rng>>8)&255u)*0.00007f);}

#include "frame_budget.h"
static frame_budget_t ui_budget;
extern uint32_t ui_last_draw_ms;
uint32_t ui_frame_period(void){return 16u; /* Menu/input refresh is not the animation cap. */}
/* Fixed counters only: no real-time logging, allocation or timing in audio workers.
 * Stage values exclude the measured audio ISR time; histograms retain total draw wall time. */
enum{PROF_FOG,PROF_STARS,PROF_TAILS,PROF_GALAXIES,PROF_CORE,PROF_EVENTS,PROF_UI,PROF_FLUSH,PROF_N};
static uint32_t profile_last[PROF_N],profile_worst[PROF_N],profile_frames[2];static uint16_t profile_hist[2][16];
static uint32_t profile_stamp,profile_audio_stamp;
static void profile_begin(void){profile_stamp=plat_ms();profile_audio_stamp=plat_audio_time_us();}
static void profile_stage(unsigned stage){uint32_t wall=plat_ms(),audio=plat_audio_time_us(),ms=wall-profile_stamp,busy=(audio-profile_audio_stamp)/1000u;uint32_t own=ms>busy?ms-busy:0;profile_last[stage]=own;if(own>profile_worst[stage])profile_worst[stage]=own;profile_stamp=wall;profile_audio_stamp=audio;if(stage<PROF_UI)ui_input_only();}
static void profile_record(unsigned mode,unsigned ms){unsigned b=ms/8;if(b>15)b=15;if(profile_frames[mode]>=60000){unsigned count=0;for(unsigned q=0;q<16;q++){profile_hist[mode][q]/=2;count+=profile_hist[mode][q];}profile_frames[mode]=count;}profile_hist[mode][b]++;profile_frames[mode]++;}
static unsigned profile_p95(unsigned mode){if(!profile_frames[mode])return 0;uint32_t count=0,target=(profile_frames[mode]*95u+99u)/100u;for(unsigned b=0;b<16;b++){count+=profile_hist[mode][b];if(count>=target)return (b+1)*8u;}return 128;}
static int gx_lowres;
uint32_t ui_last_draw_ms;static uint32_t ui_fps_start,ui_fps_frames,ui_fps;static uint32_t ui_cpu_draw_ms,ui_audio_start_us,ui_lcd_wait_ms,ui_lcd_start_us;static uint32_t ui_draw_start,ui_track_seen[8],ui_frame_ms,ui_frame_wall;static uint8_t gx_quality=EN_BALANCED_GRAPHICS?1:2,gx_recovery;
static float gx_camera=1,gx_cam_x,gx_cam_y,page_yaw,page_pitch,gx_nova_yaw,gx_nova_pitch;
static uint32_t gx_shot_seen[6];
static float gx_grb_cam_age=30,gx_grb_cam_x,gx_grb_cam_y;static uint32_t gx_grb_seen;
static float gx_nova_age=30,gx_nova_x,gx_nova_y;static uint32_t gx_camera_ms,gx_nova_seen;
#define GX_ZOOM (1.242f*gx_camera)
#define GX_CX (120.0f+gx_cam_x)
#define GX_CY (120.0f+gx_cam_y)
#define GX_SX (118.0f*GX_ZOOM)
#define GX_SY (86.0f*GX_ZOOM)
static uint8_t bt_controller=1;static uint32_t ui_remote_serial;static uint8_t ui_remote_rev[P_NPARAMS];
static const char *const GX_VIEW[NVIEWS]={"ROLES","VOICE","PHRASE","EVOLVE","JOURNEY","SPACE","INTENTION","MIXER","LIVE","OUTPUT","MUSIC MODE","DISPLAY","NOTE MAP","PERCUSSION","BASS","CONNECTIVITY / BT","DRUM SPACE","AUTO MIX","DIAGNOSTICS","DRUM PARTS","EXPRESSION","STRUM EXPRESSION","MOTIF MODES","REVERB DETAIL","ECHO DETAIL","MUSICAL TIMING","LIVING SOUNDSCAPES","STRUM DETAILS","USB SCREEN MIRROR"};

typedef struct{uint16_t bg,panel,line,accent,text,dim,second;} gx_theme_t;
static const gx_theme_t GX_THEMES[4]={
 {RGB(0,8,16),RGB(4,28,38),RGB(0,128,144),RGB(0,255,200),RGB(232,255,248),RGB(96,184,192),RGB(112,64,255)},
 {RGB(0,4,20),RGB(4,16,48),RGB(32,96,192),RGB(24,176,255),RGB(232,248,255),RGB(96,160,224),RGB(152,48,255)},
 {RGB(12,0,20),RGB(32,4,48),RGB(152,32,128),RGB(255,32,184),RGB(255,232,252),RGB(200,112,192),RGB(88,96,255)},
 {RGB(8,8,0),RGB(24,28,4),RGB(112,128,24),RGB(255,200,32),RGB(255,248,224),RGB(176,176,96),RGB(0,232,144)}};
static const gx_theme_t *gx_theme(void){return &GX_THEMES[(unsigned)proj.par[P_THEME]&3u];}
#define GX_BG (gx_theme()->bg)
#define GX_PANEL (gx_theme()->panel)
#define GX_LINE (gx_theme()->line)
#define GX_TEAL (gx_theme()->accent)
#define GX_WHITE (gx_theme()->text)
#define GX_DIM (gx_theme()->dim)
#define GX_VIOLET (gx_theme()->second)
static const uint16_t GX_COLOR[8]={RGB(24,255,204),RGB(32,180,255),RGB(120,96,255),RGB(255,64,200),RGB(48,240,96),RGB(255,152,32),RGB(152,64,255),RGB(32,255,176)};
static void gx_str(char *b,const char *s,int n){int i=0;while(s[i]&&i<n-1){b[i]=s[i];i++;}b[i]=0;}
static void gx_num(char *b,int n){char t[12];int k=0;if(n<0){*b++='-';n=-n;}do{t[k++]=(char)('0'+n%10);n/=10;}while(n);while(k)*b++=t[--k];*b=0;}
static uint16_t gx_mix(uint16_t a,uint16_t b,int t){t=t<0?0:t>16?16:t;int r=((a>>11)*(16-t)+(b>>11)*t)>>4,g=(((a>>5)&63)*(16-t)+((b>>5)&63)*t)>>4,bl=((a&31)*(16-t)+(b&31)*t)>>4;return (uint16_t)(r<<11|g<<5|bl);}
static void gx_shade(int x,int y,int w,int h,uint16_t col,int a){
 if(a<0)a=0;if(a>16)a=16;int x0=x<0?0:x,y0=y<0?0:y,x1=x+w,y1=y+h;if(x1>(int)cv_w)x1=cv_w;if(y1>(int)cv_h)y1=cv_h;if(x0>=x1||y0>=y1)return;
 for(int j=y0;j<y1;j++){uint16_t *at=cv_px+j*cv_w+x0;for(int i=x0;i<x1;i++,at++){
  uint32_t c=swap16(*at);if(!col){unsigned f=16-a;c=((((c&0xf81fu)*f)>>4)&0xf81fu)|((((c&0x07e0u)*f)>>4)&0x07e0u);}else c=gx_mix(c,col,a);*at=swap16(c);
 }}
}

static const felucca_font_t *gx_font(void){return proj.par[P_TEXT]==0?&FONT_XS:proj.par[P_TEXT]==1?&FONT_S:&FONT_T;}

/* Bounded native-pixel renderer. Transparent stars never paint black sprite boxes. */
static float gx_sin[256];
static float gx_s(float a){float q=a*256;int i=(int)fm_floorf(q);float f=q-(float)i;return gx_sin[i&255]+(gx_sin[(i+1)&255]-gx_sin[i&255])*f;}
/* Perspective projection of a tilted orbital plane with layered elevations.
 * UI and intro remain in screen space; no full-screen 3D framebuffer. */
static int gx_world3d;static float gx_height,gx_yaw_s,gx_yaw_c,gx_pitch_s,gx_pitch_c;
static float gx_project(float *x,float *y){if(!gx_world3d)return 1;
 float xx=(*x-GX_CX)/GX_SX,yy=(*y-GX_CY)/GX_SY;
 float rx=xx*gx_yaw_c-yy*gx_yaw_s,ry=xx*gx_yaw_s+yy*gx_yaw_c;
 float z=ry*gx_pitch_s+gx_height*gx_pitch_c,p=1.9f/fm_maxf(1.05f,1.9f+z);
 *x=GX_CX+rx*GX_SX*p;*y=GX_CY+(ry*gx_pitch_c-gx_height*gx_pitch_s)*GX_SY*p;
 return p;}
static float gx_fog_parallax_x,gx_fog_parallax_y;
static float gx_sky_energy,gx_sky_fog;static uint32_t gx_sky_ms;
static void gx_sky_update(void){uint32_t now=plat_ms();float dt=gx_sky_ms?(float)(now-gx_sky_ms)*.001f:0;gx_sky_ms=now;if(dt>.25f)dt=.25f;float e=0;for(int l=0;l<8;l++)e+=en_visual[l]*.125f;e=fm_minf(1,e*8);gx_sky_energy+=(e-gx_sky_energy)*dt/(e>gx_sky_energy?2.5f+dt:6+dt);gx_sky_fog+=(en_fog-gx_sky_fog)*dt/(3+dt);}
/* Stable black space; animated fog and transient celestial flashes supply
 * color independently. Keep the palette floor black even during a flash. */
static unsigned gx_major_flash(void);
static uint16_t gx_space_background=0;
static uint16_t gx_background_color(void){gx_sky_update();return gx_mix(gx_space_background,RGB(255,246,220),(int)gx_major_flash());}
static void gx_background(void){cv_rect(0,0,240,240,gx_background_color());}
static float gx_opacity=1;
#if EN_BALANCED_GRAPHICS
static uint32_t gx_blend_budget,gx_frame_blends,gx_soft_samples,gx_fog_rebuilds;
#endif
static void gx_glow(int x,int y,uint16_t c,int alpha){
 if(gx_intro_turn!=0){float dx=x-120.0f,dy=y-96.0f,cs=gx_s(gx_intro_turn+.25f),sn=gx_s(gx_intro_turn);x=(int)fm_floorf(120+dx*cs-dy*sn+.5f);y=(int)fm_floorf(96+dx*sn+dy*cs+.5f);}

 alpha=(int)((alpha>16?16:alpha)*gx_opacity+.5f);
 if(alpha<=0)return; /* Transparent coverage cannot change the pixel. */
#if EN_BALANCED_GRAPHICS
 if((unsigned)x>=cv_w||(unsigned)y>=cv_h||!gx_blend_budget)return;
 gx_blend_budget--;gx_frame_blends++;
#endif
 int stride=gx_lowres?2:1;if(gx_lowres){x&=~1;y&=~1;}
 for(int dy=0;dy<stride;dy++)for(int dx=0;dx<stride;dx++)if((unsigned)(x+dx)<cv_w&&(unsigned)(y+dy)<cv_h){uint16_t old=swap16(cv_px[(y+dy)*cv_w+x+dx]);cv_pset(x+dx,y+dy,gx_mix(old,c,alpha>16?16:alpha));}
}
/* Fractional coverage prevents slow motion from snapping between LCD pixels. */
static void gx_aa(float x,float y,uint16_t c,float alpha){gx_project(&x,&y);int ix=(int)fm_floorf(x),iy=(int)fm_floorf(y);float u=x-ix,v=y-iy;gx_glow(ix,iy,c,(int)(alpha*(1-u)*(1-v)+0.5f));gx_glow(ix+1,iy,c,(int)(alpha*u*(1-v)+0.5f));gx_glow(ix,iy+1,c,(int)(alpha*(1-u)*v+0.5f));gx_glow(ix+1,iy+1,c,(int)(alpha*u*v+0.5f));}
#if EN_BALANCED_GRAPHICS
static void gx_budget_begin(void){gx_blend_budget=30000u;gx_frame_blends=gx_soft_samples=0;}
/* Quantize only soft glow coverage. Foreground geometry/text remain native. */
static void gx_aura(float x,float y,float rx,float ry,uint16_t c,float power){
 if(rx<1||ry<1||power<=0||!gx_blend_budget)return;
 float perspective=gx_project(&x,&y);rx*=perspective;ry*=perspective;
 int x0=(int)(x-rx),x1=(int)(x+rx),y0=(int)(y-ry),y1=(int)(y+ry);
 x0=x0<0?0:x0;x1=x1>239?239:x1;y0=y0<0?0:y0;y1=y1>239?239:y1;if(x0>x1||y0>y1)return;
 int step=gx_quality?2:4;if(rx<4||ry<4)step=1;
 while((unsigned)((x1-x0+step)/step)*((y1-y0+step)/step)>1024u)step+=2;
 float cs=gx_intro_turn?gx_s(gx_intro_turn+.25f):1,sn=gx_intro_turn?gx_s(gx_intro_turn):0;
 for(int yy=y0;yy<=y1;yy+=step){float dy=((float)yy-y)/ry;
  for(int xx=x0;xx<=x1;xx+=step){if(!gx_blend_budget)return;float dx=((float)xx-x)/rx,q=1-dx*dx-dy*dy;if(q<=0)continue;
   int alpha=(int)(q*q*power*gx_opacity+.5f);if(alpha<=0)continue;if(alpha>16)alpha=16;
   int px=xx,py=yy;if(gx_intro_turn){float dx0=xx-120,dy0=yy-96;px=(int)(120+dx0*cs-dy0*sn+.5f);py=(int)(96+dx0*sn+dy0*cs+.5f);}
   if((unsigned)px>=240u||(unsigned)py>=240u)continue;
   uint16_t out=swap16(gx_mix(swap16(cv_px[py*cv_w+px]),c,alpha));
   int ex=px+step>240?240:px+step,ey=py+step>240?240:py+step;
   for(int j=py;j<ey;j++)for(int i=px;i<ex;i++)cv_px[j*cv_w+i]=out;
   gx_blend_budget--;gx_frame_blends++;gx_soft_samples++;
  }
 }
}
#else
static void gx_aura(float x,float y,float rx,float ry,uint16_t c,float power){if(rx<1||ry<1||power<=0)return;float perspective=gx_project(&x,&y);rx*=perspective;ry*=perspective;int x0=(int)(x-rx),x1=(int)(x+rx),y0=(int)(y-ry),y1=(int)(y+ry);x0=x0<0?0:x0;x1=x1>239?239:x1;y0=y0<0?0:y0;y1=y1>239?239:y1;/* Same pixel arithmetic, but each column division is reused across rows.
 * Main-thread aura work only; core1 space pass never touches this scratch. */
static float aura_dx2[240];for(int xx=x0;xx<=x1;xx++){float dx=((float)xx-x)/rx;aura_dx2[xx]=dx*dx;}
int stride=gx_lowres?2:1;for(int yy=y0;yy<=y1;yy+=stride){float dy=((float)yy-y)/ry;for(int xx=x0;xx<=x1;xx+=stride){float q=1-aura_dx2[xx]-dy*dy;if(q>0)gx_glow(xx,yy,c,(int)(q*q*power+0.5f));}}}
#endif
static void gx_ring(float x,float y,float rx,float ry,uint16_t c,int alpha){
 int n=(int)(fm_fabsf(rx)*6);if(n<64)n=64;if(n>(EN_BALANCED_GRAPHICS?(gx_quality>=2?256:128):512))n=EN_BALANCED_GRAPHICS?(gx_quality>=2?256:128):512;for(int i=0;i<n;i++){float a=(float)i/(float)n;gx_aa(x+gx_s(a+0.25f)*rx,y+gx_s(a)*ry,c,(float)alpha);}
}
/* Hand-drawn light choreography: tapered anime rays, bloom and lens ghosts.
 * No framebuffer persistence or extra full-screen allocation. */
static void gx_radiance(float x,float y,float age,float scale){
 if(age<0||age>4.5f)return;
 float pre=fm_clampf(age/0.18f,0,1),flash=age<0.18f?pre:fm_maxf(0,1-(age-0.18f)/0.55f);
 float flare=age<0.22f?0:fm_maxf(0,1-(age-0.22f)/3.5f),bloom=flash*flash;
 if((proj.par[P_REST]==EN_NIGHTFALL)){flash*=0.45f;flare*=0.65f;bloom*=0.4f;}
 gx_aura(x,y,(22+bloom*22)*scale,(15+bloom*15)*scale,GX_WHITE,flash*12);
 gx_aura(x,y,(52+flare*26)*scale,(23+flare*13)*scale,GX_TEAL,flare*5);
 /* A tapered horizontal light blade and four rotating, thinner sacred rays. */
 for(int j=-110;j<=110;j++){float q=1-fm_fabsf((float)j)/111;gx_aa(x+(float)j*scale,y,GX_WHITE,q*q*flash*15+q*flare*5);if(q>0.3f){gx_aa(x+j*scale,y-0.8f,GX_TEAL,q*q*flare*5);gx_aa(x+j*scale,y+0.8f,GX_TEAL,q*q*flare*5);}}
 for(int ray=0;ray<6;ray++){float a=(float)ray/6+age*0.016f;for(int j=2;j<47;j++){float q=1-(float)j/47;gx_aa(x+gx_s(a+0.25f)*j*scale,y+gx_s(a)*j*scale,GX_WHITE,q*q*flash*12);}}
 for(int k=0;k<5;k++){if(k==4&&((gx_animation_ms()/180u)%10u)>=4u)continue;float travel=(float)(k+1)*(12+age*7),xx=x-travel,yy=y+travel*0.24f,r=(3+k*1.7f)*scale;gx_ring(xx,yy,r,r*0.65f,k&1?GX_VIOLET:GX_TEAL,(int)(flare*(5-k)));gx_aura(xx,yy,r,r*0.65f,GX_WHITE,flare*2);}
 gx_ring(x,y,(12+age*27)*scale,(7+age*15)*scale,GX_WHITE,(int)(flare*4));
}
/* Reproject the same capture waves into each LCD band, including header/footer. */
static void gx_capture_burst(int screen_y){
 float cy=GX_CY,sx=GX_SX,sy=GX_SY;
 int flash=(int)(en_flash*((proj.par[P_REST]==EN_NIGHTFALL)?1:2));
 if(flash)gx_shade(0,0,(int)cv_w,(int)cv_h,GX_WHITE,flash);
 if(en_flash>0.12f)for(int r=18;r>0;r--){int light=(int)(en_flash*((proj.par[P_REST]==EN_NIGHTFALL)?7:15)*(1-(float)r/23));gx_ring(120,cy,(float)r*(1+en_flash*0.5f),(float)r*0.58f,GX_WHITE,light);}
 gx_radiance(120,cy,en_growth_age,1);
 for(int w=0;w<3;w++)if(en_wave_power[w]>0.01f){
  float q=en_wave[w],power=en_wave_power[w],wx=120+en_wave_x[w]*sx,wy=cy+en_wave_y[w]*sy;int light=(int)(power*7);
  gx_ring(wx,wy,q*sx,q*sy,GX_WHITE,light);
  gx_ring(wx,wy,q*sx+3,q*sy+2,GX_TEAL,light/2);
  /* Scaled accretion silhouettes: outward afterimages, not an opaque framebuffer copy. */
  if(en_wave_x[w]*en_wave_x[w]+en_wave_y[w]*en_wave_y[w]<0.01f)for(int echo=0;echo<1;echo++)for(int k=0;k<64;k++){
   float u=(float)(k%32)/32;float a=u*1.8f+(k/32)*0.5f+en_field_time*0.015f;
   float scale=1+q*1.8f+echo*0.12f;
   gx_glow((int)(120+gx_s(a+0.25f)*(3+u*39)*scale),(int)(cy+gx_s(a)*(3+u*39)*0.43f*scale),echo&1?GX_VIOLET:GX_TEAL,(int)(power*(8-echo*2)));
  }

 }
}
static float gx_galaxy_height(int layer){return .09f+(float)(layer%3)*.10f+.035f*gx_s((float)gx_animation_ms()*.000025f+layer*.13f);}
static void gx_spiral(float x,float y,int radius,int layer){
 float oldheight=gx_height;gx_height=gx_galaxy_height(layer);
 float life=en_life[layer],glow=en_energy[layer]+en_shock[layer]*0.3f,t=en_field_time;
 if(en_death[layer]>0){if(en_grb_mask&(1u<<layer)){gx_height=oldheight;return;}float q=1-en_death[layer]/6.0f;
  gx_ring(x,y,3+q*28,2+q*16,GX_WHITE,12);
  for(int k=0;k<12;k++){float a=(float)k/12;gx_aa(x+(int)(gx_s(a+0.25f)*(5+q*30)),y+(int)(gx_s(a)*(3+q*20)),GX_COLOR[layer],12);}
  gx_height=oldheight;return;
 }
 int awake=ui.view==V_PLANETS||(en_bloom_seen&(1u<<layer))!=0;
 uint16_t c=!awake?RGB(96,100,104):proj.par[LP(layer,L_MUTE)]?GX_DIM:gx_mix(GX_COLOR[layer],GX_WHITE,(int)(en_shock[layer]*6));

 if(life<0.23f){float birth=life/0.23f;gx_ring(x,y,5+birth*12,3+birth*7,c,(int)((1-birth)*8));c=gx_mix(0,c,(int)(birth*16));}
 float size=(float)radius*(1+en_bloom_blossom[layer]*0.12f)*(0.28f+0.72f*life)*(1+glow*0.10f)*(1+en_shock[layer]*0.32f)*(1+en_galaxy_growth[layer]);
 if(awake)gx_aura(x,y,size*1.65f,size*1.02f,c,4.5f+fm_minf(2,en_visual[layer]*12)+en_shock[layer]+fm_maxf(en_bloom_awake[layer],(en_bloom_seen&(1u<<layer))?0.6f:0)*5);
 if(ui.view==V_PLANETS)size=(float)radius;
 int arms=(int[]){2,2,4,3,5,2,3,4}[layer],points=gx_quality==2?(radius>=20?144:112):gx_quality==1?88:64;
 uint32_t variety=proj.seed^(uint32_t)layer*2654435761u^(uint32_t)en_field_cycles[layer]*97u;
 arms=2+(int)((variety>>9)%5u);
 float tilt=(float[]){0.66f,0.25f,0.8f,0.60f,0.88f,0.44f,0.7f,0.9f}[layer];
 tilt*=0.9f+0.1f*gx_s(t*0.012f+(float)layer*0.13f);if(ui.view==V_PLANETS)tilt=1;

 for(int j=0;j<16;j++){float a=(float)j/16+en_spin[layer],rr=size*(0.5f+0.5f*gx_s(a*3+layer));gx_aa(x+(int)(gx_s(a+0.25f)*rr),y+(int)(gx_s(a)*rr*tilt),c,4);}

 for(int a=0;a<arms;a++)for(int k=0;k<points;k++){
  float u=(float)k/(float)points;
  float phase=u*(.55f+(float)((variety>>16)&15u)*.065f)+(float)a/(float)arms+en_spin[layer]+gx_intro_age*gx_intro_object[layer]+(float)layer*0.07f;
  float r=(1+u*size*(.92f+.08f*gx_s(u*3+(float)(variety&255u)*.0039f))),px=x+r*gx_s(phase+0.25f),py=y+r*tilt*gx_s(phase);
  gx_aa(px,py,c,7+(k&7));
  gx_aa(px,py+0.65f,c,3+(k&3));
  if(awake&&k%8==0)gx_aa(px+0.5f,py-0.35f,GX_WHITE,8);
 }


 if(layer==4){for(int j=0;j<5;j++){float a=en_spin[layer]*1.2f+(float)j/5;gx_aa(x+(int)(gx_s(a+0.25f)*size*0.45f),y+(int)(gx_s(a)*size*tilt*0.45f),GX_WHITE,11);}}
 if(layer==5){int q=3+(int)(glow*5);for(int j=-q;j<=q;j++){gx_aa(x+j,y,c,16);gx_aa(x,y+j,c,16);}}
 if(layer==6)gx_ring(x,y,size*1.1f,size*tilt*1.1f,c,3);

 /* The central sun is planet0, rendered with its selected world. */
 gx_aa(x,y,awake?GX_WHITE:c,awake?16:7);gx_aa(x+0.6f,y,c,awake?10:5);gx_aa(x-0.6f,y,c,awake?8:4);
 gx_height=oldheight;
}
static const char *const BLOOM_PHASE[3]={"GENESIS","RESONANCE","ASCENSION"};
static int bloom_mode(void){return 0;}
static int bloom_eternal(void){return (proj.pool&0xff00u)==0xb100u&&(proj.pool&4u);}
enum { GX_LIGHTS=P_NPARAMS,GX_QUALITY,GX_PATCH_AUTO,GX_SAVER,GX_UNLOCK,GX_AIR,GX_UNUSED };
static int gx_param(int page,int knob){int l=ui.sel;
 if(page==V_USB_SCREEN)return GX_UNUSED;
 if(page==V_BLUETOOTH)return air_page==2&&knob>=2?GX_UNUSED:GX_AIR;
 if(page==V_TEXT)return ((int[]){P_TEXT,GX_SAVER,GX_UNLOCK,P_THEME})[knob];
 if(page==V_BASS){static const int fields[4][4]={{LP(1,L_VOICE),LP(1,L_TONE),LP(1,L_LEVEL),LP(1,L_TUNE)},{LP(1,L_ATTACK),LP(1,L_RELEASE),P_GLIDE,P_BASS},{LP(1,L_DENSITY),LP(1,L_GATE),LP(1,L_RATE),P_BASS_STRUM},{P_CHORD_PLAYBACK,P_STRUM,P_STRUM_DIR,P_STRUM_FEEL}};return fields[ui_bass_controls%4][knob];}
 if(page==V_TIMING)return ((int[]){P_QUANT_MODE,P_QUANT_GRID,P_QUANT_STRENGTH,P_QUANT_SWING})[knob];
 if(page==V_SOUNDSCAPES)return ((int[]){P_SCAPE_MODE,P_SCAPE_BANK,P_SCAPE_LEVEL,P_SCAPE_DENSITY})[knob];
 if(page==V_COMPOSITION)return ((int[]){P_STRUM_MODE,P_STRUM_FEEL,P_STRUM_DYN,P_STRUM_DIR})[knob];if(page==V_REVERB_DETAIL)return ((int[]){P_ROOM_DECAY,P_ROOM_PREDELAY,P_ROOM_DAMP,P_ROOM_MOD})[knob];if(page==V_ECHO_DETAIL)return ((int[]){P_ECHO_GRID,P_ECHO_FEEDBACK,P_ECHO,P_WIDTH})[knob];if(page==V_MOTIF_MODE)return ((int[]){P_MEMORY,P_MOTIF_VARIATION,P_EVOLVE,P_BARS})[knob];if(page==V_STRUM)return ((int[]){P_CHORD_PLAYBACK,P_STRUM,P_STRUM_DIR,P_BASS_STRUM})[knob];if(page==V_EXPRESSION)return ((int[]){P_GLIDE,P_VIBRATO,P_VIB_RATE,P_VIB_DELAY})[knob];
 /* Each global setting has one home; unassigned knobs do nothing. */
 if((page==V_SCENES&&knob>=2)||(page==V_OUTPUT&&(knob==1||knob==2))||(page==V_TEXT&&knob==3)||(page==V_MIXAUTO&&knob>=2)||(page==V_DEBUG&&knob==1))return GX_UNUSED;static const int global[17][4]={{P_EVOLVE,P_DENSITY,P_REVERB,P_MOTION},{0},{0},{P_EVOLVE,P_MEMORY,P_BARS,P_AUTO},{P_SCENE,P_FOCUS,P_BARS,P_AUTO},{P_REVERB,P_ECHO,P_WIDTH,0},{P_ANCHOR,P_TUNING,P_TEMPO,P_THEME},{0},{P_PULL,P_EVOLVE,P_HUMAN,P_DENSITY},{P_OUTPUT,P_WIDTH,P_REVERB,P_ECHO},{P_REST,P_DENSITY,P_CONTEXT_AUTO,P_REFERENCE},{P_TEXT,P_THEME,P_POWER,P_DENSITY},{0},{P_DRUMS,P_KIT,P_DRUM_LEVEL,P_DRUM_HITS},{P_BASS,LP(1,L_LEVEL),LP(1,L_ATTACK),P_BASS_STRUM}};
 if(page==V_MIXAUTO)return ((int[]){P_AUTO_LEVEL,P_AUTO_DUCK,P_FPS,P_OUTPUT})[knob];
 if(page==V_DEBUG)return ((int[]){P_FPS,P_POWER,P_AUTO_LEVEL,P_AUTO_DUCK})[knob];
 if(page==V_DRUMFX)return ((int[]){P_DRUM_DAMP,P_DRUM_AUTO,P_DRUM_ROOM,P_DRUM_ECHO})[knob];

 if(page==V_OUTPUT&&knob==3)return GX_LIGHTS;
 if(page==V_OBSERVATORY){if(ui_edit){static const uint8_t fields[3][4]={{L_VOICE,L_TONE,L_LEVEL,L_DENSITY},{L_ATTACK,L_RELEASE,L_GATE,L_TUNE},{L_REVERB,L_ECHO,L_RATE,L_VARIANT}};return LP(l,fields[ui_track_controls%3][knob]);}return ((int[]){P_STYLE,P_DRUMS,P_KIT,P_DRUM_HITS})[knob];}
 if(page==V_MATRIX)return LP(l,((int[]){L_LEVEL,L_PAN,L_MUTE,L_SOLO}[knob]));

 if(page==V_VOICE){if(knob==3)return GX_PATCH_AUTO;return LP(l,((int[]){L_VOICE,L_ATTACK,L_RELEASE}[knob]));}
 if(page==V_MOTIF)return LP(l,((int[]){L_STEPS,L_DENSITY,L_RATE,L_VARIANT}[knob]));
 if(page==V_MIXER){l=ui.lane;if(!ui_mixer_sends)return ((int[]){l==7?P_DRUM_LEVEL:LP(l,L_LEVEL),LP(l,L_PAN),LP(l,L_MUTE),LP(l,L_SOLO)})[knob];if(l==EN_DRUM_TRACK)return ((int[]){P_DRUM_LEVEL,LP(EN_DRUM_TRACK,L_PAN),P_DRUM_ROOM,P_DRUM_ECHO})[knob];return LP(l,((int[]){L_LEVEL,L_PAN,L_REVERB,L_ECHO}[knob]));}

 if(page==V_SPACE&&knob==3)return LP(l,L_REVERB);
 return global[page][knob];
}

static void gx_set(int p,int value){if(p==GX_SAVER){if(value>0){if(proj.par[P_POWER]!=5)project_display_normal=proj.par[P_POWER]==2?2:4;gx_set(P_POWER,5);}else gx_set(P_POWER,project_display_normal);return;}if(p==GX_UNLOCK){project_display_normal=value>0?2:4;if(proj.par[P_POWER]!=5)gx_set(P_POWER,project_display_normal);return;}if(p==GX_PATCH_AUTO){if(value>0)proj.patch_locks&=(uint8_t)~(1u<<ui.sel);else proj.patch_locks|=(uint8_t)(1u<<ui.sel);en_patch_locks=proj.patch_locks;if(value>0)project_arrange(en_reference());ui.dirty=1;return;}if(p==GX_UNUSED)return;if(p==GX_QUALITY){project_quality=0;en_quality(project_quality);ui.dirty=1;return;}if(p==GX_LIGHTS){proj.leds=(uint8_t)(value<0?0:value>3?3:value);ui.dirty=1;return;}const en_param_t *q=en_param_info(p);value=value<q->lo?q->lo:value>q->hi?q->hi:value;if(p==P_STRUM&&value>0)value=value<80?80:value>200?200:value;if(p==P_TEMPO&&(proj.par[P_REST]==EN_NIGHTFALL)&&value>48)value=48;
 if(p==P_POWER){if(value<=1)value=2;else if(value==3)value=proj.par[p]<value?4:2;}
 if(p>=P_LAYER&&p<P_LAYER+7*16&&((p-P_LAYER)%16==L_VOICE||ui.view==V_OBSERVATORY||ui.view==V_BASS)){proj.patch_locks|=(uint8_t)(1u<<((p-P_LAYER)/16));en_patch_locks=proj.patch_locks;}
 if(p==P_TEMPO||p==P_STRUM||p==P_CHORD_PLAYBACK){proj.par[P_CONTEXT_AUTO]=0;en_set(P_CONTEXT_AUTO,0);}
 if(p==P_KIT){proj.par[P_DRUM_AUTO]=0;en_set(P_DRUM_AUTO,0);}
 if(p==P_REFERENCE)proj.reference_manual=value?0xa5u:0;int old=proj.par[p];proj.par[p]=(int16_t)value;en_set(p,value);ui.dirty=1;if(p==P_FOCUS)ui.sel=(uint8_t)value;
 if(p==P_CONTEXT_AUTO&&value)project_arrange(en_reference());
 if(p==P_REFERENCE&&old!=value)project_arrange(value?value-1:en_reference_for(proj.piece,proj.par[P_ANCHOR])-1);
 if(p==P_DRUM_AUTO&&value&&old!=value){int16_t auto_recipe[P_NPARAMS];memcpy(auto_recipe,proj.par,sizeof auto_recipe);en_arrange(auto_recipe,proj.piece,en_reference());proj.par[P_KIT]=auto_recipe[P_KIT];en_set(P_KIT,proj.par[P_KIT]);}
 if(p==P_BASS&&old==7&&value<7){int l=1;proj.par[LP(l,L_MUTE)]=0;en_set(LP(l,L_MUTE),0);if(proj.par[LP(l,L_LEVEL)]<55){proj.par[LP(l,L_LEVEL)]=72;en_set(LP(l,L_LEVEL),72);}if(!proj.par[LP(l,L_DENSITY)]){proj.par[LP(l,L_DENSITY)]=60;en_set(LP(l,L_DENSITY),60);}}
 if(p==P_REST&&value==EN_NIGHTFALL&&proj.par[P_CONTEXT_AUTO]&&proj.par[P_TEMPO]>40){proj.par[P_TEMPO]=36;en_set(P_TEMPO,36);}
 if(p==P_STYLE&&old!=value){static const int16_t fx[4][6]={{72,15,55,25,32,2},{82,12,68,18,25,1},{78,16,48,35,38,1},{62,8,50,12,28,2}};for(int q=0;q<6;q++){proj.par[P_ROOM_DECAY+q]=fx[value][q];en_set(P_ROOM_DECAY+q,fx[value][q]);}if(proj.par[P_CONTEXT_AUTO]&&old==EN_NIGHTFALL&&value!=EN_NIGHTFALL&&proj.par[P_TEMPO]<=40){proj.par[P_TEMPO]=value==EN_CINEMATIC?60:48;en_set(P_TEMPO,proj.par[P_TEMPO]);}if(old==EN_CINEMATIC||value==EN_CINEMATIC||old==EN_NEOCLASSICAL||value==EN_NEOCLASSICAL)project_arrange(en_reference());if(value==EN_NEOCLASSICAL){if(proj.par[P_CONTEXT_AUTO]){proj.par[P_TEMPO]=66;en_set(P_TEMPO,66);}proj.par[P_DRUMS]=0;en_set(P_DRUMS,0);}}}
void ui_say(const char *a,const char *b){gx_str(ui.msg,a,31);int n=0;while(ui.msg[n])n++;if(n<20&&b&&*b){ui.msg[n++]=' ';gx_str(ui.msg+n,b,32-n);}ui.msg_until=plat_ms()+1800;}
static int gx_anysolo(void){int i,s=0;for(i=0;i<8;i++)s|=proj.par[LP(i,L_SOLO)];return s;}
static const uint8_t gx_white[16]={0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
static int gx_pad(int note){for(int i=0;i<16;i++)if(gx_white[i]==note)return i;return -1;}
void ui_activity(void){ui.idle=0;ui.activity=plat_ms();}
static uint32_t ui_grb_seen;
static void ui_sync_tracks(void){if(ui_grb_seen!=en_grb_serial){ui_grb_seen=en_grb_serial;if(!proj.reference_manual){proj.par[P_REFERENCE]=0;project_arrange(en_reference());}ui.dirty=1;}for(int l=0;l<8;l++)if(ui_track_seen[l]!=en_track_revision[l]){ui_track_seen[l]=en_track_revision[l];proj.par[LP(l,L_VOICE)]=en_track_patch[l];proj.par[LP(l,L_VARIANT)]=en_track_variant[l];proj.par[P_MASK+en_scene]|=(int16_t)(1u<<l);ui.dirty=1;}}
/* Menus stay open; the live/voice/phrase views return to the universe after 8s. */
static int ui_no_timeout(int view){return ui_hub||view!=V_OBSERVATORY;}
static int8_t ui_key_octave;static uint8_t ui_intent_pending,ui_intent_target;
static void ui_key_octave_change(int delta){int n=ui_key_octave+delta;if(n<-2)n=-2;if(n>2)n=2;ui_key_octave=(int8_t)n;en_keyboard_octave(n);char b[12];gx_num(b,n);ui_say("KEY OCTAVE",b);}
/* Strong note pulses, then moving sustain trails; fixed eight-track work. */
static uint32_t gx_dance_keys(uint32_t now,uint32_t held){
 static uint32_t seen[8],hit[8];uint32_t lights=held;unsigned used=0;
 float strongest=0;int focus=-1;
 for(int l=0;l<8;l++){if(seen[l]!=en_note_serial[l]){seen[l]=en_note_serial[l];hit[l]=now;}
  float e=en_visual[l];if(e>strongest){strongest=e;focus=l;}}
 if(!en_playing||en_frozen||strongest<.001f)return held;
 for(int l=0;l<8;l++){
  unsigned age=now-hit[l];float e=en_visual[l];
  if(e<.001f||((age>800u||used>=3u)&&l!=focus))continue;
  /* Exact1.5x travel without multiplying a long-running age into overflow. */
  unsigned travel=(age/170u)*3u+(age%170u)*3u/170u,head=(l*7u+seen[l]*5u+travel)%27u;
  if(l&1)head=26u-head;
  lights|=1u<<head;
  lights|=1u<<((head+26u)%27u);
  if(age<320u){lights|=1u<<((head+1u)%27u);lights|=1u<<((head+13u)%27u);}
  /* Sustained tones keep breathing, with dark gaps between visible sweeps. */
  if(l==focus&&((age/120u)&3u)==0)lights|=1u<<((head+25u)%27u);
  used++;
 }return lights;
}
void ui_input_only(void){ui_sync_tracks();
 uint32_t now=plat_ms(),b=plat_buttons(),k=plat_keys(),raw_b=b,raw_k=k;int i,d,delta[NE],moved=0;for(i=0;i<NE;i++){delta[i]=plat_enc(i);moved|=delta[i]!=0;}
 uint8_t test=ui.view==V_BLUETOOTH&&air_enabled&&air_prefs.calibrated==3&&(b&(1u<<B_SEL));
 if(test!=air_test){air_test=test;air_tick_ms=0;}
 air_control_tick(now);
 /* The smoothed analog master can move a few counts without a touch.
  * Require three observations beyond a 2.3% hysteresis band. Encoders and
  * buttons already arrive from the debounced panel driver. */
 uint32_t master_now=plat_master();
 int master_moved=master_now>ui.master_prev+96u||master_now+96u<ui.master_prev;
 if(master_moved){if(ui.master_samples<3)ui.master_samples++;}else ui.master_samples=0;
 if(ui.master_samples==3){moved=1;ui.master_prev=master_now;ui.master_samples=0;}
 if(ui.timer_view!=ui.view){ui.timer_view=ui.view;ui.idle=0;ui.activity=now;}
 uint32_t physical_down=b&~ui.last_b&~ui.ignore_b;
 int touched=moved||physical_down; /* Keys and releases never wake chrome. */
 if(ui.idle&&touched){
  ui_activity();ui.wake_ms=now;ui.wake_reason=physical_down?1:2;
  /* Consume the first action AND its later button release. Notes still play. */
  ui.ignore_b|=b;for(i=0;i<NE;i++)if(i!=EN_ALGO&&i!=EN_SELECT&&i!=EN_PRESET)delta[i]=0;
 }
 ui.ignore_b&=b;ui.ignore_k&=k;b&=~ui.ignore_b;k&=~ui.ignore_k;
 if(!ui.idle&&(moved||raw_b!=ui.raw_last_b||raw_k!=ui.raw_last_k||raw_b||raw_k))ui.activity=now;
 if(!ui_no_timeout(ui.view)&&!ui.idle&&(uint32_t)(now-ui.activity)>=8000u)ui.idle=1;
 if(ui_no_timeout(ui.view))ui.idle=0;
 uint32_t down=b&~ui.last_b,up=ui.last_b&~b;
 for(i=0;i<NB;i++)if(down&(1u<<i))ui.pressed[i]=now;
 if(down&(1u<<B_HOME)){if(ui_edit)ui_edit=0;else ui_open_area(0);ui.idle=0;ui_activity();down&=~(1u<<B_HOME);}
 if(delta[EN_ALGO]){ui_locate_page();ui_open_area((ui_area+delta[EN_ALGO]%5+5)%5);delta[EN_ALGO]=0;ui_activity();}
 if(up&(1u<<B_SEL)){if(ui.view==V_USB_SCREEN)plat_mirror_armed=!plat_mirror_armed;else ui_edit=!ui_edit;up&=~(1u<<B_SEL);}
 if(delta[EN_SELECT]&&ui.view==V_OBSERVATORY){ui.sel=(uint8_t)((ui.sel+delta[EN_SELECT]%7+7)%7);ui.lane=ui.sel;delta[EN_SELECT]=0;ui.dirty=1;ui_activity();}
 if(delta[EN_SELECT]&&!ui_edit){ui_topic_step(delta[EN_SELECT]);delta[EN_SELECT]=0;ui_activity();}

 if(down&(1u<<B_ENV))ui_open_area(1);
 if(down&(1u<<B_LFO))ui_open_area(2);
 if(down&(1u<<B_FX)){ui_open_area(4);ui.view=V_SPACE;ui_locate_page();}
 if(down&(1u<<B_GLO))ui_open_area(4);
 if((down&(1u<<B_ARP))&&ui.view!=V_OBSERVATORY){ui_open_area(2);ui.view=V_MOTIF;}
 if(down&(1u<<B_SEQ)){ui_open_area(2);ui.view=V_SCENES;}
 if(down&(1u<<B_PLAY)){en_freeze(0);en_play(!en_playing);}
 if(down&(1u<<B_REC)){ui.view=V_DRUMS;ui_locate_page();ui_edit=0;}
 if((up&(1u<<B_EDIT))&&ui.view==V_MIXER){ui_mixer_sends=!ui_mixer_sends;up&=~(1u<<B_EDIT);}
 if((up&(1u<<B_EDIT))&&ui.view==V_OBSERVATORY){ui_track_controls=(ui_track_controls+1)%3;ui_edit=1;up&=~(1u<<B_EDIT);ui_say("EDIT",(const char *const[]){"SOUND / TONE / LEVEL / NOTES","ENVELOPE / REGISTER","SENDS / PHRASE"}[ui_track_controls]);}
 if((up&(1u<<B_ARP))&&ui.view==V_OBSERVATORY){gx_set(LP(ui.sel,L_VARIANT),(proj.par[LP(ui.sel,L_VARIANT)]+1)&127);ui_say("EVOLVE TRACK",GALAXY_NAME[ui.sel]);}
 if((up&(1u<<B_EDIT))&&ui.view==V_BASS){ui_bass_controls=(ui_bass_controls+1)%4;up&=~(1u<<B_EDIT);}
 if((up&(1u<<B_EDIT))&&ui.view==V_BLUETOOTH){air_page=(air_page+1)%3;up&=~(1u<<B_EDIT);}
 if(up&(1u<<B_EDIT)){
  if(now-ui.pressed[B_EDIT]>700){proj.seed^=now*2654435761u;if(!proj.seed)proj.seed=1;en_planets_default(proj.seed,&proj.planets);project_apply();ui_say("NEW GALAXY SEED","");}
  else ui.view=ui.view==V_PLANETS?V_VOICE:V_PLANETS;
 }
 if(up&(1u<<B_SAVE)){if(now-ui.pressed[B_SAVE]>1300){project_piece_reset();ui_say("RECIPE RESTORED","");}else{int rc=project_save();ui.dirty=(uint8_t)(rc!=0);ui_say(rc?"SAVE FAILED":"UNIVERSE SAVED","");}}
 if(down&(1u<<B_OCTDN))ui_key_octave_change(-1);
 if(down&(1u<<B_OCTUP))ui_key_octave_change(1);
 if((d=delta[EN_SELECT])){if(ui.view==V_DRUMPARTS){ui.drum_part=(uint8_t)((ui.drum_part+d%5+5)%5);}else if(ui.view==V_PLANETS){int n=en_planets.count[ui.sel];ui.planet=(uint8_t)((ui.planet+d%n+n)%n);}else if(ui.view==V_MIXER){ui.lane=(uint8_t)((ui.lane+d%8+8)%8);if(ui.lane<7){ui.sel=ui.lane;gx_set(P_FOCUS,ui.sel);}}else{ui.sel=(uint8_t)((ui.sel+d%EN_NGAL+EN_NGAL)%EN_NGAL);ui.lane=ui.sel;gx_set(P_FOCUS,ui.sel);}}
 if((d=delta[EN_PRESET])){if(!ui_intent_pending)ui_intent_target=proj.piece;ui_intent_target=(uint8_t)(((int)ui_intent_target+d%10+10)%10);ui_intent_pending=1;delta[EN_PRESET]=0;}
 if(ui_intent_pending&&en_command_room()>=P_NPARAMS+10u){project_piece(ui_intent_target);gx_set(P_FOCUS,ui.sel);if(ui.view!=V_INTENT)ui_say("INTENTION",piece_name(proj.piece));else ui.msg_until=0;ui_intent_pending=0;}
 /* Musical keys work on every page. SEL reserves Bluetooth setup gestures. */
 uint32_t musical_keys=k&~ui.last_k;
 for(i=0;i<27;i++)if((ui.last_k&~k)&(1u<<i))en_key_release(i-12);
 if(!(EN_BLE&&ui.view==V_BLUETOOTH&&(b&(1u<<B_SEL))))for(i=0;i<27;i++)if(musical_keys&(1u<<i))en_strike(ui.sel,i-12);
 if(ui.view==V_BLUETOOTH){
  if(up&(1u<<B_EDIT))air_page=(air_page+1)%3;
  if(!air_page){if((d=delta[EN_K1]))air_control_enable(d>0);
   if((d=delta[EN_K2])){
    const char *message;
    if(air_calibrate(d>0))message=d>0?"NEAR CALIBRATED":"FAR CALIBRATED";
    else if(air_status!=2)message="CALIBRATION NEEDS SIGNAL";
    else if(!(air_prefs.calibrated&1))message="CAPTURE FAR FIRST";
    else message="NEAR TOO CLOSE TO FAR";
    ui_say(message,"");
   }
   if((d=delta[EN_K3]))air_prefs.depth=(uint8_t)gx_bound(air_prefs.depth+d,0,100);
   if((d=delta[EN_K4])){air_prefs.track=(uint8_t)gx_bound(air_prefs.track+d,0,7);en_air_filter(air_prefs.track,air_cutoff,air_resonance);}
  }else if(air_page==2){
   if(delta[EN_K1]||delta[EN_K2]){air_cutoff=(uint8_t)gx_bound(air_cutoff+delta[EN_K1],0,100);air_resonance=(uint8_t)gx_bound(air_resonance+delta[EN_K2],0,100);en_air_filter(air_prefs.track,air_cutoff,air_resonance);}
  }else{if((d=delta[EN_K1]))air_prefs.sensitivity=(uint8_t)gx_bound(air_prefs.sensitivity+d,0,100);
   if((d=delta[EN_K2]))air_prefs.smoothing=(uint8_t)gx_bound(air_prefs.smoothing+d,0,100);
   if((d=delta[EN_K3]))air_prefs.invert=d>0;
   if((d=delta[EN_K4]))air_prefs.destination=(uint8_t)gx_bound(air_prefs.destination+d,0,5);
  }goto input_done;
 }

 for(i=0;i<4;i++)if((d=delta[EN_K1+i])){if(ui.view==V_DRUMPARTS){if(i==0){if(d>0)project_drum_parts|=(uint8_t)(1u<<ui.drum_part);else project_drum_parts&=(uint8_t)~(1u<<ui.drum_part);en_drum_parts(project_drum_parts);}continue;}if(gx_param(ui.view,i)==GX_UNUSED)continue;if(ui.view==V_DEBUG&&i==2){ui.debug_detail=(uint8_t)((ui.debug_detail+1)%12);continue;}if(ui.view==V_DEBUG&&i==3){int mode=(int)en_diagnostic_mode+(d>0?1:-1);en_diagnostic(mode);ui_say("TEST MODE",(const char *const[]){"NORMAL","AUDIO ONLY","GRAPHICS ONLY","CORE0 ONLY"}[mode<0?0:mode>3?3:mode]);continue;}if(ui.view==V_OUTPUT&&i==3){gx_set(GX_LIGHTS,(int)proj.leds+d);}else if(ui.view==V_PLANETS){int n=en_planets.count[ui.sel];if(ui.planet>=n)ui.planet=0;en_planet_t v=en_planets.planet[ui.sel][ui.planet];int value=((uint8_t *)&v)[i]+d;en_planet_set(ui.sel,ui.planet,i,value);ui.dirty=1;}else{int p=gx_param(ui.view,i);gx_set(p,p==GX_SAVER?((proj.par[P_POWER]==5)?1:0)+d:p==GX_UNLOCK?(project_display_normal==2?1:0)+d:p==GX_PATCH_AUTO?((proj.patch_locks&(1u<<ui.sel))?0:1)+d:proj.par[p]+d);}}
 input_done:
 if(ui.timer_view!=ui.view){ui.timer_view=ui.view;ui.idle=0;ui.activity=now;}
 if(ui_remote_serial!=en_remote_serial){ui_remote_serial=en_remote_serial;for(unsigned p=0;p<P_NPARAMS;p++){unsigned rev=en_remote_revision(p);if(rev!=ui_remote_rev[p]){ui_remote_rev[p]=(uint8_t)rev;proj.par[p]=(int16_t)en_remote_value(p);ui.dirty=1;}}}
 project_preferences_tick();
 ui.last_b=b;ui.last_k=k;ui.raw_last_b=raw_b;ui.raw_last_k=raw_k;
 plat_glow(proj.leds==1||proj.leds==3);uint32_t leds=0,key_lights=0;
 if(proj.leds){if(en_playing)leds|=1u<<B_PLAY;if(en_frozen)leds|=1u<<B_REC;if(proj.leds==3){
  key_lights=gx_dance_keys(plat_ms(),k);

 }else if(en_live_mode==2){unsigned scale=en_scale_mask(proj.par[P_TUNING]);for(int key=0;key<27;key++)if(scale&(1u<<(key%12)))key_lights|=1u<<key;}
 else for(int l=0;l<8;l++)if(en_visual[l]>.02f)key_lights|=1u<<gx_white[l];}
 plat_leds(leds,key_lights);plat_play_red(proj.leds&&en_frozen);
}
static float gx_fog_density(float x,float y,float time){float drift=time*0.65f;float a=gx_s(x*0.0031f+y*0.0015f-drift*0.010f),b=gx_s(x*0.0055f-y*0.0021f-drift*0.017f+0.23f);float c=gx_s(x*0.0011f+y*0.0052f+drift*0.006f);float q=0.50f+a*0.23f+b*0.16f+c*0.14f;return fm_clampf((q-0.38f)*1.8f,0,1);}
/* Adjacent 4x4 cells share corners. Two rows retain the original density
 * and interpolation arithmetic without a full-screen auxiliary buffer. */
#if EN_BALANCED_GRAPHICS
/* Cached four-pixel background cells, applied before stars/foreground/UI. */
static uint16_t gx_fog_composite(uint16_t base,uint16_t fog){unsigned r=(base>>11)+(fog>>11),g=((base>>5)&63)+((fog>>5)&63),b=(base&31)+(fog&31);if(r>31)r=31;if(g>63)g=63;if(b>31)b=31;return (uint16_t)(r<<11|g<<5|b);}
static uint16_t gx_fog_color(unsigned x,unsigned y,float time,float a,uint16_t col){float q=gx_fog_density(x*4,y*4,time)*a;unsigned rr=(unsigned)((col>>11)*q*.82f+.5f),gg=(unsigned)(((col>>5)&63)*q*.82f+.5f),bb=(unsigned)((col&31)*q*.82f+.5f);return (uint16_t)(rr<<11|gg<<5|bb);}
static void gx_fog_base(int overlay,int fill,uint16_t background){
 if(overlay)return;gx_sky_update();float amount=fm_clampf(.16f+gx_sky_fog*.94f,0,1)*((proj.par[P_REST]==EN_NIGHTFALL)?.70f:1);if(amount<.01f){if(fill)cv_rect(0,0,240,240,background);return;}
 unsigned rate=en_graphics_throttle?3u:15u,tick=(unsigned)(en_field_time*rate),aq=(unsigned)(amount*32+.5f),theme=(unsigned)proj.par[P_THEME];
 static uint16_t cells[60*60];static unsigned last_tick,last_aq,last_theme,last_rate,last_tint_tick,valid;
 unsigned tint_tick=plat_ms()/500u;
 if(!valid||tick!=last_tick||aq!=last_aq||theme!=last_theme||rate!=last_rate||tint_tick!=last_tint_tick){
  float time=(float)tick/rate,a=(float)aq/32;uint16_t col=gx_mix(RGB(0,245,210),RGB(235,20,255),(int)(4+7*fm_clampf(en_visual[2]+en_visual[4],0,1)));
  for(unsigned y=0;y<60;y++){uint16_t tint=gx_mix(gx_mix(col,RGB(20,65,255),5),RGB(255,24,145),(int)(6+5*gx_s((float)tint_tick*.0015f+(float)y*.013f)));for(unsigned x=0;x<60;x++)cells[y*60+x]=gx_fog_color(x,y,time,a,tint);}
  last_tick=tick;last_aq=aq;last_theme=theme;last_rate=rate;last_tint_tick=tint_tick;valid=1;gx_fog_rebuilds++;
 }
 for(unsigned y=0;y<60;y++)for(unsigned x=0;x<60;x++){
  unsigned cell=cv_w==120?2u:4u,px=x*cell,py=y*cell;
  /* Three depth-separated samples reuse the existing60x60 density cache.
   * Different drift/parallax rates produce volume without3 caches or shaders. */
  int dx=(int)gx_fog_parallax_x,dy=(int)gx_fog_parallax_y;unsigned drift=(unsigned)(en_field_time*.28f);unsigned x0=(x+60+(dx%60))%60,y0=(y+60+(dy%60))%60;
  unsigned x1=(x+120+2*(dx%60)+drift)%60,y1=(y+120+2*(dy%60))%60,x2=(x+240+3*(dx%60)-drift%60)%60,y2=(y+180+3*(dy%60))%60;
  uint16_t fog=gx_mix(cells[y0*60+x0],cells[y1*60+x1],5);fog=gx_mix(fog,cells[y2*60+x2],3);
  uint16_t color=swap16(gx_fog_composite(fill?background:swap16(cv_px[py*cv_w+px]),fog));
  uint32_t pair=(uint32_t)color|((uint32_t)color<<16);
  for(unsigned j=0;j<cell;j++){uint16_t *row=cv_px+(py+j)*cv_w+px;__builtin_memcpy(row,&pair,4);if(cell==4)__builtin_memcpy(row+2,&pair,4);}
 }
}

static void gx_fog(int overlay){gx_fog_base(overlay,0,0);}

#else
static void gx_fog(int overlay){
 if(overlay)return;float time=(float)((uint32_t)(en_field_time*15))/15,amount=en_fog*((proj.par[P_REST]==EN_NIGHTFALL)?0.70f:1);if(amount<0.01f)return;
 static float rows[2][61];
 for(int x=0;x<=60;x++)rows[0][x]=gx_fog_density(x*4,0,time)*amount;
 for(int y=0;y<240;y+=4){
  float *top=rows[(y/4)&1],*bottom=rows[((y/4)&1)^1];
  for(int x=0;x<=60;x++)bottom[x]=gx_fog_density(x*4,y+4,time)*amount;
  for(int x=0;x<240;x+=4){
   int k=x/4;float a=top[k],b=top[k+1],c=bottom[k],d=bottom[k+1];
   for(int j=0;j<4;j++)for(int i=0;i<4;i++){float u=i*.25f,v=j*.25f,q=(a+(b-a)*u)*(1-v)+(c+(d-c)*u)*v;uint16_t col=gx_mix(RGB(0,245,210),RGB(235,20,255),(int)(4+7*fm_clampf(en_visual[2]+en_visual[4],0,1)));int rr=(int)((col>>11)*q*.82f+.5f),gg=(int)(((col>>5)&63)*q*.82f+.5f),bb=(int)((col&31)*q*.82f+.5f);uint16_t old=swap16(cv_px[(y+j)*cv_w+x+i]);cv_pset(x+i,y+j,(uint16_t)(old|(rr<<11|gg<<5|bb)));}
  }
 }
}
#endif
/* Identical base/fog pixels in one framebuffer pass instead of three. */
static void gx_background_fog(void){
#if EN_BALANCED_GRAPHICS
 gx_fog_base(0,1,gx_background_color());
#else
 gx_background();gx_fog(0);
#endif
}


/* A moving depth field, not a static wallpaper. Each hashed star travels
 * toward the viewer; endpoint fades make wrapping invisible. No star buffers. */
static void gx_starflight(float cy){
 float time=en_field_time,energy=0;for(int l=0;l<8;l++)energy+=en_visual[l]*0.125f;
 float vanx=GX_CX+gx_s(time*0.0013f)*8,vany=cy+gx_s(time*0.0011f+0.3f)*5;
 for(int k=0;k<196;k++){
  uint32_t h=(uint32_t)k*2654435761u+0x42a9u;h^=h>>13;h*=2246822519u;float z=(float)(h&4095u)/4096-time*(0.019f+(float)(k%3)*0.0048f)*(1+(float)en_bloom_phase*0.12f);z-=fm_floorf(z);
  float a=(float)((h>>12)&4095u)/4096,r=0.10f+(float)((h>>24)&255u)*0.0024f,depth=1/(0.25f+z*1.8f);
  float dx=gx_s(a+0.25f)*r*155*depth,dy=gx_s(a)*r*115*depth;
  float fade=fm_clampf(fm_minf(z,1-z)*12,0,1),light=fade*(9+(1-z)*12+fm_minf(4,energy*14)+gx_fog_density(vanx+dx,vany+dy,time)*en_fog*en_energy[k&7]*8);
  uint16_t color=gx_mix(GX_COLOR[k&7],GX_WHITE,8+(int)(en_energy[k&7]*7));gx_aa(vanx+dx,vany+dy,color,light);if(k%5==0){gx_aa(vanx+dx-.65f,vany+dy,color,light*.38f);gx_aa(vanx+dx+.65f,vany+dy,color,light*.38f);}
  if(k%4==0){float q=(1-z)*0.028f;for(int j=1;j<=4;j++)gx_aa(vanx+dx*(1-q*j),vany+dy*(1-q*j),color,light*(0.60f-(float)j*0.10f));}
 }
}
static void gx_bloom_line(float x,float y,float u,float v,uint16_t c,float light){int n=(int)(fm_maxf(fm_fabsf(x-u),fm_fabsf(y-v))*0.65f);if(n<2)n=2;if(n>120)n=120;for(int j=0;j<=n;j++){float q=(float)j/n;gx_aa(x+(u-x)*q,y+(v-y)*q,c,light);}}
static void gx_planet_body(float x,float y,float radius,uint16_t c,float flash){
 gx_aura(x,y,radius*2.5f,radius*2.1f,c,2+flash*4);gx_aura(x,y,radius+.8f,radius+.8f,c,13);
 gx_aa(x-radius*.35f,y-radius*.38f,GX_WHITE,12+flash*3);gx_aa(x+radius*.45f,y+radius*.40f,gx_mix(c,GX_TEAL,7),12);
 gx_aa(x-radius*.55f,y-radius*.12f,GX_WHITE,7);gx_aa(x+radius*.2f,y-radius*.6f,c,10);
}
static void gx_world_planet(float x,float y,float base,int l,int p){base*=en_world_growth(l,p);if(base<.04f)return;
 static const uint16_t colour[7]={RGB(118,232,214),RGB(246,202,124),RGB(166,190,250),RGB(232,148,209),RGB(140,219,158),RGB(240,154,120),RGB(199,174,250)};
 en_planet_t v=en_planets.planet[l][p];int sun=p==0&&(en_sun_mask&(1u<<l));float r=base*(sun?3.2f+(float)v.level*.006f:.6f+(float)v.level*.012f),light=(float)v.space*.01f;
 uint16_t c=sun?gx_mix(colour[v.pitch%7],GX_COLOR[5],12):colour[v.pitch%7];
 if(v.hold>=65)gx_ring(x,y,r*1.9f,r*.65f,c,4+(int)(light*5));
 gx_aura(x,y,r*(sun?3.2f:2.6f),r*(sun?2.5f:2.1f),c,(sun?4:1)+light*(sun?8:5));gx_planet_body(x,y,r,c,en_planet_flash[l][p]);
 if(r>3)for(int band=0;band<3;band++){float yy=y-r*.35f+band*r*.3f;gx_bloom_line(x-r*.6f,yy,x+r*.6f,yy+gx_s(en_spin[l]+band*.2f)*r*.10f,gx_mix(c,GX_WHITE,band==0?10:3),5);}
 if(sun){gx_aura(x-r*.15f,y-r*.18f,r*.6f,r*.6f,GX_WHITE,14);for(int ray=0;ray<6;ray++){float a=(float)ray/6+en_spin[l]*.12f;gx_bloom_line(x+gx_s(a+.25f)*r,y+gx_s(a)*r,x+gx_s(a+.25f)*r*2.2f,y+gx_s(a)*r*1.7f,c,4+light*4);}}
}
static void gx_singularity(void){
 if(!en_grb_phase)return;float x=GX_CX+en_grb_x*GX_SX,y=GX_CY+en_grb_y*GX_SY,age=en_grb_age;
 if(en_grb_phase<3){float form=en_grb_phase==1?fm_clampf(age/6,0,1):1,r=(3+form*19)*GX_ZOOM,turn=(float)gx_animation_ms()*.00014f;
  gx_aura(x,y,r*2.4f,r*1.4f,GX_VIOLET,form*5);gx_aura(x,y,r*1.6f,r*.7f,GX_TEAL,form*5);
  for(int arm=0;arm<3;arm++)for(int k=0;k<(gx_quality?72:48);k++){float u=(float)k/(gx_quality?72:48),a=u*1.7f+(float)arm/3-turn,rr=r*(.65f+u*1.8f);gx_aa(x+gx_s(a+.25f)*rr,y+gx_s(a)*rr*.42f,arm==1?GX_COLOR[5]:GX_VIOLET,(5+form*7)*(1-u));}
  gx_ring(x,y,r,r*.7f,GX_WHITE,(int)(form*11));gx_aura(x,y,r*.72f,r*.55f,0,16);gx_ring(x,y,r*1.2f,r*.45f,GX_COLOR[5],(int)(form*14));
  if(en_grb_phase==2){gx_bloom_line(x-r*2,y,x+r*2,y,GX_TEAL,8);gx_aa(x-r*2.3f,y,GX_WHITE,9);gx_aa(x+r*2.3f,y,GX_WHITE,9);}
 }else{
  float power=fm_clampf(1-age/12,0,1),front=fm_minf(1,age/1.5f),r=(8+age*30)*GX_ZOOM;
  gx_aura(x,y,(22+age*8)*GX_ZOOM,(15+age*5)*GX_ZOOM,GX_VIOLET,power*2.5f);gx_aura(x,y,12*GX_ZOOM,9*GX_ZOOM,GX_WHITE,power*14);if(age<2.5f)gx_radiance(x,y,age,1.7f);gx_ring(x,y,r,r*.60f,GX_TEAL,(int)(power*12));gx_ring(x,y,r*1.1f,r*.66f,GX_VIOLET,(int)(power*7));
  for(int side=-1;side<=1;side+=2)for(int j=0;j<96;j++){float u=(float)j/96,len=(24+front*210)*u,spread=(3+u*22)*power,bend=gx_s(u*2-age*.025f)*spread*.4f;for(int edge=-1;edge<=1;edge++){float xx=x+side*len*.8f+edge*spread*.35f,yy=y-side*len*.55f+bend+edge*spread,step=(24+front*210)/96;gx_bloom_line(xx-side*step*.8f,yy+side*step*.55f,xx,yy,edge?GX_TEAL:GX_WHITE,(1-u)*power*(edge?11:19));if(!edge&&j%12==0)gx_aura(xx,yy,3+u*3,2+u*2,GX_TEAL,power*(1-u)*5);}}
  gx_bloom_line(x-130,y,x+130,y,GX_WHITE,power*12);gx_bloom_line(x,y-46,x,y+46,GX_TEAL,power*6);for(int k=1;k<5;k++)gx_ring(x-k*19,y+k*5,(2+k)*GX_ZOOM,(2+k)*GX_ZOOM,GX_VIOLET,(int)(power*5));
 }
}

static void gx_explosions(void){
 for(int i=0;i<6;i++){en_bloom_fx_t f=en_bloom_fx[i];float life=f.kind==7?24:f.kind==6?20:f.kind==3?1.6f:f.kind==5?3.4f:3.0f;if(f.age>=life)continue;
  if(f.kind==6||f.kind==7){float x=GX_CX+f.x*GX_SX,y=GX_CY+f.y*GX_SY,age=f.age;
   float flare=fm_maxf(0,1-age/(f.kind==7?3.0f:2.2f)),fade=fm_maxf(0,1-age/(f.kind==7?24:20)),r=(8+age*8)*GX_ZOOM*1.4f;
   gx_aura(x,y,(24+age*7)*GX_ZOOM*1.4f,(17+age*5)*GX_ZOOM*1.4f,GX_COLOR[5],flare*22+fade*3);
   for(int shell=0;shell<3;shell++){for(int j=0;j<128;j++){float a=(float)j/128,filament=1+.07f*gx_s(a*11+age*.025f+shell*.17f);gx_aa(x+gx_s(a+.25f)*(r+shell*4)*filament,y+gx_s(a)*(r+shell*4)*.72f*filament,shell==1?GX_VIOLET:GX_TEAL,fade*(3+(j%7)));}}
   if(age<3){gx_radiance(x,y,age,f.kind==7?3.22f:2.52f);for(int ray=0;ray<8;ray++)for(int j=1;j<88;j++){float a=(float)ray/8+age*.006f;gx_aa(x+gx_s(a+.25f)*j,y+gx_s(a)*j,GX_WHITE,flare*(1-(float)j/88)*18);}}
   for(int j=0;j<28;j++){float a=(float)j*.618034f,rr=(12+age*(5+(j%5)))*GX_ZOOM*1.4f;gx_aa(x+gx_s(a+.25f)*rr,y+gx_s(a)*rr*.74f,GX_COLOR[5],fade*(3+j%5));}
   continue;}

  float q=f.age/life,fade=(1-q)*(1-q),power=f.power*(f.kind==4?1.5f:1)*((proj.par[P_REST]==EN_NIGHTFALL)?.60f:1),x=GX_CX+f.x*GX_SX,y=GX_CY+f.y*GX_SY;
  uint16_t c=f.kind==2||f.kind==4?GX_COLOR[5]:GX_COLOR[f.layer];float radius=(f.kind==3?2+q*18:f.kind==2||f.kind==4?3+q*95:13+q*58)*GX_ZOOM;
  gx_aura(x,y,(10+q*22)*GX_ZOOM,(7+q*15)*GX_ZOOM,GX_WHITE,fade*power*12);gx_ring(x,y,radius,radius*.68f,c,(int)(fade*power*12));gx_ring(x,y,radius+3,radius*.68f+2,GX_WHITE,(int)(fade*power*6));
  for(int k=0;k<24;k++){float a=(float)k/24+(float)(f.serial&31u)*.021f+q*.05f,wave=.72f+.28f*gx_s((float)k*.31f+q*.7f),r=radius*wave;if(f.kind==5)r=(1-q)*40*GX_ZOOM;
   float px=x+gx_s(a+.25f)*r,py=y+gx_s(a)*r*.70f;gx_aa(px,py,k%3?c:GX_WHITE,fade*power*16);gx_aa(x+gx_s(a+.25f)*(r-3),y+gx_s(a)*(r-3)*.70f,c,fade*power*7);if(k%4==0)gx_aura(px,py,2.5f,2,c,fade*power*5);
  }
  if(f.kind==0||f.kind==2||f.kind==4){float flash=fm_maxf(0,1-f.age*2.5f)*power;for(int k=-65;k<=65;k++)gx_aa(x+k,y,GX_WHITE,flash*(1-fm_fabsf((float)k)/66)*15);}
 }
}
static void gx_space_job(uint32_t arg){(void)arg;gx_background_fog();gx_starflight(GX_CY);}
static void gx_bloom(void){
 float cy=GX_CY,sx=GX_SX,sy=GX_SY,t=(float)gx_animation_ms()*.001f;
 gx_world3d=0;plat_visual_job(gx_space_job,0);gx_world3d=1;
 /* Constellations connect the actual visited galaxies, never extra tracks. */
 float linkfade=en_bloom_phase==2&&en_bloom_eternal?fm_clampf((32-en_bloom_phase_age)/4,0,1):1;
 for(int l=0;l<8;l++)for(int g=l+1;g<8;g++)if(en_bloom_links[l]&(1u<<g)){
  float light=(3.5f+en_breath*1.5f)*linkfade;gx_bloom_line(GX_CX+en_field_x[l]*sx,cy+en_field_y[l]*sy,GX_CX+en_field_x[g]*sx,cy+en_field_y[g]*sy,gx_mix(GX_TEAL,GX_COLOR[5],5),light);
  float flow=t*.09f+(float)l*.137f;flow-=fm_floorf(flow);gx_aa(GX_CX+(en_field_x[l]+(en_field_x[g]-en_field_x[l])*flow)*sx,cy+(en_field_y[l]+(en_field_y[g]-en_field_y[l])*flow)*sy,GX_WHITE,linkfade*(2+(en_energy[l]+en_energy[g])*5));
 }
 /* The Great Attractor: layered celestial flower and luminous star-heart. */
 float grow=(1+en_growth*.22f)*GX_ZOOM,breath=1+en_breath*.045f,impact=en_core_impact+en_bloom_burst*.3f;
 /* One shared turn every three minutes, independent of musical tempo. */
 float rotation=t/90.0f;
 rotation-=fm_floorf(rotation);
 int petals=6+(int)en_bloom_phase*2;
 gx_aura(GX_CX,cy,76*grow,44*grow,GX_TEAL,5.5f+impact*3);
 gx_aura(GX_CX,cy,43*grow,29*grow,GX_COLOR[5],3.5f+en_breath*1.5f);
 /* Counter-rotating luminous crowns with fine moving filigree. */
 for(int crown=0;crown<3;crown++)for(int j=0;j<168;j++){
  float a=(float)j/168,tide=1+gx_s(a*(6+crown*2)+t*.007f)*.035f;
  float rr=(47+(float)crown*4)*grow*tide,theta=a+rotation+t*(crown&1?-.0015f:.001f);
  gx_aa(GX_CX+gx_s(theta+.25f)*rr,cy+gx_s(theta)*rr*.54f,crown==1?GX_COLOR[5]:GX_TEAL,4+(float)crown+en_breath*2);
  if(j%12==0)gx_aa(GX_CX+gx_s(theta+.25f)*(rr+1.5f),cy+gx_s(theta)*(rr+1.5f)*.54f,GX_WHITE,7);
 }
 for(int petal=0;petal<petals;petal++){
  float a=(float)petal/petals+rotation,r=(17+en_breath*2+(float)en_bloom_phase)*grow;
  gx_ring(GX_CX+gx_s(a+.25f)*r,cy+gx_s(a)*r*.55f,25*grow*breath,13*grow,petal&1?GX_COLOR[5]:GX_TEAL,4+(int)(en_breath*2));
 }
 /* Four streams slowly feed the heart; music illuminates their pearls. */
 for(int arm=0;arm<4;arm++)for(int j=0;j<96;j++){
  float u=(float)j/96,a=u*1.65f+(float)arm/4+rotation-t*.004f,rr=(4+u*39)*grow;
  gx_aa(GX_CX+gx_s(a+.25f)*rr,cy+gx_s(a)*rr*.51f,j%8?GX_TEAL:GX_WHITE,6+j%6+impact*2);
 }
 for(int mote=0;mote<20;mote++){
  float a=(float)mote/20+rotation,rr=(32+(float)(mote%5)*4)*grow;
  float x=GX_CX+gx_s(a+.25f)*rr,y=cy+gx_s(a)*rr*.54f;
  gx_aura(x,y,2,1.6f,mote&1?GX_COLOR[5]:GX_TEAL,3+en_breath*2);gx_aa(x,y,GX_WHITE,7+en_energy[mote&7]*4);
 }
 gx_aura(GX_CX,cy,21*grow,13*grow,GX_WHITE,7+en_breath*2+impact*3);
 gx_aura(GX_CX,cy,10*breath*grow,7*breath*grow,GX_WHITE,15);
 gx_ring(GX_CX,cy,14*breath*grow,8*breath*grow,GX_COLOR[5],11);
 for(int ray=0;ray<6;ray++)for(int j=1;j<22;j++){
  float a=(float)ray/6+rotation,q=1-(float)j/22;
  gx_aa(GX_CX+gx_s(a+.25f)*j*grow,cy+gx_s(a)*j*grow*.65f,GX_WHITE,q*q*(7+en_breath*3+impact*5));
 }
 /* A volumetric celestial heart: rotating meridians rise above the orbit plane. */
 float savedheight=gx_height;
 for(int ring=0;ring<4;ring++)for(int j=0;j<96;j++){
  float a=(float)j/96,axis=(float)ring/4+t*.006f,rr=20*grow;
  float across=gx_s(a+.25f)*rr;gx_height=.16f+gx_s(a)*.19f;
  gx_aa(GX_CX+gx_s(axis+.25f)*across,cy+gx_s(axis)*across,GX_COLOR[ring&1?5:0],5+en_breath*3);
 }
 gx_height=savedheight;
 gx_aa(GX_CX,cy,GX_WHITE,16);
 for(int i=0;i<12;i++){
  float x=GX_CX+en_bloom_spark[i][0]*sx,y=cy+en_bloom_spark[i][1]*sy,life=en_bloom_spark[i][2],blink=0.7f+0.3f*gx_s(t*.13f+(float)i*.17f);
  gx_aa(x,y,GX_COLOR[5],life*blink*14);if(life>.7f){gx_aa(x-1,y,GX_WHITE,life*3);gx_aa(x+1,y,GX_WHITE,life*3);gx_aa(x,y-1,GX_WHITE,life*3);gx_aa(x,y+1,GX_WHITE,life*3);}
 }
 for(int l=0;l<8;l++){
  float parent_height=gx_height;gx_height=gx_galaxy_height(l);gx_opacity=en_presence[l];
  float x=GX_CX+en_field_x[l]*sx,y=cy+en_field_y[l]*sy,awake=fm_maxf(en_bloom_awake[l],(en_bloom_seen&(1u<<l))?.6f:0);
  if(awake>.15f)gx_aura(x,y,31*GX_ZOOM,21*GX_ZOOM,GX_COLOR[l],awake*7);gx_spiral(x,y,(int)((int[]){14,16,12,13,15,11,14,10}[l]*GX_ZOOM),l);
  if(awake>.15f){float a=t*.006f+(float)l*.125f;for(int j=0;j<4;j++){float q=a+(float)j/4;gx_aa(x+gx_s(q+.25f)*20,y+gx_s(q)*12,GX_COLOR[5],awake*9);}}
  if(!en_death[l])for(int p=0;p<en_planets.count[l];p++){float px,py;en_planet_position(l,p,&px,&py);float xx=GX_CX+px*sx,yy=cy+py*sy;gx_world_planet(xx,yy,(1.1f+(float)(p%3)*.3f)*GX_ZOOM,l,p);}
  if(l==ui.sel){gx_ring(x,y,21,14,GX_WHITE,2);for(int j=-2;j<=2;j++)gx_aa(x+j,y+17,GX_WHITE,9);}
  gx_height=parent_height;gx_opacity=1;
 }
 /* Small comet bodies, true curved wakes; keep attention on the spirit. */
 for(int o=1;o<12;o+=3)if(en_object_life[o]>.1f){float life=en_object_life[o],x=GX_CX+en_object_x[o]*sx,y=cy+en_object_y[o]*sy,lastx=x,lasty=y;for(int j=0;j<10;j++){float xx=GX_CX+en_comet_tail[o][j][0]*sx,yy=cy+en_comet_tail[o][j][1]*sy;gx_bloom_line(lastx,lasty,xx,yy,j<3?GX_WHITE:GX_TEAL,life*(9-(float)j*.8f));gx_aa(xx,yy+.8f,GX_VIOLET,life*(4-(float)j*.35f));lastx=xx;lasty=yy;}gx_aura(x,y,5,3.8f,GX_TEAL,life*7);gx_aa(x,y,GX_WHITE,life*16);gx_aa(x-.6f,y-.5f,GX_WHITE,life*11);}
 for(int o=0;o<12;o+=3)if(en_object_life[o]>.1f){float x=GX_CX+en_object_x[o]*sx,y=cy+en_object_y[o]*sy;gx_aa(x,y,GX_DIM,en_object_life[o]*15);gx_aa(x-1.2f,y-.7f,GX_WHITE,en_object_life[o]*11);gx_aa(x+1,y+.8f,GX_TEAL,en_object_life[o]*8);gx_aa(x-1,y+1.1f,GX_DIM,en_object_life[o]*12);}
 if(en_event==5&&!en_grb_phase&&!en_hole_swallowed){float x=GX_CX+en_hole_x*sx,y=cy+en_hole_y*sy;gx_aura(x,y,7,5,GX_VIOLET,3);gx_ring(x,y,4,2.5f,GX_VIOLET,8);if(en_ripple>0)gx_ring(x,y,en_ripple*130,en_ripple*75,GX_VIOLET,4);}
 /* A pearl spirit with wing-like light ribbons, a comet veil and beating halo. */
 for(int j=17;j>=0;j--){float q=1-(float)j/18;gx_aura(GX_CX+en_spirit_tail[j][0]*sx,cy+en_spirit_tail[j][1]*sy,2.8f,2,GX_TEAL,q*3);gx_aa(GX_CX+en_spirit_tail[j][0]*sx,cy+en_spirit_tail[j][1]*sy,GX_WHITE,q*9);}
 float x=GX_CX+en_spirit_x*sx,y=cy+en_spirit_y*sy;
 gx_aura(x,y,12,9,GX_TEAL,5);gx_aura(x,y,4.5f,5.5f,GX_WHITE,12);
 for(int side=-1;side<=1;side+=2)for(int j=0;j<18;j++){float q=(float)j/18,wing=7+en_breath*3;gx_aa(x+side*q*wing,y+gx_s(q*.5f+t*.07f)*3-q*3,GX_WHITE,8*(1-q));gx_aa(x+side*q*wing,y+gx_s(q*.5f+t*.07f)*3-q*3+1,GX_COLOR[5],4*(1-q));}
 gx_ring(x,y-6,4,1.6f,GX_COLOR[5],8);gx_aa(x,y,GX_WHITE,16);
 gx_capture_burst(0);if(en_bloom_burst>.01f)gx_ring(GX_CX,cy,25+(1-en_bloom_burst)*100,14+(1-en_bloom_burst)*65,GX_COLOR[5],(int)(en_bloom_burst*5));gx_fog(1);
 if(!ui.idle&&ui.view==V_OBSERVATORY){cv_text(8,43,&FONT_XS,BLOOM_PHASE[en_bloom_phase],GX_TEAL);for(int l=0;l<8;l++)gx_aa(82+(float)l*11,144,(en_bloom_seen&(1u<<l))?GX_COLOR[5]:GX_TEAL,(en_bloom_seen&(1u<<l))?10:3);}
}
static void gx_sky(void){float t=(float)gx_animation_ms()*.001f;for(int i=0;i<3;i++){
 en_sky_t o=en_sky[i];float x=GX_CX+o.x*GX_SX,y=GX_CY+o.y*GX_SY;gx_height=.12f+(float)i*.07f;
 if(o.kind==1){float a=t*(.12f+i*.04f),shine=3+o.light*8;gx_aura(x,y,6,5,GX_TEAL,shine);gx_aa(x,y,GX_WHITE,16);
  for(int side=-1;side<=1;side+=2)for(int j=1;j<30;j++){float len=j*side;gx_aa(x+gx_s(a+.25f)*len,y+gx_s(a)*len*.65f,GX_TEAL,(1-(float)j/30)*shine);}}
 else{gx_aura(x,y,20,13,GX_VIOLET,5+en_visual[o.layer]*10);for(int k=0;k<3;k++)gx_ring(x,y,9+k*3,3+k*1.5f,GX_COLOR[5],4+k);
  for(int side=-1;side<=1;side+=2)for(int j=0;j<34;j++){float bend=gx_s(t*.08f-j*.017f)*j*.10f;gx_aa(x+bend,y+side*j,GX_WHITE,(1-(float)j/34)*(6+o.light*5));}gx_aa(x,y,GX_WHITE,16);}}
 gx_height=0;}
static int gx_fx_snapshot(int i,en_bloom_fx_t *out){uint32_t id=en_bloom_fx[i].nova;__asm__ volatile("" ::: "memory");*out=en_bloom_fx[i];__asm__ volatile("" ::: "memory");return id==out->nova&&id==en_bloom_fx[i].nova;}
/* One continuous flash per major event, no flicker train or extra screen pass.
 * Return the strongest flash, never add overlapping flashes. Nightfall softens. */
static unsigned gx_major_flash(void){float light=0;
 for(int i=0;i<6;i++){en_bloom_fx_t f;if(!gx_fx_snapshot(i,&f)||(f.kind!=6&&f.kind!=7)||f.power<.01f||f.age<0||f.age>=.55f)continue;
  float q=(1-f.age/.55f)*fm_clampf(f.power,0,1);if(q>light)light=q;}
 return (unsigned)(light*(proj.par[P_STYLE]==EN_NIGHTFALL?2:7));
}
static float gx_camera_yaw(float t){return ui.view==V_OBSERVATORY?t*.008f+.11f*gx_s(t*.012f)+gx_nova_yaw:page_yaw;}
static float gx_camera_pitch(float t){return ui.view==V_OBSERVATORY?.105f+.095f*gx_s(t*.011f)+gx_nova_pitch:.135f+page_pitch;}
#include "celestial_runtime.h"
#include "resonance_worlds.h"
static void gx_visualizer(void){resonance_present();}


#define FX_PEARL GX_PANEL
#define FX_INK GX_WHITE
static int fx_dock_height(void){return proj.par[P_TEXT]==2?84:proj.par[P_TEXT]==1?80:76;}
static int fx_bottom(void){return 240-fx_dock_height();}
static unsigned ui_text_clips;
static void fx_text(int x,int y,int w,const char *s,uint16_t c,const felucca_font_t *f){
 /* Preserve the selected readable size where it fits; use a compact face
  * for narrow labels instead of clipping a word. */
 if(text_w(f,s)>w&&f->h>FONT_S.h)f=&FONT_S;
 if(text_w(f,s)>w&&f->h>FONT_XS.h)f=&FONT_XS;
 char b[64];int n=0,pixels=0;while(s[n]&&n<62){unsigned k=glyph(f,(unsigned char)s[n]);int advance=f->adv[k];if(pixels+advance>w)break;b[n]=s[n];pixels+=advance;n++;}if(s[n]){ui_text_clips++;
#ifdef EN_HOST
 fprintf(stderr,"TEXT CLIP x%d y%d w%d: %s\n",x,y,w,s);
#endif
 }if(s[n]&&n>2){b[n-1]='.';b[n-2]='.';}b[n]=0;cv_text(x,y,f,b,c);}
static void fx_small(int x,int y,int w,const char *s,uint16_t c){fx_text(x,y,w,s,c,gx_font());}
static void fx_body(int x,int y,int w,const char *s,uint16_t c){fx_text(x,y,w,s,c,gx_font());}
static void fx_panel(int x,int y,int w,int h,uint16_t c){(void)c;const float r=5;for(int j=0;j<h;j++){float v=j<5?r-((float)j+0.5f):j>=h-5?((float)j+0.5f)-(h-r):0;float inset=v>0?r-fm_sqrtf(fm_maxf(0,r*r-v*v)):0;int edge=(int)inset;uint16_t fill=gx_mix(GX_PANEL,GX_BG,j*8/(h?h:1));cv_rect(x+edge+1,y+j,w-edge*2-2,1,fill);float cov=1-(inset-edge);gx_glow(x+edge,y+j,GX_LINE,(int)(cov*10));gx_glow(x+w-1-edge,y+j,GX_LINE,(int)(cov*10));}cv_line(x+6,y,x+w-7,y,GX_LINE);cv_line(x+7,y+1,x+w-8,y+1,gx_mix(GX_PANEL,GX_TEAL,3));cv_line(x+6,y+h-1,x+w-7,y+h-1,gx_mix(GX_BG,GX_LINE,6));gx_aa(x+3.5f,y+6,GX_TEAL,7);}
static void fx_glyph(int x,int y,int r,uint16_t c){cv_line(x,y-r,x+r,y,c);cv_line(x+r,y,x,y+r,c);cv_line(x,y+r,x-r,y,c);cv_line(x-r,y,x,y-r,c);cv_pset(x,y,GX_WHITE);}
static const char *fx_track_state(int l){uint32_t f=en_track_flags(l);return en_death[l]>0?"REBIRTH":f&EN_TRACK_OFF?"OFF":f&EN_TRACK_MUTED?"MUTE":f&EN_TRACK_SOLO_OUT?"SOLO":f&EN_TRACK_SCENE_OUT?"WAIT":f&EN_TRACK_LEVEL_ZERO?"LEVEL 0":f&EN_TRACK_DENSITY_ZERO?"EMPTY":f&EN_TRACK_REST?"REST":"LIVE";}
static void fx_power(void){int level=plat_battery_level(),usb=plat_usb_power(),x=207,y=10;
 cv_rect(x,y,21,1,GX_TEAL);cv_rect(x,y+10,21,1,GX_TEAL);cv_rect(x,y,1,11,GX_TEAL);cv_rect(x+20,y,1,11,GX_TEAL);cv_rect(x+21,y+3,2,5,GX_TEAL);
 int pct=plat_battery_percent();char label[12];if(usb)fx_small(174,9,30,"USB",GX_TEAL);
 if(pct<0)fx_small(x+5,y,12,"?",GX_DIM);else{for(int i=0;i<(pct+24)/25&&i<4;i++)cv_rect(x+2+i*4,y+3,3,5,pct<25?GX_WHITE:GX_TEAL);if(!usb){label[0]='~';gx_num(label+1,pct);int n=0;while(label[n])n++;label[n++]='%';label[n]=0;fx_text(171,9,34,label,GX_TEAL,&FONT_XS);}}

}
static void fx_header(void){char b[24];ui_locate_page();fx_panel(5,3,230,25,FX_PEARL);gx_num(b,ui_area+1);gx_str(b+1,"/5 ",23);gx_str(b+4,UI_AREA[ui_area],20);fx_text(12,6,184,b,FX_INK,&FONT_B);fx_power();fx_text(8,30,148,GX_VIEW[ui.view],GX_TEAL,&FONT_XS);fx_text(160,30,72,GALAXY_NAME[ui.view==V_MIXER?ui.lane:ui.sel],GX_DIM,&FONT_XS);}
static const char *fx_hint(void){switch(ui.view){case V_COMPOSITION:case V_TIMING:return "* PENDING / APPLIES AT SAFE BAR";case V_STRUM:return "K1 TOGETHER/STRUM K2 GAP 80-200";case V_MOTIF_MODE:case V_EXPRESSION:return "K1-4 SHAPE / SELECT IN EDIT:ROLE";case V_BLUETOOTH:return "K2 -FAR +NEAR / EDIT: MORE";case V_OBSERVATORY:if(en_control_pending(P_STRUM)||en_control_pending(P_CHORD_PLAYBACK))return "* NEW VALUE / NEXT SAFE BAR";if(en_chord_mode>=4)return "WHITE: NOTE / BLACK: CHORD";return en_chord_mode?"CHORD KEYS / PERFECT NOTE":"SINGLE KEYS / PERFECT NOTE";case V_MATRIX:return "K1 LEVEL K2 PAN K3 MUTE K4 SOLO";case V_PLANETS:return "SELECT: WORLD / PRESET: INTENT";case V_SCENES:return "K1: SCENE / K2: GALAXY";case V_DRUMS:return "K1 ON/OFF K2 KIT K3 LEVEL K4 HITS";case V_DRUMFX:return "ARP: DRUM PARTS / K1-4: FX";case V_BASS:return "EDIT: NEXT BASS CONTROLS";case V_INTENT:return "PRESETS: RECIPE / K1: FREQUENCY";case V_VOICE:return "LIVE: STRUM / EDIT: WORLDS";default:return "HOME: LIVE / SAVE: KEEP";}}
static void fx_dock(void){char b[24],label[32];int h=fx_dock_height(),top=240-h,row=(h-24)/2;fx_text(7,top-17,226,fx_hint(),GX_DIM,&FONT_XS);
 int count=0;for(int k=0;k<4;k++)if(gx_param(ui.view,k)!=GX_UNUSED)count++;
 int slot=0;for(int k=0;k<4;k++){int p=gx_param(ui.view,k);if(p==GX_UNUSED)continue;int wide=count<=2||(count==3&&slot==2),width=wide?230:112;int x=5+(wide?0:(slot&1)*118),y=top+(count<=2?slot:slot/2)*(row+4);slot++;fx_panel(x,y,width,row,FX_PEARL);gx_num(label,k+1);int q=1;label[q++]=' ';const char *name;
  if(ui.view==V_BLUETOOTH){if(air_page==2){name=k?"RESONANCE":"CUTOFF";gx_num(b,k?air_resonance:air_cutoff);}else if(!air_page){name=(const char *const[]){"BT","CALIBRATE","DEPTH","TRACK"}[k];if(k==0)gx_str(b,air_enabled?"AIR":"OFF",24);else if(k==1)gx_str(b,"-FAR / +NEAR",24);else if(k==2)gx_num(b,air_prefs.depth);else gx_str(b,air_prefs.track==7?"DRUMS":GALAXY_NAME[air_prefs.track],24);}else{name=(const char *const[]){"SENSITIVITY","SMOOTHING","INVERT","DESTINATION"}[k];if(k==0)gx_num(b,air_prefs.sensitivity);else if(k==1)gx_num(b,air_prefs.smoothing);else if(k==2)gx_str(b,air_prefs.invert?"ON":"OFF",24);else gx_str(b,(const char *const[]){"Room send","Echo send","Tone","Pitch / octave","Cutoff","Resonance"}[air_prefs.destination],24);}}
  else if(p==GX_SAVER||p==GX_UNLOCK){name=p==GX_SAVER?"Battery Saver":"Unlock FPS";gx_str(b,(p==GX_SAVER?proj.par[P_POWER]==5:project_display_normal==2)?"ON":"OFF",24);}
  else if(p==GX_UNUSED){name="Unassigned";gx_str(b,"--",24);}
  else if(p==GX_PATCH_AUTO){name="Patch source";gx_str(b,(proj.patch_locks&(1u<<ui.sel))?"Manual":"Auto",24);}
  else if(p==GX_QUALITY){name="Synth rate";gx_str(b,project_quality?"48 kHz":"44.1 kHz",24);}
  else if(p==GX_LIGHTS){name="Lights";gx_str(b,proj.leds==3?"Dance":proj.leds==2?"Keys":proj.leds?"On":"Off",24);}

  else if(ui.view==V_PLANETS){name=(const char *const[]){"Tone/Colour","Level/Size","Space/Glow","Hold/Rings"}[k];int count=en_planets.count[ui.sel];if(ui.planet>=count)ui.planet=0;en_planet_t v=en_planets.planet[ui.sel][ui.planet];gx_num(b,((uint8_t*)&v)[k]);if(k==3){int n=0;while(b[n])n++;gx_str(b+n,v.hold>=65?" / On":" / Off",24-n);}}
  else{name=ui.view==V_DRUMS&&k==3?"Drum Hits":p==P_FOCUS?"Galaxy":en_param_info(p)->name;en_param_text(p,proj.par[p],b);}
  if(en_control_pending(p)){int n=0;while(b[n])n++;gx_str(b+n," *",24-n);}
  gx_str(label+q,name,32-q);fx_text(x+7,y+1,width-14,label,FX_INK,&FONT_XS);fx_text(x+7,y+16,width-14,b,FX_INK,gx_font());
 }
}
static uint8_t ui_galaxy_tiles;
static void fx_ensemble(void){ui_galaxy_tiles=0;
 for(int l=0;l<EN_NGAL;l++){
  int x=5+(l&1)*118,y=49+(l/2)*43;uint16_t color=GX_COLOR[l];fx_panel(x,y,112,39,GX_PANEL);
  if(l==ui.sel){cv_line(x+5,y+37,x+106,y+37,GX_TEAL);cv_line(x+1,y+5,x+1,y+32,GX_TEAL);}
  resonance_glyph(x+17,y+19,13,l);
  char label[24];label[0]=(char)('1'+l);label[1]=' ';gx_str(label+2,GALAXY_NAME[l],22);
  fx_text(x+32,y+4,77,label,l==ui.sel?GX_WHITE:color,&FONT_S);fx_text(x+32,y+22,77,fx_track_state(l),GX_DIM,&FONT_XS);ui_galaxy_tiles|=(uint8_t)(1u<<l);
 }
 fx_panel(123,178,112,39,GX_PANEL);fx_small(131,180,100,"8 PERCUSSION",GX_COLOR[7]);fx_text(131,200,100,"REC: DRUMS",GX_DIM,&FONT_XS);
 fx_small(8,224,224,"SELECT ROLE / SEL: MUTE",GX_DIM);
}

static void fx_worlds(void){int l=ui.sel,n=en_planets.count[l],bottom=ui.idle?232:fx_bottom()-15;if(ui.planet>=n)ui.planet=0;
 for(int p=0;p<n&&p<8;p++){int x=27+(p%4)*60,y=94+(p/4)*66;resonance_glyph(x,y,15,l);if(p==ui.planet)cv_line(x-15,y+22,x+15,y+22,GX_WHITE);char value[12];gx_num(value,en_planets.planet[l][p].pitch);fx_small(x-18,y+25,40,value,p==ui.planet?GX_WHITE:GX_DIM);}
 fx_small(10,47,218,"SELECT NODE / K1-K4 SHAPE",GX_DIM);
}
static void fx_voice(void){int l=ui.sel,bt=fx_bottom()-16;resonance_glyph(54,82,27,l);fx_small(95,48,137,"VOICE ENGINE",GX_DIM);fx_body(95,67,137,VOICE_NAME[proj.par[LP(l,L_VOICE)]],GX_WHITE);fx_small(95,92,137,(proj.patch_locks&(1u<<l))?"MANUAL / LOCKED":"AUTO / ROLE PALETTE",GX_COLOR[l]);
 int lastx=8,lasty=bt;for(int i=1;i<=90;i++){float u=(float)i/90,a=(float)proj.par[LP(l,L_ATTACK)]*0.005f+0.05f;float env=u<a?u/a:fm_expf(-(u-a)*(1.2f+(100-proj.par[LP(l,L_RELEASE)])*0.04f));int x=8+i*224/90,y=bt-(int)(env*fm_maxf(14,bt-113));cv_line(lastx,lasty,x,y,GX_TEAL);lastx=x;lasty=y;}}
static void fx_phrase(void){int l=ui.sel,n=16<<proj.par[LP(l,L_STEPS)],bt=fx_bottom()-17,rows=n/16;float dh=(float)(bt-53)/rows;for(int i=0;i<n;i++){int x=10+(i%16)*14,y=53+(int)((i/16)*dh);int hit=en_note_at(l,(unsigned)i);int height=hit?6+(hit*3)%((int)dh>17?14:8):2;cv_rect(x,y+((int)dh-height)/2,9,height,hit?GX_COLOR[l]:GX_PANEL);if(i==en_layer_step[l])fx_glyph(x+4,y+(int)dh-3,3,GX_WHITE);}char b[12];gx_num(b,n);fx_small(10,43,50,b,GX_WHITE);fx_small(51,43,180,"STEPS / LIVING MOTIF",GX_DIM);}
static void fx_journey_landscape(void);
static void fx_journey(void){int sc=proj.par[P_SCENE];fx_small(10,48,220,SCENE_NAME[sc],GX_TEAL);fx_small(10,66,220,en_scene_pending<8?"QUEUED / NEXT BAR":proj.par[P_AUTO]?"AUTO / SCENE PATH":"MANUAL / YOUR JOURNEY",GX_DIM);for(int l=0;l<7;l++)fx_glyph(23+l*32,90,l==ui.sel?5:3,en_scene_mask(sc)&(1u<<l)?GX_COLOR[l]:GX_LINE);fx_journey_landscape();}

static void fx_rhythm(void){int l=ui.view==V_DRUMS?7:1,bt=fx_bottom()-16;fx_panel(7,48,226,43,FX_PEARL);fx_text(17,52,207,l==7?(const char *const[]){"OFF","SPARSE","GROOVE"}[proj.par[P_DRUMS]]:VOICE_NAME[proj.par[LP(1,L_VOICE)]],FX_INK,&FONT_S);if(l==1){fx_text(12,75,216,(proj.patch_locks&2)?"BASS / MANUAL PINNED":"BASS / AUTO PATCH",GX_COLOR[1],&FONT_XS);fx_text(12,112,216,(const char *const[]){"EDIT: SOUND > ENVELOPE > RHYTHM > STRUM","ENVELOPE / GLIDE / BASS PART","DENSITY / GATE / RATE / BASS STRUM","TOGETHER / STRUM GAP / DIRECTION"}[ui_bass_controls],GX_TEAL,&FONT_XS);}
 uint32_t f=en_track_flags(l);const char *why=f&1?"TURN K1 TO ENABLE":f&2?(l==7?"MIXER: UNMUTE DRUMS":"MUTED / MIX K3"):f&4?"SOLO ACTIVE / MIX K4":f&16?(l==7?"LEVEL ZERO / TURN K3":"LEVEL ZERO / TURN K2"):f&32?(l==7?"NO HITS / TURN K4":"DENSITY ZERO"):f&64?"REST / PRESS PLAY":"FOUNDATION / ALL SCENES";fx_text(12,92,218,why,GX_TEAL,&FONT_XS);if(l==7){fx_small(12,76,154,KIT_NAME[proj.par[P_KIT]],FX_INK);fx_small(174,76,52,proj.par[P_DRUM_AUTO]?"AUTO":"MANUAL",GX_TEAL);char count[32],v[8];int hits=0;for(int q=0;q<16;q++)hits+=en_drum_note((unsigned)q)!=0;gx_str(count,"HITS ",32);gx_num(v,hits);gx_str(count+5,v,27);int n=0;while(count[n])n++;gx_str(count+n," /16 / ",32-n);while(count[n])n++;gx_str(count+n,(proj.par[P_REST]==EN_NIGHTFALL)?"NIGHT":!proj.par[P_EVOLVE]?"LOOP":en_drum_phase==2?"BREATHE":en_drum_phase==1?"FILL":"EVOLVE",32-n);fx_text(12,108,214,count,GX_DIM,&FONT_XS);}
 for(int i=0;i<16;i++){int x=10+i*14,hit=l==7?en_drum_note((unsigned)i):en_note_at(l,(unsigned)i);int hh=hit?12:3;cv_rect(x,bt-16,9,hh,hit?GX_COLOR[l]:GX_PANEL);if(en_trigger[l]&(1u<<i))cv_rect(x,bt-2,9,2,GX_WHITE);if(i==en_step)fx_glyph(x+4,bt+3,3,GX_TEAL);}}
static int fx_mix_level(int l){int manual=proj.par[l==EN_DRUM_TRACK?P_DRUM_LEVEL:LP(l,L_LEVEL)];return (int)((float)manual*(proj.par[P_AUTO_LEVEL]?en_balance[l]:1)+.5f);}
/* -48..0dBFS display range: useful motion for normal quiet ambient tracks.
 * DSP publishes a conservative post-fader stereo dry peak bound. */
static int fx_meter_height(float peak){if(peak<=.0039810717f)return 0;float db=6.0205999f*fm_log2f(peak);return (int)fm_clampf((db+48)*(115.0f/48),0,115);}
static void fx_balance(void){for(int l=0;l<8;l++){int x=8+l*29,manual=proj.par[l==EN_DRUM_TRACK?P_DRUM_LEVEL:LP(l,L_LEVEL)],level=fx_mix_level(l),height=(level>100?100:level)*115/100;uint16_t c=GX_COLOR[l];fx_small(x,47,24,(const char *const[]){"P","B","L","C","H","A","M","D"}[l],l==ui.lane?GX_WHITE:c);fx_panel(x-2,66,24,127,GX_PANEL);cv_rect(x+4,71,9,115,GX_BG);cv_rect(x+4,186-height,9,height,gx_mix(GX_BG,c,11));cv_line(x-1,186-manual*115/100,x+2,186-manual*115/100,GX_DIM);if(level>100)fx_small(x+5,60,10,"+",c);int mh=fx_meter_height(en_lane_rms[l]),ph=fx_meter_height(en_lane_peak[l]);cv_pset(x+16,186-ph,GX_WHITE);cv_rect(x+16,186-mh,2,mh,en_clip[l]?RGB(255,70,55):mh>100?RGB(255,210,70):c);cv_line(x+1,186-height,x+15,186-height,l==ui.lane?GX_WHITE:c);if(l==ui.lane)cv_line(x-1,196,x+19,196,GX_TEAL);char b[8];gx_num(b,level);fx_small(x,199,23,b,GX_WHITE);uint32_t flags=en_track_flags(l);fx_small(x+13,47,10,en_clip[l]?"!":proj.par[LP(l,L_SOLO)]?"S":flags&2?"M":flags&4?"-":"",en_clip[l]?RGB(255,70,55):GX_DIM);}char detail[32],*at=detail;gx_str(at,"PAN ",32);at+=4;gx_num(at,proj.par[LP(ui.lane,L_PAN)]);while(*at)at++;gx_str(at," / R ",32-(int)(at-detail));while(*at)at++;gx_num(at,proj.par[ui.lane==EN_DRUM_TRACK?P_DRUM_ROOM:LP(ui.lane,L_REVERB)]);while(*at)at++;gx_str(at," / E ",32-(int)(at-detail));while(*at)at++;gx_num(at,proj.par[ui.lane==EN_DRUM_TRACK?P_DRUM_ECHO:LP(ui.lane,L_ECHO)]);fx_small(7,227,228,ui_mixer_sends?"K1 VOL K2 PAN K3 ROOM K4 ECHO":"K1 VOL K2 PAN K3 MUTE K4 SOLO",GX_WHITE);fx_small(7,213,228,ui_edit?"SELECT: TRACK / EDIT: SENDS":"SEL: TRACK EDIT / EDIT: SENDS",GX_TEAL);}
/* Bounded analytic displays. Never imply these curves measure audio. */
static void fx_value(int x,int y,const char *name,int value,const char *unit){char b[40],n[12];gx_str(b,name,40);int k=0;while(b[k])k++;gx_num(n,value);gx_str(b+k,n,40-k);while(b[k])k++;gx_str(b+k,unit,40-k);fx_small(x,y,228-x,b,GX_DIM);}
#include "living_graphs.h"

static void fx_bluetooth(void){
 fx_text(12,51,216,"CONNECTIVITY / BT AIR",GX_WHITE,&FONT_S);
 fx_text(12,73,216,(const char *const[]){"OFF","SCANNING","SIGNAL","NO SIGNAL","ERROR / RADIO UNAVAILABLE"}[air_status],GX_TEAL,&FONT_S);
 if(air_status==2)fx_value(12,94,"RSSI ",air_raw," dBm");
 else fx_small(12,94,105,"NO LIVE RSSI",GX_DIM);
 cv_rect(120,95,103,5,GX_PANEL);
 cv_rect(120,95,(unsigned)air_value*103u/32767u,5,GX_TEAL);
 if(air_prefs.calibrated&1)fx_value(12,108,"FAR ",air_prefs.far_rssi," dBm");
 else fx_small(12,108,216,"CALIBRATE FAR THEN NEAR",GX_DIM);
 if(air_prefs.calibrated&2)fx_value(120,108,"NEAR ",air_prefs.near_rssi," dBm");
 if(air_test){fx_small(12,125,216,"TEST: ROOM SEND / RELEASE SEL",GX_TEAL);return;}
 if(air_prefs.destination==3&&air_prefs.track<7){
  int request=(int)((unsigned)air_value*air_prefs.depth/32767u);
  fx_value(12,125,"PITCH REQUEST ",request,"%");
  fx_value(12,141,"LAST NOTE ",(int)(en_freq[air_prefs.track]+.5f)," Hz");
 }else{
  if(air_prefs.calibrated==3){
   int span=air_prefs.near_rssi-air_prefs.far_rssi;
   fx_value(12,125,"SPAN ",span<0?-span:span," dB");
   fx_small(100,125,128,"HOLD SEL: TEST",GX_TEAL);
  }
 }
}

static void fx_auto_mix(void){
 fx_small(12,52,216,"INDEPENDENT / AUTO-SAVED",GX_TEAL);
 fx_small(12,80,216,proj.par[P_AUTO_LEVEL]?"K1 AUTO LEVEL: ON":"K1 AUTO LEVEL: OFF",GX_WHITE);
 fx_small(12,105,216,proj.par[P_AUTO_DUCK]?"K2 AUTO DUCK: ON":"K2 AUTO DUCK: OFF",GX_WHITE);
 fx_small(12,130,216,"FPS OVERLAY: DIAGNOSTICS",GX_DIM);
 fx_small(12,157,216,"SPEAKER EQ: OUTPUT",GX_DIM);
}
static void fx_drumparts(void){
 static const char *const name[5]={"KICK / BASS DRUM","SNARE","CLOSED HI-HAT","OPEN HI-HAT","CYMBAL / CRASH"};
 for(int i=0;i<5;i++){int y=49+i*29;fx_small(12,y,164,name[i],i==ui.drum_part?GX_WHITE:GX_DIM);fx_small(181,y,47,project_drum_parts&(1u<<i)?"ON":"OFF",project_drum_parts&(1u<<i)?GX_TEAL:GX_DIM);if(i==ui.drum_part)cv_line(10,y+19,229,y+19,GX_LINE);}
 fx_small(12,205,216,"SEL: EDIT / SELECT: PART",GX_TEAL);fx_small(12,224,216,"K1 ON/OFF / ARP: PERCUSSION",GX_DIM);
}
static void fx_debug(void){
 if(ui.debug_detail==11){uint32_t v[6];char b[40];plat_audio_metrics(v);static const char *const n[6]={"CALLBACK LAST US ","CALLBACK MAX US ","DEADLINE US ","DEADLINE MISSES ","DMA UNDERRUNS ","BLOCKS RENDERED "};for(unsigned q=0;q<6;q++){gx_str(b,n[q],40);unsigned at=0;while(b[at])at++;gx_num(b+at,v[q]);fx_small(12,53+q*25,216,b,GX_WHITE);}fx_small(12,211,216,"FPS ms = DISPLAY FRAME INTERVAL",GX_DIM);return;}

 if(ui.debug_detail==9||ui.debug_detail==10){uint32_t v[6];char b[40];static const char *const names[6]={"CONTROL US ","WORLD/SETUP US ","VOICES US ","MIX/FX US ","QUEUE DROPS ","CLOCK COALESCED "};for(unsigned q=0;q<4;q++)v[q]=ui.debug_detail==9?en_dsp_stage_us[q]:en_dsp_stage_max_us[q];v[4]=en_queue_drops;v[5]=en_clock_coalesced;for(unsigned q=0;q<6;q++){gx_str(b,names[q],40);unsigned at=0;while(b[at])at++;gx_num(b+at,v[q]);fx_small(12,53+q*25,216,b,GX_WHITE);}fx_small(12,211,216,ui.debug_detail==9?"LAST BLOCK / K3: MAX":"MAX SINCE BOOT / NOT FPS",GX_DIM);return;}

 if(ui.debug_detail==7||ui.debug_detail==8){char b[40];uint32_t v[6];static const char *const timing[6]={"TEMPO APPLY MS ","TEMPO EVENT MS ","STRUM APPLY MS ","STRUM CHORD MS ","MODE APPLY MS ","MODE CHORD MS "};static const char *const input[6]={"DISPATCH P50 US ","DISPATCH P95 US ","DISPATCH P99 US ","UI SUBMIT P50 US ","UI SUBMIT P95 US ","UI SUBMIT P99 US "};for(unsigned q=0;q<6;q++){v[q]=ui.debug_detail==7?en_control_latency(q/2,q&1):plat_control_percentile(q/3,(unsigned[]){50,95,99}[q%3]);gx_str(b,(ui.debug_detail==7?timing:input)[q],40);unsigned n=0;while(b[n])n++;if(v[q]==~0u)gx_str(b+n,"PENDING",40-n);else gx_num(b+n,v[q]);fx_small(12,53+q*25,216,b,GX_WHITE);}fx_small(12,216,216,ui.debug_detail==7?"DSP CLOCK / 256-FRAME RESOLUTION":"8192us BINS / >=131072us OVERFLOW",GX_DIM);return;}

 if(ui.debug_detail==5||ui.debug_detail==6){char b[40];uint32_t values[6];static const char *const input[6]={"DISPATCH LAST US ","DISPATCH MAX US ","UI SUBMIT LAST US ","UI SUBMIT MAX US ","COALESCED STEPS ","INPUT STAMP "};static const char *const audio[6]={"CALLBACK P50 US ","CALLBACK P95 US ","CALLBACK P99 US ","CORE0 P95 US ","CORE1 P95 US ","SCAPE DECODES "};if(ui.debug_detail==5)plat_control_metrics(values);else{values[0]=plat_audio_percentile(50);values[1]=plat_audio_percentile(95);values[2]=plat_audio_percentile(99);values[3]=en_core_percentile(0,95);values[4]=en_core_percentile(1,95);values[5]=en_soundscape_decodes;}for(unsigned q=0;q<6;q++){gx_str(b,(ui.debug_detail==5?input:audio)[q],40);unsigned n=0;while(b[n])n++;gx_num(b+n,(int)values[q]);fx_small(12,53+q*25,216,b,GX_WHITE);}fx_small(12,217,216,ui.debug_detail==5?"UI SUBMIT / NOT PHOTON LATENCY":"512us BINS / >=8192us OVERFLOW",GX_DIM);return;}

 if(ui.debug_detail==4){uint32_t m[4];char b[40];plat_input_metrics(m);fx_small(12,48,216,"NATIVE INPUT / LIVE COUNTERS",GX_TEAL);
  static const char *const names[4]={"SCAN FRAMES ","PHYSICAL HELD ","PENDING PRESS ","SCAN COLUMN "};
  for(unsigned i=0;i<4;i++){gx_str(b,names[i],40);unsigned n=0;while(b[n])n++;gx_num(b+n,m[i]);fx_small(12,78+i*26,216,b,GX_WHITE);}
  fx_small(12,194,216,"ALGO: PAGE / DISPLAY: SAVER OFF",GX_TEAL);fx_small(12,218,216,"SCAN COUNTER IS HARDWARE ONLY",GX_DIM);return;
 }

 if(ui.debug_detail==3){char b[40];fx_small(12,48,216,EN_CELESTIAL_3D?"3D ACTIVE / DRAW LAST+MAX":"PLANAR FALLBACK / LAST+MAX",GX_TEAL);static const char *const labels[8]={"FOG ","STARS ","TAILS ","GALAXIES ","CORE ","EVENTS ","UI ","FLUSH "};for(unsigned i=0;i<8;i++){gx_str(b,labels[i],40);unsigned n=0;while(b[n])n++;gx_num(b+n,(int)profile_last[i]);while(b[n])n++;gx_str(b+n," / ",40-n);while(b[n])n++;gx_num(b+n,(int)profile_worst[i]);fx_small(12,66+i*16,216,b,GX_WHITE);}gx_str(b,"P95 H/F ~  ",40);gx_num(b+11,profile_p95(0));unsigned n=0;while(b[n])n++;gx_str(b+n," / ",40-n);while(b[n])n++;gx_num(b+n,profile_p95(1));fx_small(12,199,216,b,GX_TEAL);fx_small(12,222,216,"8ms BINS / 128ms OVERFLOW",GX_DIM);return;}

 if(ui.debug_detail==2){uint32_t metrics[8];char b[40];plat_bt_metrics(metrics);fx_small(12,48,216,"BLUETOOTH / LOCAL METRICS",GX_TEAL);
  static const char *const labels[8]={"START STAGE ","HEAP HIGH B ","ALLOC FAIL ","SERVICE MAX US ","IRQ LEAKS ","RX DROPS ","TX DROPS ","LOCAL Q MAX MS "};
  for(unsigned i=0;i<8;i++){gx_str(b,labels[i],40);unsigned n=0;while(b[n])n++;gx_num(b+n,(int)metrics[i]);fx_small(12,68+(int)i*18,216,b,GX_WHITE);}
  fx_small(12,222,216,"K3 INFO / RADIO NEEDS FM1",GX_DIM);return;
 }
char b[40],*at;
 if(ui.debug_detail){
  fx_small(12,48,216,"MIX / MEMORY / INPUT",GX_TEAL);
  fx_small(12,69,216,proj.par[P_AUTO_LEVEL]?"AUTO LEVEL: ON":"AUTO LEVEL: OFF",GX_WHITE);
  fx_small(12,90,216,proj.par[P_AUTO_DUCK]?"AUTO DUCK: ON":"AUTO DUCK: OFF",GX_WHITE);
  gx_str(b,"MASTER ",40);gx_num(b+7,(int)(plat_master()*100u/4096u));at=b;while(*at)at++;gx_str(at,"%",40-(int)(at-b));fx_small(12,111,216,b,GX_WHITE);
  gx_str(b,"PEAK GUARD ",40);gx_num(b+11,(int)(en_mix_limit*100));at=b;while(*at)at++;gx_str(at,"% GAIN",40-(int)(at-b));fx_small(12,132,216,b,GX_WHITE);
  gx_str(b,"STATIC ",40);gx_num(b+7,(int)(plat_ram_used()/1024));at=b;while(*at)at++;gx_str(at," KiB",40-(int)(at-b));fx_small(12,153,216,b,GX_WHITE);
  gx_str(b,"C1 STACK ",40);gx_num(b+9,(int)plat_core_stack_used());at=b;while(*at)at++;gx_str(at," B",40-(int)(at-b));fx_small(12,174,216,b,GX_WHITE);
  fx_small(12,195,216,ui.wake_reason==1?"WAKE: BUTTON":ui.wake_reason==2?"WAKE: KNOB":"WAKE: BOOT",GX_DIM);
  fx_small(12,222,216,"K3: PERFORMANCE / OC: OFF",GX_DIM);return;
 }

 const char *render=(const char *const[]){"HD 20","HD 20","UNLOCK FPS","HD 20","HD 20","SAVER 10"}[proj.par[P_POWER]];
 gx_str(b,render,40);at=b;while(*at)at++;gx_str(at," / ",40-(int)(at-b));while(*at)at++;gx_str(at,(const char *const[]){"LIVE","AUDIO TEST","GRAPHICS TEST","CORE0 TEST"}[en_diagnostic_mode&3],40-(int)(at-b));at=b;while(*at)at++;gx_str(at," " EN_VERSION,40-(int)(at-b));fx_small(12,48,216,b,GX_TEAL);
 gx_num(b,(int)ui_fps);at=b;while(*at)at++;gx_str(at," FPS / FRAME ",40-(int)(at-b));while(*at)at++;gx_num(at,(int)ui_frame_ms);while(*at)at++;gx_str(at,"ms",40-(int)(at-b));fx_small(12,69,216,b,GX_WHITE);
 gx_num(b,(int)ui_cpu_draw_ms);at=b;while(*at)at++;gx_str(at,"ms UI / LCD ",40-(int)(at-b));while(*at)at++;gx_num(at,(int)ui_lcd_wait_ms);while(*at)at++;gx_str(at,"ms",40-(int)(at-b));fx_small(12,90,216,b,GX_WHITE);
 gx_num(b,(int)plat_cpu_pct());at=b;while(*at)at++;gx_str(at,"% AUDIO / XRUN ",40-(int)(at-b));while(*at)at++;gx_num(at,(int)plat_xruns());fx_small(12,111,216,b,GX_WHITE);
 gx_num(b,(int)plat_clock_mhz());at=b;while(*at)at++;gx_str(at,"MHz / CORES ",40-(int)(at-b));while(*at)at++;gx_num(at,plat_core_ready()?2:1);fx_small(12,132,216,b,GX_WHITE);
 gx_num(b,(int)en_active_voices);at=b;while(*at)at++;gx_str(at,en_graphics_throttle?" VOICES / GFX ECO":" VOICES / GFX FULL",40-(int)(at-b));fx_small(12,153,216,b,GX_WHITE);
 gx_num(b,(int)(plat_ram_used()/1024));at=b;while(*at)at++;gx_str(at,"KiB / C1 ",40-(int)(at-b));while(*at)at++;gx_num(at,(int)plat_core_stack_used());while(*at)at++;gx_str(at,"B",40-(int)(at-b));fx_small(12,174,216,b,GX_DIM);
 gx_str(b,"C0/C1 ",40);gx_num(b+6,(int)en_core_job_peak_us[0]);at=b;while(*at)at++;*at++='/';gx_num(at,(int)en_core_job_peak_us[1]);at=b;while(*at)at++;gx_str(at,"us",40-(int)(at-b));fx_small(12,195,216,b,GX_TEAL);
 fx_small(12,222,216,"K1 FPS / K3 INFO / K4 TEST",GX_DIM);
}
static void fx_settings(void){int bt=fx_bottom()-17;char b[24];
 if(ui.view==V_INTENT){fx_panel(8,49,224,43,FX_PEARL);fx_text(18,53,203,piece_name(proj.piece),FX_INK,&FONT_M);gx_num(b,(int)HEALING_HZ[proj.par[P_ANCHOR]]);fx_text(15,99,115,b,GX_TEAL,&FONT_L);fx_small(143,109,84,en_scale_name(proj.par[P_TUNING]),GX_DIM);}
 else if(ui.view==V_OUTPUT){fx_panel(8,49,224,44,FX_PEARL);fx_text(18,55,205,proj.par[P_OUTPUT]==2?(en_output_route?"AUTO / SPEAKER":"AUTO / USB AUDIO"):proj.par[P_OUTPUT]?"FM1 SPEAKER":"EXTERNAL / JACK",FX_INK,&FONT_M);fx_body(12,102,219,en_output_route?"SPEAKER / JACK: MANUAL":"FULL-RANGE / JACK: MANUAL",GX_TEAL);char *at=b;*at++='C';*at++=plat_core_ready()?'2':'1';*at++=' ';*at++='J';gx_num(at,(int)(plat_core_jobs()%10000u));while(*at)at++;*at++=' ';*at++='F';gx_num(at,(int)ui_fps);while(*at)at++;*at++=' ';*at++='S';gx_num(at,(int)(en_shed_count%10000u));fx_text(12,122,216,"44.1k / AUTO USB, JACK MANUAL",GX_TEAL,&FONT_XS);at=b;*at++='A';gx_num(at,(int)plat_cpu_pct());while(*at)at++;*at++='%';*at++=' ';*at++='X';gx_num(at,(int)(plat_xruns()%10000u));while(*at)at++;*at++=' ';*at++='M';gx_num(at,(int)plat_clock_mhz());}
 else if(ui.view==V_REST){fx_panel(8,49,224,43,FX_PEARL);fx_text(18,55,206,(const char *const[]){"AMBIENCE","NIGHTFALL","CINEMATIC","NEOCLASSICAL"}[proj.par[P_STYLE]],FX_INK,&FONT_M);if(proj.par[P_STYLE]==EN_CINEMATIC){fx_small(14,101,110,"ARRANGEMENT",GX_DIM);fx_body(144,99,84,(const char *const[]){"Theme","Build","Peak","Release"}[en_cinematic_stage&3],GX_TEAL);}else{fx_small(14,101,110,"CONTINUOUS PLAY",GX_DIM);}if(bt>136)fx_text(14,121,213,(proj.par[P_REFERENCE]?REFERENCE_NAME[proj.par[P_REFERENCE]]:proj.par[P_STYLE]==EN_NEOCLASSICAL?"AUTO / REF9 CHAMBER":REFERENCE_NAME[en_reference()+1]),GX_DIM,&FONT_XS);}
 else {fx_small(12,47,216,THEME_NAME[proj.par[P_THEME]&3],GX_TEAL);fx_panel(8,72,224,43,FX_PEARL);fx_text(18,77,204,(const char *const[]){"Small","Medium","Large"}[proj.par[P_TEXT]],FX_INK,&FONT_M);fx_small(12,121,216,"EIN-1",GX_TEAL);}
}
/* 4.8-second original native intro: drifting stardust, an unfolding halo,
 * luminous petals and a title reveal. The same renderer runs on FM1 and WASM. */
static void gx_intro(float sec){
 resonance_frame();float fade=fm_clampf(sec*.7f,0,1);gx_shade(0,146,240,94,0,12);
 fx_text((240-text_w(&FONT_M,"EIN-1"))/2,158,200,"EIN-1",gx_mix(GX_BG,GX_WHITE,(int)(fade*16)),&FONT_M);
 fx_small(70,188,160,"Created by Ziao",GX_TEAL);fx_small(35,210,205,"THE CELESTIAL AWAKENING",GX_DIM);
}

static void ui_power_reset(void);
void ui_init(void){ui_hub=0;ui_area=ui_topic=ui_edit=ui_track_controls=0;crystal_has_frame=0;ui_power_reset();journey_write=journey_last=0;fx_curve_valid=0;ui_remote_serial=0;memset(ui_remote_rev,0,sizeof ui_remote_rev);celestial_view_yaw=celestial_from_yaw=celestial_to_yaw=.015f;celestial_view_tilt=celestial_from_tilt=celestial_to_tilt=.80f;gx_sky_energy=gx_sky_fog=0;gx_sky_ms=0;celestial_shot_age=30;celestial_camera_ms=0;celestial_hole_seen=0;celestial_grb_seen=en_grb_serial;celestial_nova_seen=en_supernova_serial;celestial_capture_seen=en_captures;ui_intent_pending=0;ui_intent_target=proj.piece;ui_key_octave=0;ui_grb_seen=en_grb_serial;for(int i=0;i<6;i++)gx_shot_seen[i]=0;gx_nova_seen=en_supernova_serial;gx_nova_age=30;gx_grb_cam_age=30;gx_grb_seen=en_grb_serial;for(int l=0;l<8;l++)ui_track_seen[l]=en_track_revision[l];gx_nova_yaw=gx_nova_pitch=0;gx_visual_ms=0;gx_visual_wall=plat_ms();gx_intro_rng=proj.seed^plat_ms()^0xa71ac701u;if(!gx_intro_rng)gx_intro_rng=1;gx_intro_scene_rate=gx_intro_rate();for(int l=0;l<8;l++)gx_intro_object[l]=gx_intro_rate();for(int i=0;i<256;i++)gx_sin[i]=fm_sin_turns((float)i/256);memset(&ui,0,sizeof ui);ui.view=V_OBSERVATORY;ui.sel=(uint8_t)proj.par[P_FOCUS];ui.lane=ui.sel;ui.activity=plat_ms();ui.master_prev=plat_master();boot_ms=plat_ms();}
int ui_dirty(void){return ui.dirty;}
static void gx_music_cues(void){}

#include "power_safety.h"
static power_safety_t battery_policy;
static uint8_t battery_muted,battery_saved,battery_save_failed;
static unsigned battery_countdown;
static void ui_power_reset(void){memset(&battery_policy,0,sizeof battery_policy);battery_muted=battery_saved=battery_save_failed=0;battery_countdown=0;}
static void ui_power_safety(void){
 unsigned state=power_safety_step(&battery_policy,plat_ms(),plat_battery_percent(),plat_usb_power());
 battery_countdown=state;
 if(!state){if(battery_muted){en_mute(0);battery_muted=0;}battery_saved=battery_save_failed=0;}
 if(state>=58&&!battery_muted){en_mute(1);battery_muted=1;}
 if(state==61&&!battery_saved){en_play(0);battery_save_failed=project_save()!=0;battery_saved=1;}

 /* No validated FM1 power-latch/wake routine is available. Keep USB recovery
  * reachable and visibly request the switch; never claim the unit powered off. */
}
static void ui_power_notice(void){
 if(battery_countdown){char text[32];cv_rect(8,91,224,56,GX_BG);
 fx_text(15,95,210,"CRITICAL BATTERY",GX_WHITE,&FONT_S);
 if(battery_countdown&&battery_countdown<61){gx_num(text,61-battery_countdown);int n=0;while(text[n])n++;gx_str(text+n,"s / CONNECT USB",32-n);}
 else gx_str(text,battery_save_failed?"SAVE FAILED / SWITCH OFF":"SAVED / SWITCH OFF",32);
 fx_text(15,119,210,text,GX_WHITE,&FONT_S);}
}

static void ui_present(void){profile_stage(PROF_UI);if(proj.par[P_FPS]){char b[32],*at;gx_num(b,(int)ui_fps);at=b;while(*at)at++;gx_str(at,"/",32-(int)(at-b));while(*at)at++;gx_num(at,(int)ui_frame_ms);while(*at)at++;gx_str(at,"ms",32-(int)(at-b));cv_rect(2,226,78,13,GX_BG);cv_text(4,226,&FONT_XS,b,GX_TEAL);}ui_power_notice();plat_brightness(proj.par[P_POWER]==5);cv_blit(0,0);profile_stage(PROF_FLUSH);plat_control_visible();ui_last_draw_ms=plat_ms()-ui_draw_start;if(ui.view==V_OBSERVATORY){unsigned m=ui.idle?1:0,b=ui_last_draw_ms/8u;if(b>15)b=15;profile_record(m,ui_last_draw_ms);}uint32_t audio_ms=(plat_audio_time_us()-ui_audio_start_us)/1000u;ui_lcd_wait_ms=(plat_lcd_wait_us()-ui_lcd_start_us)/1000u;uint32_t waits=audio_ms+ui_lcd_wait_ms;ui_cpu_draw_ms=ui_last_draw_ms>waits?ui_last_draw_ms-waits:0;if(ui_last_draw_ms>33||plat_cpu_pct()>80){if(gx_quality)gx_quality--;gx_recovery=0;}else if(ui_last_draw_ms<23&&plat_cpu_pct()<60&&++gx_recovery>=120){if(gx_quality<2)gx_quality++;gx_recovery=0;}}
void ui_frame(void){
 /* Audio and input keep their own cadence. Do not redraw a frozen animation
  * at the menu rate; deliberate input can still refresh chrome immediately. */
 ui_input_only();static uint32_t present_wall,present_activity;uint32_t instant=plat_ms();
 uint32_t visual_period=frame_budget_period(&ui_budget,instant,plat_cpu_pct(),plat_xruns(),ui_last_draw_ms,proj.par[P_POWER]);
 if(ui.view==V_OBSERVATORY&&present_wall&&instant-present_wall<visual_period&&ui.activity==present_activity)return;
 present_wall=instant;present_activity=ui.activity;
 fx_history_tick();
 ui_power_safety();
#if EN_BALANCED_GRAPHICS
 gx_budget_begin();
#endif
ui_draw_start=plat_ms();ui_audio_start_us=plat_audio_time_us();ui_lcd_start_us=plat_lcd_wait_us();ui_input_only();uint32_t wall=plat_ms();ui_frame_ms=ui_frame_wall?wall-ui_frame_wall:0;ui_frame_wall=wall;gx_visual_ms+=wall-gx_visual_wall;gx_visual_wall=wall;ui.frame++;ui_fps_frames++;if(wall-ui_fps_start>=1000u){ui_fps=ui_fps_frames*1000u/(wall-ui_fps_start);ui_fps_start=wall;ui_fps_frames=0;}if(en_scene_pending>=8&&en_scene!=proj.par[P_SCENE])proj.par[P_SCENE]=en_scene;cv_setup(240,240);
 if(plat_ms()-boot_ms<ui_splash_ms){ui.activity=plat_ms();ui.idle=0;gx_intro((float)(plat_ms()-boot_ms)*0.001f);ui_present();return;}
 if(en_diagnostic_mode==1)gx_background();else{
 if(!ui_hub&&(ui.view==V_OBSERVATORY||ui.idle))gx_visualizer();else cv_rect(0,0,240,240,0);}profile_begin();
 if(ui.view==V_STRUM){char v[24];en_param_text(P_STRUM_MODE,proj.par[P_STRUM_MODE],v);fx_panel(10,50,220,72,GX_PANEL);fx_text(18,56,204,"STRUM EXPRESSION",GX_DIM,&FONT_XS);fx_text(18,77,204,v,GX_TEAL,&FONT_S);fx_text(18,100,204,"WIND / STRINGS / HARP",GX_DIM,&FONT_XS);}
 if(ui.view==V_MOTIF_MODE){char v[24];en_param_text(P_MEMORY,proj.par[P_MEMORY],v);fx_small(10,49,220,v,GX_TEAL);fx_motif_landscape(65,fx_bottom()-24);}
 if(ui.view==V_EXPRESSION){char v[24];en_param_text(P_MEMORY,proj.par[P_MEMORY],v);fx_panel(10,50,220,40,GX_PANEL);fx_text(18,56,204,"MOTIF MEMORY",GX_DIM,&FONT_XS);fx_text(18,74,204,v,GX_TEAL,&FONT_S);fx_vibrato();}
 if(ui.view==V_PLANETS)fx_worlds();if(!en_playing&&!ui.idle){gx_shade(85,5,70,17,0,13);fx_small(98,8,59,"PAUSED",GX_WHITE);}
 if(ui.idle){ui_present();return;}

 fx_header();
 if(plat_battery_percent()>=0&&plat_battery_percent()<25&&!plat_usb_power())fx_text(62,30,165,"LOW BATTERY / ESTIMATE",GX_WHITE,&FONT_XS);
 switch(ui.view){case V_OBSERVATORY:{fx_text(8,43,224,VOICE_NAME[proj.par[LP(ui.sel,L_VOICE)]],GX_WHITE,&FONT_XS);fx_text(8,57,224,ui_edit?(const char *const[]){"EDIT SOUND / TONE / LEVEL / NOTES","EDIT ENVELOPE / REGISTER","EDIT SENDS / PHRASE"}[ui_track_controls]:"SELECT:TRACK SEL:EDIT ARP:EVOLVE",GX_TEAL,&FONT_XS);break;}case V_TIMING:case V_COMPOSITION:case V_SOUNDSCAPES:{char v[24];int p=gx_param(ui.view,0);en_param_text(p,proj.par[p],v);fx_panel(10,52,220,72,GX_PANEL);fx_text(18,61,204,v,GX_TEAL,&FONT_M);if(ui.view==V_SOUNDSCAPES){char active[24];if(en_soundscape_active>=0)en_param_text(P_SCAPE_BANK,en_soundscape_active,active);else gx_str(active,(const char *const[]){"OFF / FADING","AUTO SELECTING","SOLO: BED MUTED","LEVEL ZERO","SOURCE UNAVAILABLE","WAITING FOR CROSSFADE"}[en_soundscape_reason<6?en_soundscape_reason:1],24);fx_small(14,102,212,active,GX_TEAL);}
 if(ui.view!=V_SOUNDSCAPES){int pending=proj.par[p]!=en_remote_value(p);fx_small(14,102,212,pending?"CHANGE AT NEXT BAR":"ACTIVE",GX_TEAL);}
 fx_small(14,125,212,ui.view==V_TIMING?"SHARED PULSE / ORGANIC GESTURES":ui.view==V_COMPOSITION?"TOGETHER / STRUM / AUTO":"REAL FIELD RECORDINGS",GX_DIM);break;}case V_USB_SCREEN:{
 uint32_t metrics[4];plat_mirror_metrics(metrics);
 fx_small(12,70,216,"USB MIRROR",GX_TEAL);
 fx_small(12,100,216,!plat_mirror_armed?"OFF":metrics[0]?"BROWSER SUBSCRIBED":"WAITING FOR BROWSER",GX_WHITE);
 fx_small(12,130,216,plat_mirror_armed?"SEL: STOP MIRROR":"SEL: START MIRROR",GX_TEAL);
 char queued[16],submitted[16];
 gx_num(queued,metrics[1]);
 gx_num(submitted,metrics[3]);
 fx_small(12,156,110,"QUEUED",GX_DIM);
 fx_small(126,156,102,"USB SUBMITTED",GX_DIM);
 fx_small(12,174,110,queued,GX_WHITE);
 fx_small(126,174,102,submitted,GX_WHITE);
 break;}case V_DRUMPARTS:fx_drumparts();break;case V_MATRIX:fx_ensemble();break;case V_VOICE:fx_voice();break;case V_PLANETS:break;case V_MOTIF:fx_phrase();break;case V_SCENES:fx_journey();break;case V_MIXER:fx_balance();break;case V_MIXAUTO:fx_auto_mix();break;case V_DEBUG:fx_debug();break;case V_DRUMS:case V_BASS:fx_rhythm();break;case V_DRUMFX:fx_small(12,55,216,"PERCUSSION / TRACK 8",GX_TEAL);fx_small(12,85,216,proj.par[P_DRUM_AUTO]?"AUTO KIT / REFERENCE FX":"MANUAL KIT / YOUR FX",GX_DIM);break;case V_DRIFT:case V_SPACE:case V_REVERB_DETAIL:case V_ECHO_DETAIL:fx_flow();break;case V_BLUETOOTH:fx_bluetooth();break;case V_INTENT:case V_OUTPUT:case V_REST:case V_TEXT:fx_settings();break;}
 if(ui.view!=V_USB_SCREEN&&ui.view!=V_DRUMPARTS&&ui.view!=V_MATRIX&&ui.view!=V_MIXER&&ui.view!=V_DEBUG&&ui.view!=V_MIXAUTO){gx_shade(0,fx_bottom()-19,240,259-fx_bottom(),0,10);fx_dock();}
 if(plat_ms()<ui.msg_until){fx_panel(7,45,226,25,FX_PEARL);fx_small(15,51,210,ui.msg,FX_INK);}
 if(ui.view==V_USB_SCREEN)fx_text(4,227,232,"ALGO:PAGE HOME:BACK",GX_TEAL,&FONT_XS);
 else if(ui.view!=V_MIXER&&ui.view!=V_OBSERVATORY)fx_text(4,227,232,ui_edit?"EDIT SELECT:ROLE SEL:DONE HOME:BACK":"ALGO:PAGE SELECT:SECTION SEL:EDIT",GX_TEAL,&FONT_XS);ui_present();
}
