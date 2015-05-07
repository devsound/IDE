#include <mchck.h>

static struct dma_ctx_t {
        dma_cb* cb;
} ctx[4];

void
dma_init(void)
{
        SIM.scgc6.dmamux = 1;
        SIM.scgc7.dma = 1;
        /* set some defaults */
        int i;
        for (i = 0; i < 4; i++) {
                volatile struct DMA_TCD_t* tcd = &DMA.tcd[i];
                tcd->soff = 0; /* no source offset */
                tcd->attr.raw = 0;
                tcd->slast = 0; /* no source address adjustment */
                tcd->doff = 0; /* no dest offset */
                tcd->citer.elinkno.elink = 0; /* no minor loop linking */
                tcd->citer.elinkno.iter = 1; /* 1 major loop iteration */
                tcd->dlastsga = 0; /* no dest address adjustment */
                tcd->csr.intmajor = 1; /* enable interrupt after major loop finishes */
                tcd->csr.majorlinkch = 0; /* don't chain (link) after major loop */
                tcd->csr.bwc = 0; /* no dma stalls */
                tcd->biter.elinkno.elink = 0; /* no minor loop linking */
                tcd->biter.elinkno.iter = 1; /* 1 major loop iteration */
        }
        DMA.seei.saee = 1; /* enable all error interrupts */
        int_enable(IRQ_DMA0);
        int_enable(IRQ_DMA1);
        int_enable(IRQ_DMA2);
        int_enable(IRQ_DMA3);
        int_enable(IRQ_DMA_error);
}

void
dma_set_arbitration(enum dma_arbitration_t arb)
{
        DMA.cr.erca = arb;
}

void
dma_from(enum dma_channel ch, void* addr, size_t count, enum dma_transfer_size_t tsize, size_t off, uint8_t mod)
{
        volatile struct DMA_TCD_t* tcd = &DMA.tcd[ch];
        tcd->saddr = (uint32_t)addr;
        tcd->soff = off;
        tcd->attr.ssize = tsize;
        tcd->attr.smod = mod;
        tcd->nbytes.mlno.nbytes = count * (1 << tsize);
        tcd->slast = -(1 << tsize); // adjust to tsize by default
}

void
dma_to(enum dma_channel ch, void* addr, size_t count, enum dma_transfer_size_t tsize, size_t off, uint8_t mod)
{
        volatile struct DMA_TCD_t* tcd = &DMA.tcd[ch];
        tcd->daddr = (uint32_t)addr;
        tcd->doff = off;
        tcd->attr.dsize = tsize;
        tcd->attr.dmod = mod;
        tcd->nbytes.mlno.nbytes = count * (1 << tsize);
        tcd->dlastsga = -(1 << tsize); // adjust to tsize by default
}

void
dma_major_loop_count(enum dma_channel ch, uint16_t iter)
{
        volatile struct DMA_TCD_t* tcd = &DMA.tcd[ch];
        if (tcd->citer.elinkyes.elink) {
                tcd->citer.elinkyes.iter = iter;
                tcd->biter.elinkyes.iter = iter;
        } else {
                tcd->citer.elinkno.iter = iter;
                tcd->biter.elinkno.iter = iter;
        }
}

void
dma_start(enum dma_channel ch, enum dma_mux_source_t source, uint8_t trig, dma_cb* cb)
{
        ctx[ch].cb = cb;
        volatile struct DMAMUX_t *mux = &DMAMUX[ch];
        mux->enbl = 0;
        mux->trig = 0;
        mux->source = source;
        DMA.tcd[ch].csr.start = 1;
        if (trig)
                mux->trig = 1;
        mux->enbl = 1;
}

void
dma_cancel(enum dma_channel ch)
{
        DMAMUX[ch].enbl = 0;
        DMA.cr.cx = 1;
}

void
dma_set_priority(enum dma_channel ch, uint8_t prio)
{
        DMA.dchpri[ch].chpri = prio;
}

void
dma_enable_channel_preemption(enum dma_channel ch, uint8_t on)
{
        DMA.dchpri[ch].ecp = on;
}

void
dma_enable_preempt_ability(enum dma_channel ch, uint8_t on)
{
        DMA.dchpri[ch].dpa = !on;
}

void
dma_from_addr_adj(enum dma_channel ch, uint32_t off)
{
        DMA.tcd[ch].slast = off;
}

void
dma_to_addr_adj(enum dma_channel ch, uint32_t off)
{
        DMA.tcd[ch].dlastsga = off;
}

#define COMMON_HANDLER(ch)                                        \
        uint8_t curr = DMA.tcd[ch].citer.elinkyes.elink ?        \
                DMA.tcd[ch].citer.elinkyes.iter :                \
                DMA.tcd[ch].citer.elinkno.iter;                \
        ctx[ch].cb(ch, 0, curr);                                \
        /* BEGIN freescale 4588 workaround */                        \
        if (DMAMUX[ch].trig) {                                        \
                DMA.tcd[ch].csr.dreq = 0;                        \
                DMAMUX[ch].enbl = 0;                                \
                DMAMUX[ch].enbl = 1;                                \
                DMA.serq.serq = ch;                                \
        }                                                        \
        /* END freescale 4588 workaround */                        \
        DMA.cint.cint = ch;                                        \

void
DMA0_Handler(void)
{
        COMMON_HANDLER(0);
}

void
DMA1_Handler(void)
{
        COMMON_HANDLER(1);
}

void
DMA2_Handler(void)
{
        COMMON_HANDLER(2);
}

void
DMA3_Handler(void)
{
        COMMON_HANDLER(3);
}

void
DMA_error_Handler(void)
{
        uint32_t err = *(uint8_t*)&DMA.es;
        if (DMA.es.cpe)
                err |= DMA_ERR_CHNL_PRIO;
        if (DMA.es.ecx)
                err |= DMA_ERR_TRNFS_CANCL;
        ctx[DMA.es.errchn].cb(DMA.es.errchn, err, 0);
        DMA.cerr.cerr = DMA.es.errchn; // clear
}