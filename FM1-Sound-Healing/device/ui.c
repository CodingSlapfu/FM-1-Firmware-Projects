/* SPDX-License-Identifier: MPL-2.0 */
static const char *pages[]={"PLAY","SOUND","EVOLVE","MIX"};
static const char *beds[]={"Off","Wind","Water","Rain","Forest","Ocean","Birds","Leaves"};
static const char *groups[]={"Instrument","Nature","Frequency","Binaural","Percussion"};
static void num(char *s,int v){char t[12];int n=0;unsigned u=v<0?-v:v;if(v<0)*s++='-';do{t[n++]=(char)('0'+u%10);u/=10;}while(u);while(n)*s++=t[--n];*s=0;}
static void label(int x,int y,const char *name,int value){char v[12];num(v,value);cv_text(x,y,&FONT_XS,name,RGB(126,166,164));cv_text(x,y+15,&FONT_B,v,RGB(218,240,220));}
static void geometry(int offset){for(int t=0;t<8;t++){float meter=vis_input.meter[t];uint16_t c=engine.tracks[t].mute?RGB(25,45,49):RGB(65+(int)(meter*150),105+t*12,130+t*10);for(int j=1;j<=40;j++)cv_line(vis_output.xy[t][j-1][0],vis_output.xy[t][j-1][1]-offset,vis_output.xy[t][j][0],vis_output.xy[t][j][1]-offset,c);}}

static void draw(void){unsigned revision=state_revision;int first=dirty?0:1,last=dirty?5:3;if(debug){first=0;last=5;}
 for(int band=first;band<last;band++){cv_begin(240,48,RGB(5,17,22));
 if(debug){if(band==0)cv_text(10,8,&FONT_B,"PERFORMANCE " APP_VERSION,C_WHITE);if(band==1){label(10,0,"PEAK us",audio_max);label(130,0,"MEAN us",mean_us);}if(band==2){label(10,0,"P95 us",p95);label(130,0,"P99 us",p99);}if(band==3){label(10,0,"DMA XRUN",plat_xruns());label(130,0,"CPU %",plat_cpu_pct());}if(band==4){cv_text(10,0,&FONT_XS,plat_cpu2_on()?"2 CORES / AUDIO ON CORE 0":"1 CORE / SAFE FALLBACK",C_WHITE);label(10,15,"QUEUE DROPS",dropped);}}
 else{
 if(band==0){cv_text(10,3,&FONT_B,pages[page],RGB(174,236,208));cv_text(100,5,&FONT_XS,engine.running?"PLAYING":"STOPPED",C_WHITE);cv_text(10,25,&FONT_XS,page==1?groups[group]:sr_scenes[engine.scene].name,RGB(126,166,164));}
 if(band==1||band==2){geometry(band*48);if(band==2){char n[12];num(n,selected+1);cv_text(8,32,&FONT_XS,n,RGB(174,236,208));cv_text(25,32,&FONT_XS,sr_patches[engine.tracks[selected].patch].name,engine.tracks[selected].mute?RGB(80,100,100):C_WHITE);}}
 if(band==3){
 if(page==0){label(8,0,"K1 SCENE",engine.scene+1);label(126,0,"K2 ENERGY %",(int)(engine.energy*100));}
 if(page==1){switch(group){case 0:label(8,0,"K1 PATCH",engine.tracks[selected].patch+1);label(126,0,"K2 LEVEL %",engine.tracks[selected].volume*100);break;case 1:cv_text(8,0,&FONT_XS,"K1 NATURE",C_WHITE);cv_text(8,16,&FONT_B,beds[engine.bed_type],C_WHITE);label(126,0,"K2 LEVEL %",engine.bed_level*100);break;case 2:label(8,0,"K1 TONE Hz",sr_frequencies[engine.frequency].hz);label(126,0,"K2 LEVEL %",engine.tone_level*100);break;case 3:label(8,0,"K1 CARRIER Hz",engine.binaural_carrier);label(126,0,"K2 BEAT x10",engine.binaural_rate*10);break;case 4:label(8,0,"K1 KIT",engine.drum_kit+1);label(126,0,"K2 LEVEL %",engine.drum_level*100);break;}}
 if(page==2){label(8,0,"K1 VARIATION",engine.variation*100);label(126,0,"K2 FREEZE",engine.freeze);}
 if(page==3){label(8,0,"K1 LEVEL %",engine.tracks[selected].volume*100);label(126,0,"K2 PAN %",engine.tracks[selected].pan*100);}}
 if(band==4){const char *hint=page==0?"K3 TONE  K4 NATURE":page==2?"K3 TEMPO  K4 ENERGY":page==3?"K3 REVERB  K4 DELAY":group==0?"K3 PAN  K4 REVERB":group==1?"K3 REVERB  K4 DELAY":group==2?"K3 432/440  K4 ENERGY":group==3?"K3 LEVEL  K4 ON/OFF":"K3 DENSITY  K4 ON/OFF";cv_text(8,0,&FONT_XS,hint,C_WHITE);cv_text(8,15,&FONT_XS,"ALGO PAGE  SELECT TRACK",RGB(126,166,164));cv_text(8,30,&FONT_XS,engine.binaural_enabled?"HEADPHONES / STEREO":"EDIT GROUP  ENV 432/440",RGB(174,236,208));}}
 cv_blit(0,band*48);}dirty=0;displayed_revision=revision;}
