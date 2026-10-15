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
 /* Device windows must take precedence over the broad host RAM test range. */
 if(p>=0x00700000u&&p<0x00702000u)*v=((uint8_t*)b->aica)[p-0x00700000u];
 else if(p>=0x11000000u&&p<0x11000400u)*v=((uint8_t*)b->pvr)[p-0x11000000u];
 else if(p<b->ram_size&&b->ram)*v=b->ram[p];
 else return -1;
 b->reads++;
 return 0;}
int dk_bus_write8(dk_bus_t*b,uint32_t a,uint8_t v){uint32_t p;if(!b||phys(b,a,1,0,&p))return -1;
 /* Match read priority so MMIO never silently aliases RAM. */
 if(p>=0x00700000u&&p<0x00702000u)((uint8_t*)b->aica)[p-0x00700000u]=v;
 else if(p>=0x11000000u&&p<0x11000400u)((uint8_t*)b->pvr)[p-0x11000000u]=v;
 else if(p<b->ram_size&&b->ram)b->ram[p]=v;
 else return -1;
 b->writes++;
 return 0;}
int dk_bus_read16(dk_bus_t*b,uint32_t a,uint16_t*v){uint8_t x,y;if((a&1)||!v||dk_bus_read8(b,a,&x)||dk_bus_read8(b,a+1,&y))return -1;*v=(uint16_t)(((uint16_t)x<<8)|y);return 0;}
int dk_bus_write16(dk_bus_t*b,uint32_t a,uint16_t v){return (a&1)?-1:(dk_bus_write8(b,a,(uint8_t)(v>>8))||dk_bus_write8(b,a+1,(uint8_t)v)?-1:0);}
int dk_bus_read32(dk_bus_t*b,uint32_t a,uint32_t*v){uint16_t x,y;if((a&3)||!v||dk_bus_read16(b,a,&x)||dk_bus_read16(b,a+2,&y))return -1;*v=((uint32_t)x<<16)|y;return 0;}
int dk_bus_write32(dk_bus_t*b,uint32_t a,uint32_t v){return (a&3)?-1:(dk_bus_write16(b,a,(uint16_t)(v>>16))||dk_bus_write16(b,a+2,(uint16_t)v)?-1:0);}
void dk_sh4_reset(dk_sh4_t*c,uint32_t pc){if(c){memset(c,0,sizeof(*c));c->pc=pc;}}
static void branch(dk_sh4_t*c,uint32_t target){c->branch_target=target;c->branch_pending=1;}
int dk_sh4_step(dk_sh4_t*c,dk_bus_t*b){
 uint16_t op;uint32_t pc,oldtarget=0;uint8_t was_pending;unsigned n,m;
 if(!c||!b||c->halted)return -1;
 pc=c->pc;
 if(dk_bus_read16(b,pc,&op)){c->halted=1;return -1;}
 was_pending=c->branch_pending;if(was_pending)oldtarget=c->branch_target;c->branch_pending=0;c->pc=pc+2;n=(op>>8)&15;m=(op>>4)&15;
 if(op==0x0009){}
 else if((op&0xf000)==0xe000)c->r[n]=(uint32_t)(int32_t)(int8_t)op;
 else if((op&0xf000)==0x7000)c->r[n]+=(uint32_t)(int32_t)(int8_t)op;
 else if((op&0xf00f)==0x6003)c->r[n]=c->r[m];
 else if((op&0xf00f)==0x6000){uint8_t v;if(dk_bus_read8(b,c->r[m],&v))return -1;c->r[n]=(uint32_t)(int32_t)(int8_t)v;}
 else if((op&0xf00f)==0x6001){uint16_t v;if(dk_bus_read16(b,c->r[m],&v))return -1;c->r[n]=(uint32_t)(int32_t)(int16_t)v;}
 else if((op&0xf00f)==0x6004){uint8_t v;if(dk_bus_read8(b,c->r[m],&v))return -1;c->r[n]=(uint32_t)(int32_t)(int8_t)v;if(n!=m)c->r[m]++;}
 else if((op&0xf00f)==0x6005){uint16_t v;if(dk_bus_read16(b,c->r[m],&v))return -1;c->r[n]=(uint32_t)(int32_t)(int16_t)v;if(n!=m)c->r[m]+=2;}
 else if((op&0xf00f)==0x3000){if(c->r[n]==c->r[m])c->sr|=1u;else c->sr&=~1u;}
 else if((op&0xf00f)==0x3002){if(c->r[n]>=c->r[m])c->sr|=1u;else c->sr&=~1u;}
 else if((op&0xf00f)==0x3003){if((int32_t)c->r[n]>=(int32_t)c->r[m])c->sr|=1u;else c->sr&=~1u;}
 else if((op&0xf00f)==0x3006){if(c->r[n]>c->r[m])c->sr|=1u;else c->sr&=~1u;}
 else if((op&0xf00f)==0x3007){if((int32_t)c->r[n]>(int32_t)c->r[m])c->sr|=1u;else c->sr&=~1u;}
 else if((op&0xf00f)==0x2009)c->r[n]&=c->r[m];
 else if((op&0xf00f)==0x200a)c->r[n]^=c->r[m];
 else if((op&0xf00f)==0x200b)c->r[n]|=c->r[m];
 else if((op&0xf00f)==0x6007)c->r[n]=~c->r[m];
 else if((op&0xf00f)==0x6008)c->r[n]=(c->r[m]&0xffff0000u)|((c->r[m]&0xffu)<<8)|((c->r[m]>>8)&0xffu);
 else if((op&0xf00f)==0x6009)c->r[n]=(c->r[m]<<16)|(c->r[m]>>16);
 else if((op&0xf00f)==0x600c)c->r[n]=c->r[m]&0xffu;
 else if((op&0xf00f)==0x600d)c->r[n]=c->r[m]&0xffffu;
 else if((op&0xf00f)==0x600e)c->r[n]=(uint32_t)(int32_t)(int8_t)c->r[m];
 else if((op&0xf00f)==0x600f)c->r[n]=(uint32_t)(int32_t)(int16_t)c->r[m];
 else if((op&0xf00f)==0x200d)c->r[n]=(c->r[n]>>16)|(c->r[m]<<16);
 else if((op&0xf00f)==0x600b)c->r[n]=0u-c->r[m];
 else if((op&0xf0ff)==0x0029){if(c->sr&1u)c->r[n]|=1u;else c->r[n]&=~1u;}
 else if((op&0xf0ff)==0x4000)c->r[n]<<=1;
 else if((op&0xf0ff)==0x4001)c->r[n]>>=1;
 else if((op&0xf0ff)==0x4004)c->r[n]=(c->r[n]<<1)|(c->r[n]>>31);
 else if((op&0xf0ff)==0x4005)c->r[n]=(c->r[n]>>1)|(c->r[n]<<31);
 else if((op&0xf0ff)==0x4008)c->r[n]<<=2;
 else if((op&0xf0ff)==0x4009)c->r[n]>>=2;
 else if((op&0xf0ff)==0x4018)c->r[n]<<=8;
 else if((op&0xf0ff)==0x4019)c->r[n]>>=8;
 else if((op&0xff00)==0xc800){if((c->r[0]&(uint32_t)(uint8_t)op)==0)c->sr|=1u;else c->sr&=~1u;}
 else if((op&0xff00)==0xc900)c->r[0]&=(uint32_t)(uint8_t)op;
 else if((op&0xff00)==0xca00)c->r[0]^=(uint32_t)(uint8_t)op;
 else if((op&0xff00)==0xcb00)c->r[0]|=(uint32_t)(uint8_t)op;
 else if((op&0xff00)==0x8800){if(c->r[0]==(uint32_t)(int32_t)(int8_t)op)c->sr|=1u;else c->sr&=~1u;}
 else if((op&0xf00f)==0x2008){if((c->r[n]&c->r[m])==0)c->sr|=1u;else c->sr&=~1u;}
 else if((op&0xf00f)==0x2000){if(dk_bus_write8(b,c->r[n],(uint8_t)c->r[m]))return -1;}
 else if((op&0xf00f)==0x2001){if(dk_bus_write16(b,c->r[n],(uint16_t)c->r[m]))return -1;}
 else if((op&0xf00f)==0x2004){c->r[n]--;if(dk_bus_write8(b,c->r[n],(uint8_t)c->r[m]))return -1;}
 else if((op&0xf00f)==0x2005){c->r[n]-=2;if(dk_bus_write16(b,c->r[n],(uint16_t)c->r[m]))return -1;}
 else if((op&0xf0ff)==0x4010){c->r[n]--;if(c->r[n]==0)c->sr|=1u;else c->sr&=~1u;}
 else if(op==0x0008)c->sr&=~1u;
 else if(op==0x0018)c->sr|=1u;
 else if((op&0xf00f)==0x300c)c->r[n]+=c->r[m];
 else if((op&0xf00f)==0x3008)c->r[n]-=c->r[m];
 else if((op&0xf00f)==0x2002){if(dk_bus_write32(b,c->r[n],c->r[m]))return -1;}
 else if((op&0xf00f)==0x2006){c->r[n]-=4;if(dk_bus_write32(b,c->r[n],c->r[m]))return -1;}
 else if((op&0xf00f)==0x6002){uint32_t v;if(dk_bus_read32(b,c->r[m],&v))return -1;c->r[n]=v;}
 else if((op&0xf00f)==0x6006){uint32_t v;if(dk_bus_read32(b,c->r[m],&v))return -1;c->r[n]=v;if(n!=m)c->r[m]+=4;}
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
uint32_t dk_dma_step(dk_dma_t*d,dk_bus_t*b,uint32_t budget){
 uint32_t k=0;uint8_t v;if(!d||!b)return 0;
 while(d->active&&k<budget&&d->done<d->length){
  /* Reject wrapped source/destination addresses instead of aliasing low RAM. */
  if(d->done>UINT32_MAX-d->src||d->done>UINT32_MAX-d->dst||
     dk_bus_read8(b,d->src+d->done,&v)||dk_bus_write8(b,d->dst+d->done,v)){
   d->error=1;d->active=0;break;
  }
  d->done++;k++;
 }
 if(d->done>=d->length)d->active=0;
 return k;
}
void dk_irq_raise(dk_irq_t*i,uint32_t bits){if(i){i->pending|=bits;i->raised++;}}
void dk_irq_ack(dk_irq_t*i,uint32_t bits){if(i)i->pending&=~bits;}
uint32_t dk_irq_active(const dk_irq_t*i){return i?(i->pending&~i->mask):0;}
void dk_scheduler_advance(dk_scheduler_t*s,uint64_t c){if(s)s->now+=c;}
void dk_scheduler_schedule(dk_scheduler_t*s,uint64_t d){if(s){s->deadline=(UINT64_MAX-s->now<d)?UINT64_MAX:s->now+d;s->armed=1;}}
int dk_scheduler_due(const dk_scheduler_t*s){return s&&s->armed&&s->now>=s->deadline;}
void dk_pvr_reset(dk_pvr2_t*p){if(p){memset(p,0,sizeof(*p));p->width=640;p->height=480;}}
int dk_pvr_set_mode(dk_pvr2_t*p,uint32_t w,uint32_t h){if(!p||!w||!h||w>1024||h>1024)return -1;p->width=w;p->height=h;return 0;}
int dk_pvr_attach_framebuffer(dk_pvr2_t*p,uint32_t*pixels,size_t capacity){if(!p||!pixels||capacity<(size_t)p->width*p->height)return -1;p->framebuffer=pixels;p->capacity=capacity;return 0;}
int dk_pvr_clear(dk_pvr2_t*p,uint32_t rgba){size_t i,n;if(!p||!p->framebuffer)return -1;n=(size_t)p->width*p->height;if(n>p->capacity)return -1;for(i=0;i<n;i++)p->framebuffer[i]=rgba;return 0;}
int dk_pvr_fill_rect(dk_pvr2_t*p,int x,int y,int w,int h,uint32_t rgba){
 int x0,y0,x1,y1,yy,xx;int64_t rx1,ry1;
 if(!p||!p->framebuffer||w<=0||h<=0||p->capacity<(size_t)p->width*p->height)return -1;
 rx1=(int64_t)x+(int64_t)w;ry1=(int64_t)y+(int64_t)h;
 x0=x<0?0:x;y0=y<0?0:y;x1=(int)p->width;y1=(int)p->height;
 if(rx1<x1)x1=(int)rx1;
 if(ry1<y1)y1=(int)ry1;
 if(x0>=x1||y0>=y1)return 0;
 for(yy=y0;yy<y1;yy++)for(xx=x0;xx<x1;xx++)p->framebuffer[(size_t)yy*p->width+(size_t)xx]=rgba;
 return 0;
}
void dk_pvr_present(dk_pvr2_t*p){if(p){p->frames++;p->vblank=1;}}
void dk_aica_reset(dk_aica_t*a){if(a){memset(a,0,sizeof(*a));a->sample_rate=44100;}}
size_t dk_aica_mix_tone(dk_aica_t*a,uint32_t frequency,int16_t amplitude,size_t count,int16_t*out){size_t i;if(!a||!out||!a->sample_rate)return 0;if(count>2048)count=2048;for(i=0;i<count;i++){a->phase=(uint32_t)(((uint64_t)a->phase+(uint64_t)frequency)%a->sample_rate);out[i]=(a->phase<a->sample_rate/2)?amplitude:(int16_t)(-(int32_t)amplitude);a->mix[i]=out[i];}a->samples+=(uint32_t)count;return count;}
void dk_gdrom_reset(dk_gdrom_t*g){if(g){memset(g,0,sizeof(*g));g->ready=1;}}
int dk_gdrom_mount(dk_gdrom_t*g,const uint8_t*image,size_t bytes){if(!g||!image||!bytes||bytes%2048)return -1;g->image=image;g->image_size=bytes;g->sectors=(uint32_t)(bytes/2048);g->ready=1;g->reading=0;return 0;}
int dk_gdrom_read_sector(dk_gdrom_t*g,uint32_t lba,uint8_t out[2048]){if(!g||!out||!g->ready||!g->image||lba>=g->sectors)return -1;g->reading=1;g->lba=lba;memcpy(out,g->image+(size_t)lba*2048,2048);g->reading=0;return 0;}
void dk_maple_reset(dk_maple_t*p){if(p){memset(p,0,sizeof(*p));p->connected=1;}}
void dk_maple_set_state(dk_maple_t*p,uint16_t buttons,int8_t lx,int8_t ly,int8_t rx,int8_t ry){if(p){p->buttons=buttons;p->lx=lx;p->ly=ly;p->rx=rx;p->ry=ry;p->connected=1;}}
int dk_machine_init(dk_machine_t*m,uint8_t*ram,size_t size,uint32_t entry){if(!m||!ram||size<4096)return -1;memset(m,0,sizeof(*m));m->ram=ram;dk_mmu_reset(&m->mmu);dk_bus_init(&m->bus,ram,size,&m->mmu);dk_sh4_reset(&m->cpu,entry);dk_dma_reset(&m->dma);dk_pvr_reset(&m->pvr);dk_aica_reset(&m->aica);dk_gdrom_reset(&m->gdrom);dk_maple_reset(&m->pad);return 0;}
int dk_machine_step(dk_machine_t*m,uint32_t budget,uint32_t cycles){int rc;uint8_t dma_was_active;if(!m)return -1;rc=dk_sh4_step(&m->cpu,&m->bus);dma_was_active=m->dma.active;dk_dma_step(&m->dma,&m->bus,budget);dk_scheduler_advance(&m->scheduler,cycles);if(dma_was_active&&!m->dma.active&&!m->dma.error&&m->dma.done==m->dma.length)dk_irq_raise(&m->irq,DK_IRQ_DMA);return rc;}
