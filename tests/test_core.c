#include "dreamkai360.h"
#include <assert.h>
#include <stdlib.h>
int main(void){
 uint8_t *ram=(uint8_t*)calloc(1,DK_RAM_SIZE); uint8_t disc[4096]={0},sector[2048]; uint32_t *fb=(uint32_t*)calloc(640u*480u,sizeof(uint32_t)); int16_t audio[128]; dk_machine_t m; uint32_t v=0; dk_mmu_t mmu;
 assert(ram&&fb); assert(dk_machine_init(&m,ram,DK_RAM_SIZE,0)==0);
 ram[0]=0xe1;ram[1]=0x2a;ram[2]=0x71;ram[3]=0x01;
 assert(dk_sh4_step(&m.cpu,&m.bus)==0&&m.cpu.r[1]==42);
 assert(dk_sh4_step(&m.cpu,&m.bus)==0&&m.cpu.r[1]==43);
 /* SH-4 register ADD and SUB */
 ram[4]=0x32;ram[5]=0x0c; /* ADD R0,R2 */
 ram[6]=0x32;ram[7]=0x08; /* SUB R0,R2 */
 m.cpu.pc=4;m.cpu.r[2]=7;
 assert(dk_sh4_step(&m.cpu,&m.bus)==0&&m.cpu.r[2]==7);
 assert(dk_sh4_step(&m.cpu,&m.bus)==0&&m.cpu.r[2]==7);
 assert(dk_bus_write32(&m.bus,0x100,0x12345678)==0);
 assert(dk_bus_read32(&m.bus,0x100,&v)==0&&v==0x12345678);
 dk_mmu_reset(&mmu);mmu.enabled=1;assert(dk_mmu_map(&mmu,0,0x1000,0x2000,1,1)==0);
 {uint32_t p=0;assert(dk_mmu_translate(&mmu,0x1234,0,1,&p)==0&&p==0x2234);}
 dk_dma_reset(&m.dma);assert(dk_dma_start(&m.dma,0x100,0x200,4)==0);
 assert(dk_dma_step(&m.dma,&m.bus,4)==4);assert(dk_bus_read32(&m.bus,0x200,&v)==0&&v==0x12345678);
 dk_pvr_reset(&m.pvr);assert(dk_pvr_set_mode(&m.pvr,640,480)==0);
 assert(dk_pvr_attach_framebuffer(&m.pvr,fb,640u*480u)==0);
 assert(dk_pvr_clear(&m.pvr,0xff000000u)==0);
 assert(dk_pvr_fill_rect(&m.pvr,10,10,4,4,0xffffffffu)==0);
 assert(fb[10u*640u+10u]==0xffffffffu&&fb[0]==0xff000000u);
 dk_pvr_present(&m.pvr);assert(m.pvr.frames==1);
 dk_irq_raise(&m.irq,DK_IRQ_VBLANK);assert(dk_irq_active(&m.irq)&DK_IRQ_VBLANK);
 free(ram);return 0;
}
