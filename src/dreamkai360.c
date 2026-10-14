#include "dreamkai360.h"
#include <string.h>

void dk_mmu_reset(dk_mmu_t *m){ if(m) memset(m,0,sizeof(*m)); }
int dk_mmu_map(dk_mmu_t *m,unsigned s,uint32_t va,uint32_t pa,int wr,int ex){
 if(!m||s>=DK_TLB_ENTRIES||(va&(DK_PAGE_SIZE-1))||(pa&(DK_PAGE_SIZE-1))) return -1;
 m->e[s]=(dk_tlb_entry_t){va>>12,pa>>12,1,(uint8_t)!!wr,(uint8_t)!!ex,m->asid}; return 0;
}
int dk_mmu_translate(const dk_mmu_t *m,uint32_t va,int wr,int ex,uint32_t *pa){
 unsigned i; if(!pa)return -1;
 if(!m||!m->enabled||va>=0x80000000u){*pa=va&0x1fffffffu;return 0;}
 for(i=0;i<DK_TLB_ENTRIES;i++){const dk_tlb_entry_t *e=&m->e[i];
  if(e->valid&&e->vpn==(va>>12)&&(e->asid==m->asid||((va&0xe0000000u)==0xe0000000u))){
   if((wr&&!e->write)||(ex&&!e->execute))return -2;
   *pa=(e->ppn<<12)|(va&(DK_PAGE_SIZE-1));return 0;
  }}
 return -3;
}
static int phys(dk_bus_t *b,uint32_t va,int wr,int ex,uint32_t *p){
 if(dk_mmu_translate(b?b->mmu:0,va,wr,ex,p)){if(b)b->faults++;return -1;}
 *p &= 0x1fffffffu; return 0;
}
void dk_bus_init(dk_bus_t *b,uint8_t *r,size_t n,dk_mmu_t *m){if(!b)return;memset(b,0,sizeof(*b));b->ram=r;b->ram_size=n;b->mmu=m;}
int dk_bus_read8(dk_bus_t*b,uint32_t a,uint8_t*v){uint32_t p;if(!b||!v||phys(b,a,0,0,&p))return -1;
 if(p<b->ram_size&&b->ram){*v=b->ram[p];b->reads++;return 0;}
 if(p>=0x11000000u&&p<0x11000400u)*v=((uint8_t*)b->pvr)[p-0x11000000u];
 else if(p>=0x00700000u&&p<0x00702000u)*v=((uint8_t*)b->aica)[p-0x00700000u];
 else return -1;b->reads++;return 0;}
int dk_bus_write8(dk_bus_t*b,uint32_t a,uint8_t v){uint32_t p;if(!b||phys(b,a,1,0,&p))return -1;
 if(p<b->ram_size&&b->ram){b->ram[p]=v;b->writes++;return 0;}
 if(p>=0x11000000u&&p<0x11000400u)((uint8_t*)b->pvr)[p-0x11000000u]=v;
 else if(p>=0x00700000u&&p<0x00702000u)((uint8_t*)b->aica)[p-0x00700000u]=v;
 else return -1;b->writes++;return 0;}
