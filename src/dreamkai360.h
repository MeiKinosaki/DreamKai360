#ifndef DREAMKAI360_CORE_H
#define DREAMKAI360_CORE_H
#include <stddef.h>
#include <stdint.h>

#define DK_RAM_SIZE (16u * 1024u * 1024u)
#define DK_TLB_ENTRIES 64u
#define DK_PAGE_SIZE 4096u
#define DK_IRQ_DMA 0x00000001u
#define DK_IRQ_VBLANK 0x00000002u

typedef struct { uint32_t vpn, ppn; uint8_t valid, write, execute, asid; } dk_tlb_entry_t;
typedef struct { dk_tlb_entry_t e[DK_TLB_ENTRIES]; uint8_t asid, enabled; } dk_mmu_t;
typedef struct {
  uint32_t r[16], pc, pr, sr, gbr, vbr, mach, macl;
  uint64_t cycles; uint32_t branch_target; uint8_t branch_pending, halted;
} dk_sh4_t;
typedef struct {
  uint8_t *ram; size_t ram_size; dk_mmu_t *mmu;
  uint32_t pvr[256], aica[2048], gdrom[64], maple[64];
  uint64_t reads, writes, fetches, faults;
} dk_bus_t;
typedef struct { uint32_t src,dst,length,done; uint8_t active,error; } dk_dma_t;
typedef struct { uint32_t pending,mask; uint64_t raised; } dk_irq_t;
typedef struct { uint64_t now,deadline; uint8_t armed; } dk_scheduler_t;
typedef struct { uint32_t width,height,frames; uint8_t enabled,vblank; uint32_t *framebuffer; size_t capacity; } dk_pvr2_t;
typedef struct { uint32_t sample_rate,samples; uint8_t enabled; int16_t mix[2048]; uint32_t phase; } dk_aica_t;
typedef struct { uint32_t lba,sectors; uint8_t ready,reading; const uint8_t *image; size_t image_size; } dk_gdrom_t;
typedef struct { uint16_t buttons; int8_t lx,ly,rx,ry; uint8_t connected; } dk_maple_t;
typedef struct { dk_mmu_t mmu; dk_bus_t bus; dk_sh4_t cpu; dk_dma_t dma; dk_irq_t irq; dk_scheduler_t scheduler; dk_pvr2_t pvr; dk_aica_t aica; dk_gdrom_t gdrom; dk_maple_t pad; uint8_t *ram; } dk_machine_t;

void dk_mmu_reset(dk_mmu_t *m);
int dk_mmu_map(dk_mmu_t *m,unsigned slot,uint32_t va,uint32_t pa,int wr,int ex);
int dk_mmu_translate(const dk_mmu_t *m,uint32_t va,int wr,int ex,uint32_t *pa);
void dk_bus_init(dk_bus_t *b,uint8_t *ram,size_t size,dk_mmu_t *mmu);
int dk_bus_read8(dk_bus_t *b,uint32_t a,uint8_t *v);
int dk_bus_write8(dk_bus_t *b,uint32_t a,uint8_t v);
int dk_bus_read16(dk_bus_t *b,uint32_t a,uint16_t *v);
int dk_bus_write16(dk_bus_t *b,uint32_t a,uint16_t v);
int dk_bus_read32(dk_bus_t *b,uint32_t a,uint32_t *v);
int dk_bus_write32(dk_bus_t *b,uint32_t a,uint32_t v);
void dk_sh4_reset(dk_sh4_t *c,uint32_t pc);
int dk_sh4_step(dk_sh4_t *c,dk_bus_t *b);
void dk_dma_reset(dk_dma_t *d);
int dk_dma_start(dk_dma_t *d,uint32_t src,uint32_t dst,uint32_t len);
uint32_t dk_dma_step(dk_dma_t *d,dk_bus_t *b,uint32_t budget);
void dk_irq_raise(dk_irq_t *i,uint32_t bits);
void dk_irq_ack(dk_irq_t *i,uint32_t bits);
uint32_t dk_irq_active(const dk_irq_t *i);
void dk_scheduler_advance(dk_scheduler_t *s,uint64_t cycles);
void dk_scheduler_schedule(dk_scheduler_t *s,uint64_t delay);
int dk_scheduler_due(const dk_scheduler_t *s);
void dk_pvr_reset(dk_pvr2_t *p);
int dk_pvr_attach_framebuffer(dk_pvr2_t *p,uint32_t *pixels,size_t capacity);
int dk_pvr_clear(dk_pvr2_t *p,uint32_t rgba);
int dk_pvr_fill_rect(dk_pvr2_t *p,int x,int y,int w,int h,uint32_t rgba);
void dk_pvr_present(dk_pvr2_t *p);
int dk_pvr_set_mode(dk_pvr2_t *p,uint32_t w,uint32_t h);
void dk_aica_reset(dk_aica_t *a);
size_t dk_aica_mix_tone(dk_aica_t *a,uint32_t frequency,int16_t amplitude,size_t count,int16_t *out);
void dk_gdrom_reset(dk_gdrom_t *g);
int dk_gdrom_mount(dk_gdrom_t *g,const uint8_t *image,size_t bytes);
int dk_gdrom_read_sector(dk_gdrom_t *g,uint32_t lba,uint8_t out[2048]);
void dk_maple_reset(dk_maple_t *p);
void dk_maple_set_state(dk_maple_t *p,uint16_t buttons,int8_t lx,int8_t ly,int8_t rx,int8_t ry);
int dk_machine_init(dk_machine_t *m,uint8_t *ram,size_t size,uint32_t entry);
int dk_machine_step(dk_machine_t *m,uint32_t dma_budget,uint32_t cycles);
#endif
