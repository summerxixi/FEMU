#include "../nvme.h"

/* Coperd: FEMU Memory Backend (mbe) for emulated SSD */

int init_dram_backend(SsdDramBackend **mbe, int64_t nbytes)
{
    SsdDramBackend *b = *mbe = g_malloc0(sizeof(SsdDramBackend));

    b->size = nbytes;
    b->logical_space = g_malloc0(nbytes); //分配大小为 nbytes 的内存空间，并将其地址赋值给 b->logical_space。

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
    int nextReq = 0;
    int lastReq = 0;
    int sg_cur_index = 0; //当前表项的索引
    dma_addr_t sg_cur_byte = 0;//当前表项的字节偏移量
    dma_addr_t cur_addr, cur_len;//用于存储当前散列表项内的地址和长度。
    uint64_t mb_oft = lbal[0]; //mb_oft 用于跟踪目标内存缓冲区在逻辑空间中的偏移量，mb 是指向逻辑空间的指针。
    void *mb = b->logical_space;
    femu_log("The lbal[0] value is: %lu lllllllllllllllllllll\n", lbal[0] );

    DMADirection dir = DMA_DIRECTION_FROM_DEVICE;

    if (is_write) {
        dir = DMA_DIRECTION_TO_DEVICE;
    }
// as 是指向地址空间的指针，addr 是要读写的内存地址，buf 是用于存储数据的缓冲区指针，len 是要读写的数据长度，dir 是 DMA 的传输方向，attrs 是内存事务的属性。
// static inline MemTxResult dma_memory_rw(AddressSpace *as, dma_addr_t addr,void *buf, dma_addr_t len, DMADirection dir, MemTxAttrs attrs)
    while (sg_cur_index < qsg->nsg) {
        cur_addr = qsg->sg[sg_cur_index].base + sg_cur_byte; //dma里当前数据位置，qsg内部的sglist链表存放数据，链表根据sg_cur_index寻找到要访问的数据地址，并赋值给cur_addr
        cur_len = qsg->sg[sg_cur_index].len - sg_cur_byte;//剩余没处理的部分
        // （qsg->as:地址空间的数据结构，并包含了地址空间的名称、内存区域、当前映射、ioeventfd以及相关的链表,cur_assr是数据地址，cur_len衡量需要写的数据大小；
        if (dma_memory_rw(qsg->as, cur_addr, mb + mb_oft, cur_len, dir, MEMTXATTRS_UNSPECIFIED)) {
            femu_err("dma_memory_rw error\n");
        }
        //femu_log("cur_addr: %lu, cur_len:%lu, sg_cur_index:%d , mb_oft:%lu\n ",cur_addr,cur_len,sg_cur_index,mb_oft);

// 更新，要读下一块，同时如果链表1数据读完了，就读下一个链表，并且是指偏移量为0
        sg_cur_byte += cur_len;
        if (sg_cur_byte == qsg->sg[sg_cur_index].len) {
            sg_cur_byte = 0;
            ++sg_cur_index;
        }

        if (b->femu_mode == FEMU_OCSSD_MODE) {
            mb_oft = lbal[sg_cur_index];//奥奥，原来模式不同，不同的模式可能需要不同的计算方式来确定下一块数据的逻辑空间偏移量
        } else if (b->femu_mode == FEMU_BBSSD_MODE ||
                  b->femu_mode == FEMU_NOSSD_MODE ||
                  b->femu_mode == FEMU_ZNSSD_MODE) {
            mb_oft += cur_len;
            // femu_log("mb_oft:%lu    ---------------------------------------------------------------------------------------------------------\n",mb_oft);
        } else {
            assert(0);
        }
    }
    femu_log("cur_index:%lu",sg_cur_index);
// 在写操作完成后清理QEMUSGList结构所使用的资源。
    qemu_sglist_destroy(qsg);

    return 0;
}


    if (is_write) {
        dir = DMA_DIRECTION_TO_DEVICE;
    }

    while (sg_cur_index < qsg->nsg) {
        cur_addr = qsg->sg[sg_cur_index].base + sg_cur_byte;
        cur_len = qsg->sg[sg_cur_index].len - sg_cur_byte;
        if (dma_memory_rw(qsg->as, cur_addr, mb + mb_oft, cur_len, dir, MEMTXATTRS_UNSPECIFIED)) {
            femu_err("dma_memory_rw error\n");
        }

        sg_cur_byte += cur_len;
        if (sg_cur_byte == qsg->sg[sg_cur_index].len) {
            sg_cur_byte = 0;
            ++sg_cur_index;
        }

femu_err("INNNNNNNNNNNNNNNNNNNNNNNNDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDNN the BACKEND_RW___________MODE\n");
        if (b->femu_mode == FEMU_OCSSD_MODE) {
            mb_oft = lbal[sg_cur_index];
        } else if (b->femu_mode == FEMU_BBSSD_MODE ||
                   b->femu_mode == FEMU_NOSSD_MODE ||
                   b->femu_mode == FEMU_ZNSSD_MODE) {
            mb_oft += cur_len;
        } else {
            assert(0);
        }
    }

    qemu_sglist_destroy(qsg);

    return 0;
}