int dk_bus_read16(dk_bus_t*b,uint32_t a,uint16_t*v){uint8_t x,y;if((a&1)||!v||dk_bus_read8(b,a,&x)||dk_bus_read8(b,a+1,&y))return -1;*v=(uint16_t)(((uint16_t)x<<8)|y);return 0;}
int dk_bus_write16(dk_bus_t*b,uint32_t a,uint16_t v){return (a&1)?-1:(dk_bus_write8(b,a,(uint8_t)(v>>8))||dk_bus_write8(b,a+1,(uint8_t)v)?-1:0);}
int dk_bus_read32(dk_bus_t*b,uint32_t a,uint32_t*v){uint16_t x,y;if((a&3)||!v||dk_bus_read16(b,a,&x)||dk_bus_read16(b,a+2,&y))return -1;*v=((uint32_t)x<<16)|y;return 0;}
int dk_bus_write32(dk_bus_t*b,uint32_t a,uint32_t v){return (a&3)?-1:(dk_bus_write16(b,a,(uint16_t)(v>>16))||dk_bus_write16(b,a+2,(uint16_t)v)?-1:0);}
void dk_sh4_reset(dk_sh4_t*c,uint32_t pc){if(c){memset(c,0,sizeof(*c));c->pc=pc;}}
static void branch(dk_sh4_t*c,uint32_t target){c->branch_target=target;c->branch_pending=1;}
int dk_sh4_step(dk_sh4_t*c,dk_bus_t*b){
 uint16_t op;uint32_t pc,oldtarget=0;uint8_t was_pending;unsigned n,m;
 if(!c||!b||c->halted)return -1;pc=c->pc;
 if(dk_bus_read16(b,pc,&op)){c->halted=1;return -1;}
 was_pending=c->branch_pending;if(was_pending)oldtarget=c->branch_target;c->branch_pending=0;c->pc=pc+2;n=(op>>8)&15;m=(op>>4)&15;
 if(op==0x0009){}
 else if((op&0xf000)==0xe000)c->r[n]=(uint32_t)(int32_t)(int8_t)op;
 else if((op&0xf00f)==0x7000)c->r[n]+=(uint32_t)(int32_t)(int8_t)op;
 else if((op&0xf00f)==0x6003)c->r[n]=c->r[m];
 else if((op&0xf00f)==0x300c)c->r[n]+=c->r[m];
 else if((op&0xf00f)==0x3008)c->r[n]-=c->r[m];
 else if((op&0xf00f)==0x2002){if(dk_bus_write32(b,c->r[n],c->r[m]))return -1;}
 else if((op&0xf00f)==0x6002){uint32_t v;if(dk_bus_read32(b,c->r[m],&v))return -1;c->r[n]=v;}
 else if((op&0xf000)==0xa000)branch(c,pc+4+((int32_t)(int16_t)(op<<4)>>3));
 else if((op&0xff00)==0x8900){if(c->sr&1)branch(c,pc+4+((int8_t)op*2));}
 else if((op&0xff00)==0x8b00){if(!(c->sr&1))branch(c,pc+4+((int8_t)op*2));}
 else if(op==0x000b)branch(c,c->pr);
 else if(op==0x001b)c->halted=1;
 else {c->halted=1;return -2;}
 if(was_pending){if(c->branch_pending){c->halted=1;return -3;}c->pc=oldtarget;}
 c->r[0]=0;c->cycles+=2;b->fetches++;return 0;
}
void dk_dma_reset(dk_dma_t*d){if(d)memset(d,0,sizeof(*d));}
int dk_dma_start(dk_dma_t*d,uint32_t s,uint32_t t,uint32_t n){if(!d||!n||d->active)return -1;d->src=s;d->dst=t;d->length=n;d->done=0;d->active=1;d->error=0;return 0;}
uint32_t dk_dma_step(dk_dma_t*d,dk_bus_t*b,uint32_t budget){uint32_t k=0;uint8_t v;if(!d||!b)return 0;while(d->active&&k<budget&&d->done<d->length){if(dk_bus_read8(b,d->src+d->done,&v)||dk_bus_write8(b,d->dst+d->done,v)){d->error=1;d->active=0;break;}d->done++;k++;}if(d->done>=d->length)d->active=0;return k;}
void dk_irq_raise(dk_irq_t*i,uint32_t bits){if(i){i->pending|=bits;i->raised++;}}
uint32_t dk_irq_active(const dk_irq_t*i){return i?(i->pending&~i->mask):0;}
void dk_scheduler_advance(dk_scheduler_t*s,uint64_t c){if(s)s->now+=c;}
void dk_scheduler_schedule(dk_scheduler_t*s,uint64_t d){if(s){s->deadline=s->now+d;s->armed=1;}}
void dk_pvr_reset(dk_pvr2_t*p){if(p){memset(p,0,sizeof(*p));p->width=640;p->height=480;}}
int dk_pvr_set_mode(dk_pvr2_t*p,uint32_t w,uint32_t h){if(!p||!w||!h||w>1024||h>1024)return -1;p->width=w;p->height=h;return 0;}
void dk_aica_reset(dk_aica_t*a){if(a){memset(a,0,sizeof(*a));a->sample_rate=44100;}}
void dk_gdrom_reset(dk_gdrom_t*g){if(g){memset(g,0,sizeof(*g));g->ready=1;}}
void dk_maple_reset(dk_maple_t*p){if(p){memset(p,0,sizeof(*p));p->connected=1;}}
int dk_machine_init(dk_machine_t*m,uint8_t*ram,size_t size,uint32_t entry){if(!m||!ram||size<4096)return -1;memset(m,0,sizeof(*m));m->ram=ram;dk_mmu_reset(&m->mmu);dk_bus_init(&m->bus,ram,size,&m->mmu);dk_sh4_reset(&m->cpu,entry);dk_dma_reset(&m->dma);dk_pvr_reset(&m->pvr);dk_aica_reset(&m->aica);dk_gdrom_reset(&m->gdrom);dk_maple_reset(&m->pad);return 0;}
int dk_machine_step(dk_machine_t*m,uint32_t budget,uint32_t cycles){int rc;if(!m)return -1;rc=dk_sh4_step(&m->cpu,&m->bus);dk_dma_step(&m->dma,&m->bus,budget);dk_scheduler_advance(&m->scheduler,cycles);if(!m->dma.active&&!m->dma.error&&m->dma.length&&m->dma.done==m->dma.length)dk_irq_raise(&m->irq,DK_IRQ_DMA);return rc;}
