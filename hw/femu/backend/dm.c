#include "../nvme.h"

/* Coperd: FEMU Memory Backend (mbe) for emulated SSD */

int init_dram_backend(SsdDramBackend **mbe, int64_t nbytes)
{
    SsdDramBackend *b = *mbe = g_malloc0(sizeof(SsdDramBackend));

    b->size = nbytes;
    b->logical_space = g_malloc0(nbytes);

    if (mlock(b->logical_space, nbytes) == -1) {
        femu_err("Failed to pin the memory backend to the host DRAM\n");
        g_free(b->logical_space);
        abort();
    }

    return 0;
}

void free_dram_backend(SsdDramBackend *b)
{
    if (b->logical_space) {
        munlock(b->logical_space, b->size);
        g_free(b->logical_space);
    }
}

int backend_rw(SsdDramBackend *b, QEMUSGList *qsg, uint64_t *lbal, bool is_write)
{
    int sg_cur_index = 0;
    dma_addr_t sg_cur_byte = 0;
    dma_addr_t cur_addr, cur_len;
    uint64_t mb_oft = lbal[0];
    void *mb = b->logical_space;
    uint64_t chdata = 0;
    femu_log("The lbal[0] value is: %lu -------------------------------------------------------------------\n", lbal[0] );
      
 


    DMADirection dir = DMA_DIRECTION_FROM_DEVICE;

    if (is_write) {
        dir = DMA_DIRECTION_TO_DEVICE;
    }

    while (sg_cur_index < qsg->nsg) {
        cur_addr = qsg->sg[sg_cur_index].base + sg_cur_byte;
        cur_len = qsg->sg[sg_cur_index].len - sg_cur_byte;
        
        if (dma_memory_rw(qsg->as, cur_addr, mb + mb_oft, cur_len, dir, MEMTXATTRS_UNSPECIFIED)) {
            femu_err("dma_memory_rw error\n");
        }
      // femu_log("Data at address %p: %lu\n", (void*)(mb + mb_oft), *(uint64_t*)(mb + mb_oft));
       //void* ptr = (void*)(uintptr_t)0x7f60abfff010;
    //   uint64_t* data_ptr = (uint64_t*)ptr;
       //uint64_t data = *data_ptr;

        
        femu_log("cur_addr: %lu, cur_addr_data:%lu, sg_cur_index:%d, data at %p: %lu \n  ",cur_addr,*(cur_addr),sg_cur_index,(void*)(mb + mb_oft), *(uint64_t*)(mb + mb_oft));
        sg_cur_byte += cur_len;
        if (sg_cur_byte == qsg->sg[sg_cur_index].len) {
            sg_cur_byte = 0;
            ++sg_cur_index;
        }
        
        

//femu_err("INNNNNNNNNNNNNNNNNNNNNNNNDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDNN the BACKEND_RW___________MODE\n");
        if (b->femu_mode == FEMU_OCSSD_MODE) {
            mb_oft = lbal[sg_cur_index];
        } else if (b->femu_mode == FEMU_BBSSD_MODE ||
                  b->femu_mode == FEMU_NOSSD_MODE ||
                  b->femu_mode == FEMU_ZNSSD_MODE) {
         femu_log("mb_oft:%lu *****************************************************************************\n",mb_oft) ;        
              //    femu_log("mb_oft:%lu   ,data1: %lu,data2: %lu ,data3: %lu  \n",mb_oft,*(uint64_t*)0x7f60ac004010,*(uint64_t*) 0x7f60abfff010,*(uint64_t*)0x7f60ac003010);
            mb_oft += cur_len;
        } else {
            assert(0);
        }
    }

    qemu_sglist_destroy(qsg);

    return 0;
}