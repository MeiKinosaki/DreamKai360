#include "dreamkai360.h"
#include <assert.h>
#include <stdlib.h>
int main(void){
 uint8_t *ram=(uint8_t*)calloc(1,DK_RAM_SIZE); uint8_t disc[4096]={0},sector[2048]; uint32_t *fb=(uint32_t*)calloc(640u*480u,sizeof(uint32_t)); dk_machine_t m; uint32_t v=0; dk_mmu_t mmu;
 assert(ram&&fb); assert(dk_machine_init(&m,ram,DK_RAM_SIZE,0)==0);
 ram[0]=0xe1;ram[1]=0x2a;ram[2]=0x71;ram[3]=0x01;
 assert(dk_sh4_step(&m.cpu,&m.bus)==0&&m.cpu.r[1]==42);
 assert(dk_sh4_step(&m.cpu,&m.bus)==0&&m.cpu.r[1]==43);
 /* SH-4 register ADD and SUB */
 ram[4]=0x32;ram[5]=0x3c; /* ADD R3,R2 */
 ram[6]=0x32;ram[7]=0x38; /* SUB R3,R2 */
 m.cpu.pc=4;m.cpu.r[2]=7;m.cpu.r[3]=4;
 assert(dk_sh4_step(&m.cpu,&m.bus)==0&&m.cpu.r[2]==11);
 assert(dk_sh4_step(&m.cpu,&m.bus)==0&&m.cpu.r[2]==7);
 assert(dk_bus_write32(&m.bus,0x100,0x12345678)==0);
 assert(dk_bus_read32(&m.bus,0x100,&v)==0&&v==0x12345678);
 assert(dk_bus_read32(&m.bus,0x101,&v)!=0 && dk_bus_write16(&m.bus,0x101,0xabcd)!=0);
 /* AICA MMIO must win over the 16 MiB test RAM range at the same address. */
 { uint8_t got=0; ram[0x00700000u]=0x11;
   assert(dk_bus_write8(&m.bus,0x00700000u,0xa5)==0);
   assert(((uint8_t*)m.bus.aica)[0]==0xa5 && ram[0x00700000u]==0x11);
   assert(dk_bus_read8(&m.bus,0x00700000u,&got)==0 && got==0xa5);
 }
 /* PVR register-window reads and writes are backed by the bus register bank. */
 { uint8_t got=0; assert(dk_bus_write8(&m.bus,0x11000000u,0x5a)==0);
   assert(dk_bus_read8(&m.bus,0x11000000u,&got)==0 && got==0x5a);
 }
 dk_mmu_reset(&mmu);mmu.enabled=1;assert(dk_mmu_map(&mmu,0,0x1000,0x2000,1,1)==0);
 {uint32_t p=0;
   assert(dk_mmu_translate(&mmu,0x1234,0,1,&p)==0&&p==0x2234);
   assert(dk_mmu_translate(&mmu,0x1234,1,0,&p)==0&&p==0x2234);
   assert(dk_mmu_map(&mmu,1,0x3000,0x4000,0,1)==0);
   assert(dk_mmu_translate(&mmu,0x3000,1,0,&p)!=0); /* read-only page rejects writes */
   assert(dk_mmu_translate(&mmu,0x3000,0,1,&p)==0&&p==0x4000);
   assert(dk_mmu_map(&mmu,2,0x6000,0x7000,1,0)==0);
   assert(dk_mmu_translate(&mmu,0x6000,0,1,&p)!=0); /* non-executable page faults */
   mmu.asid=1;
   assert(dk_mmu_translate(&mmu,0x1234,0,0,&p)!=0); /* ASID mismatch */
   mmu.asid=0;
   assert(dk_mmu_translate(&mmu,0x1234,0,0,&p)==0&&p==0x2234);
   assert(dk_mmu_translate(&mmu,0x5000,0,0,&p)!=0); /* unmapped page faults */
 }
 /* Verify that the CPU bus actually uses MMU translations and permissions. */
 { uint8_t got=0; uint64_t old_faults=m.bus.faults;
   assert(dk_mmu_map(&mmu,3,0x1000,0x2000,1,1)==0);
   m.bus.mmu=&mmu; ram[0x2000]=0x6a;
   assert(dk_bus_read8(&m.bus,0x1000,&got)==0 && got==0x6a);
   assert(dk_bus_write8(&m.bus,0x1001,0x5c)==0 && ram[0x2001]==0x5c);
   assert(dk_mmu_map(&mmu,4,0x3000,0x4000,0,1)==0);
   assert(dk_bus_write8(&m.bus,0x3000,0x77)!=0);
   assert(m.bus.faults==old_faults+1 && ram[0x4000]==0);
   assert(dk_bus_read8(&m.bus,0x3000,&got)==0);
   /* Instruction fetches must honor execute permission, unlike data reads. */
   ram[0x2000]=0x00; ram[0x2001]=0x09; m.cpu.pc=0x1000;
   m.cpu.halted=0; m.cpu.branch_pending=0;
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.pc==0x1002);
   assert(dk_mmu_map(&mmu,5,0x5000,0x6000,1,0)==0);
   ram[0x6000]=0x00; ram[0x6001]=0x09; m.cpu.pc=0x5000;
   m.cpu.halted=0; m.cpu.branch_pending=0; old_faults=m.bus.faults;
   assert(dk_sh4_step(&m.cpu,&m.bus)!=0 && m.cpu.halted);
   assert(m.bus.faults==old_faults+1);
   m.bus.mmu=&m.mmu;
 }
 dk_dma_reset(&m.dma);assert(dk_dma_start(&m.dma,0x100,0x200,4)==0);
 assert(dk_dma_step(&m.dma,&m.bus,0)==0 && m.dma.active && m.dma.done==0);
 assert(dk_dma_step(&m.dma,&m.bus,4)==4);assert(dk_bus_read32(&m.bus,0x200,&v)==0&&v==0x12345678);
 /* DMA address wrap must fail closed instead of wrapping after progress. */
 dk_dma_reset(&m.dma);assert(dk_dma_start(&m.dma,0x100,0x300,2)==0);
 assert(dk_dma_step(&m.dma,&m.bus,1)==1 && m.dma.done==1);
 m.dma.src=UINT32_MAX;
 assert(dk_dma_step(&m.dma,&m.bus,1)==0);
 assert(m.dma.error && !m.dma.active && m.dma.done==1);
 dk_pvr_reset(&m.pvr);assert(dk_pvr_set_mode(&m.pvr,640,480)==0);
 assert(dk_pvr_set_mode(&m.pvr,0,480)!=0 && dk_pvr_set_mode(&m.pvr,2048,480)!=0);
 assert(dk_pvr_attach_framebuffer(&m.pvr,fb,1u)!=0);
 assert(dk_pvr_attach_framebuffer(&m.pvr,fb,640u*480u)==0);
 assert(dk_pvr_clear(&m.pvr,0xff000000u)==0);
 assert(dk_pvr_fill_rect(&m.pvr,10,10,4,4,0xffffffffu)==0);
 assert(dk_pvr_fill_rect(&m.pvr,-3,-2,5,4,0xff00ff00u)==0);
 assert(dk_pvr_fill_rect(&m.pvr,2147483640,0,100,1,0xffffffffu)==0);
 assert(fb[0]==0xff00ff00u && fb[1u*640u+1u]==0xff00ff00u);
 assert(fb[10u*640u+10u]==0xffffffffu);
 dk_pvr_present(&m.pvr);assert(m.pvr.frames==1);
 dk_irq_raise(&m.irq,DK_IRQ_VBLANK);assert(dk_irq_active(&m.irq)&DK_IRQ_VBLANK);
 m.irq.mask=DK_IRQ_VBLANK;assert(!(dk_irq_active(&m.irq)&DK_IRQ_VBLANK));
 m.irq.mask=0;assert(dk_irq_active(&m.irq)&DK_IRQ_VBLANK);
 dk_irq_ack(&m.irq,DK_IRQ_VBLANK);assert(!(dk_irq_active(&m.irq)&DK_IRQ_VBLANK));
 dk_scheduler_schedule(&m.scheduler,5);assert(!dk_scheduler_due(&m.scheduler));
 dk_scheduler_advance(&m.scheduler,4);assert(!dk_scheduler_due(&m.scheduler));
 dk_scheduler_advance(&m.scheduler,1);assert(dk_scheduler_due(&m.scheduler));
 /* Deadline arithmetic saturates instead of wrapping to an already-due time. */
 { dk_scheduler_t s={UINT64_MAX-2u,0,0}; dk_scheduler_schedule(&s,10u);
   assert(s.deadline==UINT64_MAX && !dk_scheduler_due(&s));
   dk_scheduler_advance(&s,10u);
   assert(s.now==UINT64_MAX && dk_scheduler_due(&s));
 }
 /* AICA: phase accumulator remains bounded even above sample rate. */
 { dk_aica_t a; int16_t tone[4]; dk_aica_reset(&a);
   assert(dk_aica_mix_tone(&a,1000000u,12000,4,tone)==4);
   assert(a.samples==4 && a.phase<a.sample_rate);
   a.phase=0;
   assert(dk_aica_mix_tone(&a,22049u,INT16_MIN,2,tone)==2);
   assert(tone[0]==INT16_MIN && tone[1]==INT16_MAX);
 }
 /* SH-4 T-bit comparisons and pre-decrement/post-increment memory ops. */
 { uint32_t word=0; m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x400;
   m.cpu.r[2]=5; m.cpu.r[3]=5; ram[0x400]=0x32; ram[0x401]=0x30; /* CMP/EQ R3,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && (m.cpu.sr&1u));
   m.cpu.r[2]=0x500; m.cpu.r[3]=0x12345678;
   ram[0x402]=0x22; ram[0x403]=0x36; /* MOV.L R3,@-R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x4fc);
   assert(dk_bus_read32(&m.bus,0x4fc,&word)==0 && word==0x12345678);
   m.cpu.r[3]=0x4fc; ram[0x404]=0x62; ram[0x405]=0x36; /* MOV.L @R3+,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x12345678 && m.cpu.r[3]==0x500);
   /* Signed byte/word loads and byte/word stores. */
   m.cpu.r[2]=0x520; m.cpu.r[3]=0x80;
   ram[0x406]=0x22; ram[0x407]=0x30; /* MOV.B R3,@R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && ram[0x520]==0x80);
   m.cpu.r[3]=0x520; ram[0x408]=0x62; ram[0x409]=0x30; /* MOV.B @R3,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xffffff80u);
   m.cpu.r[2]=0x522; m.cpu.r[3]=0x8001;
   ram[0x40a]=0x22; ram[0x40b]=0x31; /* MOV.W R3,@R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && ram[0x522]==0x80 && ram[0x523]==0x01);
   m.cpu.r[3]=0x522; ram[0x40c]=0x62; ram[0x40d]=0x31; /* MOV.W @R3,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xffff8001u);
 }
 /* SH-4 unsigned/signed compares, logic, NOT/NEG and shifts. */
 { m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x440;
   m.cpu.r[2]=7; m.cpu.r[3]=5;
   ram[0x440]=0x32; ram[0x441]=0x32; /* CMP/HS R3,R2 (opcode 0x3232) */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && (m.cpu.sr&1u));
   ram[0x442]=0x32; ram[0x443]=0x37; /* CMP/GT R3,R2 (opcode 0x3237) */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && (m.cpu.sr&1u));
   m.cpu.r[2]=0xf0; m.cpu.r[3]=0x0f;
   ram[0x444]=0x22; ram[0x445]=0x39; /* AND R3,R2 (opcode 0x2239) */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x00);
   m.cpu.r[2]=0xf0; ram[0x446]=0x22; ram[0x447]=0x3b; /* OR R3,R2 (opcode 0x223b) */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xff);
   ram[0x448]=0x62; ram[0x449]=0x27; /* NOT R2,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==~0xffu);
   m.cpu.r[2]=8; ram[0x44a]=0x42; ram[0x44b]=0x00; /* SHLL R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==16);
   ram[0x44c]=0x42; ram[0x44d]=0x01; /* SHLR R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==8);
 }
 /* SH-4 immediate arithmetic, TST and DT regression cases. */
 { m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x4c0;
   ram[0x4c0]=0xe2; ram[0x4c1]=0xff; /* MOV #-1,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xffffffffu);
   ram[0x4c2]=0x72; ram[0x4c3]=0xff; /* ADD #-1,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xfffffffeu);
   m.cpu.r[2]=0xf0u; m.cpu.r[3]=0x0fu; ram[0x4c4]=0x22; ram[0x4c5]=0x38; /* TST R3,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && (m.cpu.sr&1u));
   m.cpu.r[3]=0xf0u; ram[0x4c6]=0x22; ram[0x4c7]=0x38;
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && !(m.cpu.sr&1u));
   m.cpu.r[2]=1u; ram[0x4c8]=0x42; ram[0x4c9]=0x10; /* DT R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0u && (m.cpu.sr&1u));
   m.cpu.r[2]=2u; ram[0x4ca]=0x42; ram[0x4cb]=0x10;
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==1u && !(m.cpu.sr&1u));
 }
 /* SH-4 arithmetic shifts preserve sign and report the shifted bit. */
 { m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x4b0;
   m.cpu.r[2]=0x80000001u; ram[0x4b0]=0x42; ram[0x4b1]=0x21; /* SHAR R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xc0000000u && (m.cpu.sr&1u));
   m.cpu.r[2]=0x40000000u; ram[0x4b2]=0x42; ram[0x4b3]=0x20; /* SHAL R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x80000000u && !(m.cpu.sr&1u));
 }
 /* SH-4 single-bit shifts copy the outgoing bit into T. */
 { m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x4a0;
   m.cpu.r[2]=0x80000000u; ram[0x4a0]=0x42; ram[0x4a1]=0x00; /* SHLL R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0u && (m.cpu.sr&1u));
   m.cpu.r[2]=1u; ram[0x4a2]=0x42; ram[0x4a3]=0x01; /* SHLR R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0u && (m.cpu.sr&1u));
 }
 /* SH-4 data transforms: SWAP, zero/sign extension and XTRCT. */
 { m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x460;
   m.cpu.r[2]=0x12345678u; m.cpu.r[3]=0x89abcdefu;
   ram[0x460]=0x62; ram[0x461]=0x28; /* SWAP.B R2,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x12347856u);
   ram[0x462]=0x62; ram[0x463]=0x29; /* SWAP.W R2,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x78561234u);
   m.cpu.r[3]=0x12345680u; ram[0x464]=0x62; ram[0x465]=0x3c; /* EXTU.B R3,R2 (0x623c) */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x80u);
   ram[0x466]=0x62; ram[0x467]=0x3e; /* EXTS.B R3,R2 (0x623e) */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xffffff80u);
   m.cpu.r[3]=0x12348001u; ram[0x468]=0x62; ram[0x469]=0x3d; /* EXTU.W R3,R2 (0x623d) */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x8001u);
   ram[0x46a]=0x62; ram[0x46b]=0x3f; /* EXTS.W R3,R2 (0x623f) */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xffff8001u);
   m.cpu.r[2]=0x11223344u; m.cpu.r[3]=0xaabbccddu;
   ram[0x46c]=0x22; ram[0x46d]=0x3d; /* XTRCT R3,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xccdd1122u);
 }
 /* SH-4 MOVT and additional rotate/shift variants. */
 { m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x480; m.cpu.sr|=1u;
   m.cpu.r[2]=0x12345678u;
   ram[0x480]=0x02; ram[0x481]=0x29; /* MOVT R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==1u);
   m.cpu.r[2]=0x80000001u; ram[0x482]=0x42; ram[0x483]=0x04; /* ROTL R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x00000003u && (m.cpu.sr&1u));
   m.cpu.r[2]=0x80000001u; ram[0x484]=0x42; ram[0x485]=0x05; /* ROTR R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xc0000000u && (m.cpu.sr&1u));
   m.cpu.r[2]=3; ram[0x486]=0x42; ram[0x487]=0x08; /* SHLL2 R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==12u);
   ram[0x488]=0x42; ram[0x489]=0x09; /* SHLR2 R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==3u);
   m.cpu.r[2]=0x12345678u; ram[0x48a]=0x42; ram[0x48b]=0x18; /* SHLL8 R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x34567800u);
   ram[0x48c]=0x42; ram[0x48d]=0x19; /* SHLR8 R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x00345678u);
 }
 /* SH-4 signed positive/non-negative comparisons. */
 { m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x5c0;
   m.cpu.r[2]=0xffffffffu; ram[0x5c0]=0x42; ram[0x5c1]=0x11; /* CMP/PZ R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && !(m.cpu.sr&1u));
   m.cpu.r[2]=0; ram[0x5c2]=0x42; ram[0x5c3]=0x11;
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && (m.cpu.sr&1u));
   m.cpu.r[2]=1; ram[0x5c4]=0x42; ram[0x5c5]=0x15; /* CMP/PL R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && (m.cpu.sr&1u));
   m.cpu.r[2]=0; ram[0x5c6]=0x42; ram[0x5c7]=0x15;
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && !(m.cpu.sr&1u));
 }
 /* SH-4 NEGC, DIV0U and DIV0S flag setup. */
 { m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x5c0;
   m.cpu.r[2]=1; m.cpu.sr&=~1u;
   ram[0x5c0]=0x62; ram[0x5c1]=0x2a; /* NEGC R2,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xffffffffu && (m.cpu.sr&1u));
   m.cpu.r[2]=1; m.cpu.sr|=0x301u;
   ram[0x5c2]=0x00; ram[0x5c3]=0x19; /* DIV0U */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && (m.cpu.sr&0x301u)==0u);
   m.cpu.r[2]=0x80000000u; m.cpu.r[3]=1u;
   ram[0x5c4]=0x23; ram[0x5c5]=0x27; /* DIV0S R2,R3: M=1, Q=0, T=1 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && (m.cpu.sr&0x300u)==0x200u && (m.cpu.sr&1u));
   m.cpu.mach=0x12345678u; m.cpu.macl=0xabcdef01u;
   ram[0x5c6]=0x00; ram[0x5c7]=0x28; /* CLRMAC */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.mach==0u && m.cpu.macl==0u);
   m.cpu.r[4]=0x13579bdfu; ram[0x5c8]=0x44; ram[0x5c9]=0x0a; /* LDS R4,MACH */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.mach==0x13579bdfu);
   ram[0x5ca]=0x44; ram[0x5cb]=0x1a; /* LDS R4,MACL */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.macl==0x13579bdfu);
   ram[0x5cc]=0x44; ram[0x5cd]=0x2a; /* LDS R4,PR */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.pr==0x13579bdfu);
   m.cpu.mach=0x2468ace0u; m.cpu.macl=0x0badf00du; m.cpu.pr=0xfeed1234u;
   ram[0x5ce]=0x04; ram[0x5cf]=0x0a; /* STS MACH,R4 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[4]==0x2468ace0u);
   ram[0x5d0]=0x04; ram[0x5d1]=0x1a; /* STS MACL,R4 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[4]==0x0badf00du);
   ram[0x5d2]=0x04; ram[0x5d3]=0x2a; /* STS PR,R4 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[4]==0xfeed1234u);
   /* SH-4 STS.L/LDS.L special registers through @-Rn/@Rn+. */
   m.cpu.r[5]=0x380u; m.cpu.mach=0x10203040u;
   ram[0x5d4]=0x45; ram[0x5d5]=0x02; /* STS.L MACH,@-R5 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[5]==0x37cu);
   { uint32_t v=0; assert(dk_bus_read32(&m.bus,0x37cu,&v)==0 && v==0x10203040u); }
   m.cpu.r[5]=0x37cu; m.cpu.mach=0u;
   ram[0x5d6]=0x45; ram[0x5d7]=0x06; /* LDS.L @R5+,MACH */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.mach==0x10203040u && m.cpu.r[5]==0x380u);
 }
 /* SH-4 carry/borrow and signed-overflow arithmetic regression tests. */
 { m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x5d0;
   m.cpu.r[2]=0xffffffffu; m.cpu.r[3]=0u; m.cpu.sr|=1u;
   ram[0x5d0]=0x32; ram[0x5d1]=0x3e; /* ADDC R3,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0u && (m.cpu.sr&1u));
   m.cpu.r[2]=0u; m.cpu.r[3]=0u; m.cpu.sr|=1u;
   ram[0x5d2]=0x32; ram[0x5d3]=0x3a; /* SUBC R3,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xffffffffu && (m.cpu.sr&1u));
   m.cpu.r[2]=0x7fffffffu; m.cpu.r[3]=1u;
   ram[0x5d4]=0x32; ram[0x5d5]=0x3f; /* ADDV R3,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x80000000u && (m.cpu.sr&1u));
   m.cpu.r[2]=0x80000000u; m.cpu.r[3]=1u;
   ram[0x5d6]=0x32; ram[0x5d7]=0x3b; /* SUBV R3,R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x7fffffffu && (m.cpu.sr&1u));
 }
 /* SH-4 BRA and conditional branch displacement sign-extension regressions. */
 { m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x600; m.cpu.sr&=~1u;
   ram[0x600]=0xa0; ram[0x601]=0x02; /* BRA +2 => target PC 0x608 after delay slot */
   ram[0x602]=0x00; ram[0x603]=0x09; /* delay-slot NOP */
   ram[0x604]=0x00; ram[0x605]=0x09;
   ram[0x606]=0x00; ram[0x607]=0x09;
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.pc==0x602 && m.cpu.branch_pending);
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.pc==0x608 && !m.cpu.branch_pending);
   m.cpu.pc=0x620; m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.sr|=1u;
   ram[0x620]=0x89; ram[0x621]=0xfe; /* BT -2 => PC+4-4 = 0x620 */
   ram[0x622]=0x00; ram[0x623]=0x09;
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.branch_pending);
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.pc==0x620 && !m.cpu.branch_pending);
   m.cpu.pc=0x640; m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.sr&=~1u;
   ram[0x640]=0x8b; ram[0x641]=0x01; /* BF +1 is taken when T=0 */
   ram[0x642]=0x00; ram[0x643]=0x09; /* delay-slot NOP */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.branch_pending && m.cpu.pc==0x642);
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && !m.cpu.branch_pending && m.cpu.pc==0x646);
 }
 /* SH-4 PC-relative loads and MOVA. */
 { m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x660;
   ram[0x660]=0x92; ram[0x661]=0x02; /* MOV.W @(2,PC),R2 => 0x668 */
   ram[0x668]=0x80; ram[0x669]=0x01;
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xffff8001u);
   m.cpu.pc=0x66c; ram[0x66c]=0xc4; ram[0x66d]=0x04; /* MOV.B @(4,PC),R0 => 0x674 */
   ram[0x674]=0x80;
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[0]==0xffffff80u);
   m.cpu.pc=0x670; ram[0x670]=0xd3; ram[0x671]=0x01; /* MOV.L @(1,PC),R3 => 0x678 */
   ram[0x678]=0x12; ram[0x679]=0x34; ram[0x67a]=0x56; ram[0x67b]=0x78;
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[3]==0x12345678u);
   m.cpu.pc=0x680; ram[0x680]=0xc7; ram[0x681]=0x03; /* MOVA @(3,PC),R0 => 0x690 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[0]==0x690u);
 }
 /* GD-ROM sector bounds, malformed sizes and Maple input state. */
 assert(dk_gdrom_mount(&m.gdrom,disc,sizeof(disc)-1u)!=0);
