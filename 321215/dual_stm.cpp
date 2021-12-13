#include "dual_stm.hpp"
#include "segment.hpp"
#include "batcher.hpp"
#include "transaction.hpp"
#include <assert.h>
#include <string.h>
#include <iostream>
#include "debug.hpp"

// Create (i.e. allocate + init) a new shared memory region, with one first allocated segment of
// the requested size and alignment.
// Initializes also the batcher
DualStm::DualStm(std::size_t i_size, std::size_t i_alignment):
alignment(i_alignment), size_first_segment(i_size){
    std::size_t start_address = std::atomic_fetch_add(&end_address, i_size);
    std::size_t end_address = start_address + i_size;
    std::size_t num_words = i_size / alignment;
    Segment* segment = new Segment(alignment, num_words, start_address);
    segments[start_address] = segment;
    addresses[end_address] = start_address;
    batcher = new Batcher(this);
}

// deallocate all segments, delete Batcher
DualStm::~DualStm(){
    for (auto it = segments.begin(); it != segments.end(); it++){
        delete it->second;
    }
    delete batcher;
}

// Get a pointer in shared memory to the first allocated segment of the shared memory region
void* DualStm::getHead(){
    std::size_t first_address = 1;
    return reinterpret_cast<void*>(first_address);
}



// adds segment to segments, invoked by the batcher at the end of one epoch (sequentially)
// to add newly allocated segments by the committed transactions
void DualStm::addSegment(std::size_t start_addr, Segment* segment){
    segments[start_addr] = segment;
    std::size_t seg_end_addr = start_addr + (segment->num_words * segment->alignment);
    addresses[seg_end_addr] = start_addr;
}


// deletes segment with address at start address, if not already deleted.
// invoked by the batcher at the end of one epoch
void DualStm::freeSegment(size_t start_address){
    if (segments.count(start_address) == 1){
        Segment* sg = segments[start_address];
        std::size_t seg_end_addr = start_address + (sg->alignment * sg->num_words);
        addresses.erase(seg_end_addr);
        delete sg;
        segments.erase(start_address);
    }
}


// returns segment with start_address <= address < end_address, looks in segment allocated 
// by the transaction and the ones already in the STM
Segment* DualStm::findSegment(std::size_t address, Transaction* tx){
    Segment* sg = tx->findSegment(address);
    // not allocated by the current transaction, so it should be allocated in STM
    if (sg == NULL){
        auto it = addresses.upper_bound(address);
        std::size_t start_addr = it -> second;
        assert((address >= start_addr) == true);
        sg = segments[start_addr];
    }
    return sg;
}

// Begin a new transaction on the given shared memory region. Adds transaction to the
// batcher
Transaction* DualStm::begin(bool is_read_only){
    Transaction* tx = batcher -> enter(is_read_only);
    return tx;
}


// Read operation in a transaction
// source is the start address
// target is output buffer that has to be written
// size: length to copy in bytes
// Returns: true: the transaction can continue, false: the transaction has aborted
bool DualStm::read(Transaction* tx, void const * source, std::size_t size, void* target){
    std::size_t addr = reinterpret_cast<std::size_t>(source);
    Segment* sg = findSegment(addr, tx);
    std::size_t start_word_idx = (addr - sg->start_address) / alignment;
    std::size_t num_words = size / alignment;
    bool can_continue = sg->read(start_word_idx, num_words, tx, target);
    if(!can_continue){
        batcher -> leave(tx);
    }
    return can_continue;
}


// Write operation in the transaction, source in a private region and target in the shared region.
// source is the input buffer
// size: length to copy in bytes
// target: start address
// Returns: true: the transaction can continue, false: the transaction has aborted
bool DualStm::write(Transaction* tx, void const* source, std::size_t size, void * target){
    std::size_t addr = reinterpret_cast<std::size_t>(target);
    Segment* sg = findSegment(addr, tx);
    std::size_t start_word_idx = (addr - sg->start_address) / alignment;
    std::size_t num_words = size / alignment;
    bool can_continue = sg -> write(start_word_idx, num_words, tx, source);
    if(!can_continue){
        batcher -> leave(tx);
    }
    return can_continue;
}


// allocates new segment, increments atomically end_address and adds the segment to the allocated 
// segments in the transaction. If at the end the transaction is committed, the allocated segments are
// added to the STM by the batcher at the end of an epoch 
bool DualStm::alloc(Transaction* tx, std::size_t size, void ** target){
    std::size_t start_address = std::atomic_fetch_add(&end_address, size);
    DEBUG_MSG("Allocated segment at address: " << start_address);
    std::size_t num_words = size / alignment;
    Segment* segment = new Segment(alignment, num_words, start_address);
    tx -> addSegment(segment, start_address);
    memcpy(target, &start_address, sizeof(void*));
    return true;
}


// adds start_address of segment (contained in target) to list of segments to free in transaction
bool DualStm::free(Transaction* tx, void* target){
    std::size_t seg_start_addr = reinterpret_cast<std::size_t>(target);
    tx->freed.push_back(seg_start_addr);
    return true;
}



// End the given transaction.
// returns true if transaction was committed, false if it was aborted
bool DualStm::end(Transaction* tx){
    bool committed = !tx->aborted;
    batcher->leave(tx);
    return committed;
}

