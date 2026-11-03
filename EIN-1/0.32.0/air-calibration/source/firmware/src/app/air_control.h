/* SPDX-License-Identifier: GPL-3.0-only. Passive aggregate RF control, no identities.
 * UI/main loop only. Sample median, calibration and EMA at10Hz, neutral on loss.
 * Destination0 room send,1 echo send,2 tone. Manual values are never changed.
 */
#pragma once
#include <stdint.h>
typedef struct {int16_t far_rssi,near_rssi;uint8_t sensitivity,smoothing,depth,invert,track,destination,calibrated,reserved;} air_preferences_t;
static air_preferences_t air_prefs={-55,-75,50,65,35,0,2,0,0,0};
static uint32_t air_enabled_ms;
static uint8_t air_enabled,air_status; /*0 OFF,1 SCANNING,2 SIGNAL,3 NO SIGNAL,4 ERROR */
static int16_t air_raw=-127;static uint16_t air_value;static uint32_t air_tick_ms;
static void air_control_enable(int on){
 if((on!=0)==air_enabled){
  if(!on)air_status=0;
  return;
 }
 if(!plat_air_mode(on)){
  if(on)plat_air_mode(0);
  air_enabled=0;
  air_status=4;
 }else{
  air_enabled=(uint8_t)(on!=0);
  air_status=on?1:0;
 }
 air_tick_ms=0;
 air_enabled_ms=0;
}
static int air_calibrate(int near){if(!air_enabled||air_status!=2)return 0;if(near){if(!(air_prefs.calibrated&1))return 0;if(air_raw-air_prefs.far_rssi<4&&air_prefs.far_rssi-air_raw<4)return 0;air_prefs.near_rssi=air_raw;air_prefs.calibrated|=2;}else{air_prefs.far_rssi=air_raw;air_prefs.calibrated=1;}return 1;}
static void air_control_tick(uint32_t now){
 if(!air_enabled&&!air_value)return;
 if(air_enabled&&!air_enabled_ms)air_enabled_ms=now;
 if(air_tick_ms&&(uint32_t)(now-air_tick_ms)<60)return;air_tick_ms=now;
 int raw=-127;uint32_t stamp=0;int ok=air_enabled?plat_air_signal(&raw,&stamp):0;air_raw=(int16_t)raw;
 int fresh=ok>0&&stamp&&(uint32_t)(now-stamp)<1500u;
 if(air_enabled){if(ok<0){plat_air_mode(0);air_enabled=0;air_status=4;}else air_status=fresh?2:stamp||(uint32_t)(now-air_enabled_ms)>=1500u?3:1;}
 int target=0,span=air_prefs.near_rssi-air_prefs.far_rssi;
 if(fresh&&air_prefs.calibrated==3&&(span>=4||span<=-4)){
  target=(raw-air_prefs.far_rssi)*32767/span;
  target=target*(50+(int)air_prefs.sensitivity)/100;
  if(target<0)target=0;if(target>32767)target=32767;if(air_prefs.invert)target=32767-target;
 }
 /* Hysteresis removes RF chatter; bounded exponential attack/release. */
 if(target-(int)air_value>300||(int)air_value-target>300||!target){int divisor=target>(int)air_value?2+air_prefs.smoothing/25:3+air_prefs.smoothing/15;int step=(target-air_value)/divisor;if(!step&&target!=air_value)step=target>air_value?1:-1;air_value=(uint16_t)((int)air_value+step);if(!target&&air_value<16)air_value=0;}
 en_air_control(air_prefs.track,air_prefs.track==7&&air_prefs.destination==3?2:air_prefs.destination,air_prefs.track==7&&air_prefs.destination==3?0:(unsigned)air_value*air_prefs.depth/100u);
}
