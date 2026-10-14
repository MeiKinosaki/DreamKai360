#include "dreamkai360.h"
#include <assert.h>
#include <stdlib.h>
int main(void){
 uint8_t *ram=(uint8_t*)calloc(1,DK_RAM_SIZE); dk_machine_t m; uint32_t v=0; dk_mmu_t mmu;
 assert(ram); assert(dk_machine_init(&m,ram,DK_RAM_SIZE,0)==0);
 ram[0]=0xe1;ram[1]=0x2a;ram[2]=0x71;ram[3]=0x01;
 assert(dk_sh4_step(&m.cpu,&m.bus)==0&&m.cpu.r[1]==42);
 assert(dk_sh4_step(&m.cpu,&m.bus)==0&&m.cpu.r[1]==43);
 assert(dk_bus_write32(&m.bus,0x100,0x12345678)==0);
 assert(dk_bus_read32(&m.bus,0x100,&v)==0&&v==0x12345678);
 dk_mmu_reset(&mmu);mmu.enabled=1;assert(dk_mmu_map(&mmu,0,0x1000,0x2000,1,1)==0);
 {uint32_t p=0;assert(dk_mmu_translate(&mmu,0x1234,0,1,&p)==0&&p==0x2234);}
 dk_dma_reset(&m.dma);assert(dk_dma_start(&m.dma,0x100,0x200,4)==0);
 assert(dk_dma_step(&m.dma,&m.bus,4)==4);assert(dk_bus_read32(&m.bus,0x200,&v)==0&&v==0x12345678);
 dk_pvr_reset(&m.pvr);assert(dk_pvr_set_mode(&m.pvr,640,480)==0);
 dk_irq_raise(&m.irq,DK_IRQ_VBLANK);assert(dk_irq_active(&m.irq)&DK_IRQ_VBLANK);
 free(ram);return 0;
}
