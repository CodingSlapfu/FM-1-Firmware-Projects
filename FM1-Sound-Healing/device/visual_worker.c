/* SPDX-License-Identifier: MPL-2.0
 * Optional core-1 geometry job. Audio never submits or waits for worker jobs.
 * Inputs are frozen through completion; fallback holds a failed worker before reuse. */
#if defined(PLAT_HOST) && FM1B_CPU2
static int host_worker_on,host_worker_reason;
int plat_cpu2_set(int on){const char *m=getenv("FM1_WORKER_TEST");host_worker_on=on && !(m&&!strcmp(m,"off"));return host_worker_on?0:-1;}
int plat_cpu2_on(void){return host_worker_on;}
int plat_cpu2_run(void(*fn)(uint32_t),uint32_t arg){if(!host_worker_on)return 0;const char*m=getenv("FM1_WORKER_TEST");if(m&&strcmp(m,"ok")){host_worker_reason=!strcmp(m,"bad")?1:!strcmp(m,"late")?2:4;return 1;}fn(arg);return 1;}
int plat_cpu2_wait(void){if(host_worker_reason){host_worker_on=0;return -1;}return 0;}
void plat_cpu2_sync(void){}
int plat_cpu2_busy(void){return 0;}
int plat_cpu2_gave_up(void){return host_worker_reason;}
uint32_t plat_cpu2_bad(void){return host_worker_reason==1;}
int plat_cpu2_barred(void){return host_worker_reason!=0;}
uint32_t plat_cpu2_stack(void){return 0;}
#endif
static struct {float time,meter[8];} vis_input __attribute__((aligned(64)));
static struct {int16_t xy[8][41][2];unsigned busy_us;} vis_output __attribute__((aligned(64)));
static unsigned worker_wait_us,worker_fallbacks;
static void visual_job(uint32_t unused){(void)unused;unsigned begin=plat_ticks24();for(int t=0;t<8;t++){float phase=vis_input.time+t*.785398f;float radius=32.f+t*4.f+vis_input.meter[t]*28.f;for(int j=0;j<=40;j++){float a=j*.1570796f;float petal=radius*(.78f+.22f*fm_sinf(3*a+phase));float z=fm_sinf(a+phase);vis_output.xy[t][j][0]=120+(int)(petal*fm_sinf(a+phase));vis_output.xy[t][j][1]=87+(int)(petal*fm_sinf(a*2+phase)*.45f+z*7);}}vis_output.busy_us=(plat_ticks24()-begin)/24;}
static void prepare_geometry(void){vis_input.time=(float)(plat_ms()%120000)*.00012f;for(int t=0;t<8;t++)vis_input.meter[t]=engine.tracks[t].meter;plat_cpu2_sync();unsigned begin=plat_ticks24();if(plat_cpu2_run(visual_job,0)){if(plat_cpu2_wait()!=0){worker_fallbacks++;visual_job(0);}}else visual_job(0);worker_wait_us=(plat_ticks24()-begin)/24;plat_cpu2_sync();}