#if SIZE_MAX > UINT32_MAX
 assert(dk_gdrom_mount(&m.gdrom,disc,((size_t)UINT32_MAX+1u)*2048u)!=0);
#endif
 assert(dk_gdrom_mount(&m.gdrom,disc,sizeof(disc))==0);
 disc[2048]=0x5a;
 assert(dk_gdrom_read_sector(&m.gdrom,1,sector)==0 && sector[0]==0x5a);
 assert(dk_gdrom_read_sector(&m.gdrom,2,sector)!=0);
 dk_maple_set_state(&m.pad,0x1234,-10,20,-30,40);
 assert(m.pad.connected && m.pad.buttons==0x1234 && m.pad.lx==-10 && m.pad.ry==40);
 /* Machine raises the DMA IRQ once on the completion edge. */
 m.cpu.halted=0; m.cpu.pc=0; m.cpu.branch_pending=0;
 ram[0]=0x00; ram[1]=0x09; /* NOP */
 assert(dk_dma_start(&m.dma,0x100,0x300,4)==0);
 m.irq.raised=0;
 assert(dk_machine_step(&m,4,2)==0);
 assert(!m.dma.active && (m.irq.pending&DK_IRQ_DMA) && m.irq.raised==1);
 assert(dk_machine_step(&m,4,2)==0 && m.irq.raised==1);
 /* SH-4 indexed loads: MOV.B/W/L @(R0,Rm),Rn. */
 { m.cpu.halted=0; m.cpu.branch_pending=0; m.cpu.pc=0x690;
   m.cpu.r[0]=0x700u; m.cpu.r[3]=0x20u;
   ram[0x720]=0x80; /* signed byte */
   ram[0x722]=0x80; ram[0x723]=0x01; /* signed big-endian word */
   ram[0x724]=0x89; ram[0x725]=0xab; ram[0x726]=0xcd; ram[0x727]=0xef;
   ram[0x690]=0x02; ram[0x691]=0x3c; /* MOV.B @(R0,R3),R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xffffff80u);
   m.cpu.r[3]=0x22u; m.cpu.pc=0x692; ram[0x692]=0x02; ram[0x693]=0x3d; /* MOV.W @(R0,R3),R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0xffff8001u);
   m.cpu.r[3]=0x24u; m.cpu.pc=0x694; ram[0x694]=0x02; ram[0x695]=0x3e; /* MOV.L @(R0,R3),R2 */
   assert(dk_sh4_step(&m.cpu,&m.bus)==0 && m.cpu.r[2]==0x89abcdefu);
   assert(m.cpu.r[0]==0x700u && m.cpu.r[3]==0x24u);
 }
 free(fb); free(ram);return 0;
}
