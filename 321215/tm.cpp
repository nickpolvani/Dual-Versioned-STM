/**
 * @file   tm.c
 * @author [...]
 *
 * @section LICENSE
 *
 * [...]
 *
 * @section DESCRIPTION
 *
 * Implementation of your own transaction manager.
 * You can completely rewrite this file (and create more files) as you wish.
 * Only the interface (i.e. exported symbols and semantic) must be preserved.
**/

// Requested features
//#define _GNU_SOURCE
#define _POSIX_C_SOURCE   200809L
#ifdef __STDC_NO_ATOMICS__
    #error Current C11 compiler does not support atomic operations
#endif

// External headers

#include "debug.hpp"

// Internal headers

#include <tm.hpp>
#include "word.hpp"
#include "dual_stm.hpp"
#include "macros.h"
#include "transaction.hpp"
#include "segment.hpp"
#include "transaction.hpp"
#include "batcher.hpp"
#include <iostream>
#include <string.h>


static size_t convertToInt(void const * a, size_t size);

// converts character array
// to string and returns it
static size_t convertToInt(void const * a, size_t size){
    size_t i;
    memcpy(&i, a, size);  
    return i;
}


/** Create (i.e. allocate + init) a new shared memory region, with one first non-free-able allocated segment of the requested size and alignment.
 * @param size  Size of the first shared segment of memory to allocate (in bytes), must be a positive multiple of the alignment
 * @param align Alignment (in bytes, must be a power of 2) that the shared memory region must support
 * @return Opaque shared memory region handle, 'invalid_shared' on failure
**/
shared_t tm_create(size_t size, size_t align) noexcept {
    DualStm* stm = new DualStm(size, align);
    return stm;
}

/** Destroy (i.e. clean-up + free) a given shared memory region.
 * @param shared Shared memory region to destroy, with no running transaction
**/
void tm_destroy(shared_t shared) noexcept {
    DualStm* stm = reinterpret_cast<DualStm*>(shared);
    delete stm;
}

/** [thread-safe] Return the start address of the first allocated segment in the shared memory region.
 * @param shared Shared memory region to query
 * @return Start address of the first allocated segment
**/
void* tm_start(shared_t shared)noexcept {
    DualStm* stm = reinterpret_cast<DualStm*>(shared);
    return stm->getHead();
}

/** [thread-safe] Return the size (in bytes) of the first allocated segment of the shared memory region.
 * @param shared Shared memory region to query
 * @return First allocated segment size
**/
size_t tm_size(shared_t shared)noexcept{
    DualStm* stm = reinterpret_cast<DualStm*>(shared);
    return stm->getSize();
}

/** [thread-safe] Return the alignment (in bytes) of the memory accesses on the given shared memory region.
 * @param shared Shared memory region to query
 * @return Alignment used globally
**/
size_t tm_align(shared_t shared)noexcept {
    DualStm* stm = reinterpret_cast<DualStm*>(shared);
    return stm->alignment;
}

/** [thread-safe] Begin a new transaction on the given shared memory region.
 * @param shared Shared memory region to start a transaction on
 * @param is_ro  Whether the transaction is read-only
 * @return Opaque transaction ID, 'invalid_tx' on failure
**/
tx_t tm_begin(shared_t shared, bool is_ro) noexcept{
    DualStm* stm = reinterpret_cast<DualStm*>(shared);
    Transaction* tx = stm->begin(is_ro);
    tx_t res = reinterpret_cast<tx_t>(tx);
    return res;
}

/** [thread-safe] End the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to end
 * @return Whether the whole transaction committed
**/
bool tm_end(shared_t shared, tx_t tx)noexcept {
    DualStm* stm = reinterpret_cast<DualStm*>(shared);
    Transaction* t = reinterpret_cast<Transaction*>(tx);
    bool committed = stm->end(t);
    return committed;
}

/** [thread-safe] Read operation in the given transaction, source in the shared region and target in a private region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in the shared region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in a private region)
 * @return Whether the whole transaction can continue
**/
bool tm_read(shared_t shared, tx_t tx, void const* source, size_t size, void* target)noexcept {
    DualStm* stm = reinterpret_cast<DualStm*>(shared);
    Transaction* t = reinterpret_cast<Transaction*>(tx);
    bool can_continue = stm->read(t, source, size, target);
    if (can_continue){
      //  DEBUG_MSG("Transaction " << t->tr_num << " successfully read " << convertToInt(target, size) << " in " << size << " bytes from address: " << reinterpret_cast<std::size_t>(source));
    }else{
      //  DEBUG_MSG("Transaction " << t->tr_num << " aborted when reading " << size << " bytes from address: " << reinterpret_cast<std::size_t>(source));
    }
    return can_continue;
}





/** [thread-safe] Write operation in the given transaction, source in a private region and target in the shared region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in a private region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in the shared region)
 * @return Whether the whole transaction can continue
**/
bool tm_write(shared_t shared, tx_t tx, void const* source, size_t size, void* target)noexcept {
    DualStm* stm = reinterpret_cast<DualStm*>(shared);
    Transaction* t = reinterpret_cast<Transaction*>(tx);
    bool can_continue = stm->write(t, source, size, target);
    if (can_continue){
       // DEBUG_MSG("Transaction " << t->tr_num << " successfully wrote " << convertToInt(source, size) << " in " << size << " bytes to address: " << reinterpret_cast<std::size_t>(target));
    }else{
       // DEBUG_MSG("Transaction " << t->tr_num << " aborted when writing " << convertToInt(source, size) << " in " << size << " bytes to address: " << reinterpret_cast<std::size_t>(target));
    }
    return can_continue;
}

/** [thread-safe] Memory allocation in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param size   Allocation requested size (in bytes), must be a positive multiple of the alignment
 * @param target Pointer in private memory receiving the address of the first byte of the newly allocated, aligned segment
 * @return Whether the whole transaction can continue (success/nomem), or not (abort_alloc)
**/
Alloc tm_alloc(shared_t shared, tx_t tx, size_t size, void** target) noexcept {
    DualStm* stm = reinterpret_cast<DualStm*>(shared);
    Transaction* t = reinterpret_cast<Transaction*>(tx);
    std::cout<<"Transaction " << t->tr_num << " epoch " << t->epoch <<" allocating segment of size " << size << "\n";

    bool can_continue = stm -> alloc(t, size, target);
    if (can_continue){
        char addr[8];
        memcpy(addr, target, 8);
        std::cout << "allocated segment of size "  << size << " at address " << convertToInt(addr, 8) << std::endl;
        return Alloc(0);
    }else{
        std::cout << "Failed allocating segment\n";
        return Alloc(1);
    }
}

/** [thread-safe] Memory freeing in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param target Address of the first byte of the previously allocated segment to deallocate
 * @return Whether the whole transaction can continue
**/
bool tm_free(shared_t shared, tx_t tx, void* target) noexcept{
    DualStm* stm = reinterpret_cast<DualStm*>(shared);
    Transaction* t = reinterpret_cast<Transaction*>(tx);
    std::size_t addr = reinterpret_cast<std::size_t>(target);
    std::cout<<"Transaction " << t->tr_num << " epoch " << t->epoch << " freeing segment starting at: " << addr << "\n";
    bool can_continue = stm->free(t, target);
    if(can_continue){
        std::cout << "Segment successfully freed from address: " << addr << std::endl;
    }
    return can_continue;
}
