#ifndef DUAL_STM_H
#define DUAL_STM_H

#include <map>
#include <cstddef>
#include <atomic>

class Segment;
class Batcher;
class Transaction;

// dual-versioned Software Transactional Memory
class DualStm{
    private:
        std::map<std::size_t, Segment*> segments;
        // addresses[seg_end_address] returns start address of corresponding segment
        std::map<std::size_t, std::size_t> addresses;
        std::atomic<std::size_t> end_address{1};

    public:
        std::size_t alignment;
        std::size_t size_first_segment;
        
        // thread batcher, makes threads execute concurrently in batches and creates points in time
        // where no thread is running
        Batcher* batcher;

        // Create (i.e. allocate + init) a new shared memory region, with one first allocated segment of
        // the requested size and alignment.
        // Initializes also the batcher
        DualStm(std::size_t size, std::size_t alignment);
        
        // deallocate all segments
        ~DualStm();

        // allocates new segment, increments atomically end_address and adds the segment to the allocated 
        // segments in the transaction. If at the end the transaction is committed, the allocated segments are
        // added to the STM by the batcher at the end of an epoch 
        bool alloc(Transaction* tx, std::size_t size, void ** target);

        // Get a pointer in shared memory to the first allocated segment of the shared memory region
        void* getHead();

        // adds segment to segments, invoked by the batcher at the end of one epoch
        // to add newly allocated segments by the committed transactions
        void addSegment(std::size_t start_addr, Segment* segment);


        // deletes segment with address at start address, if not already deleted.
        // invoked by the batcher at the end of one epoch
        void freeSegment(size_t start_address);

        // Get the size to the first allocated segment of the shared memory region.
        std::size_t getSize(){
            return size_first_segment;
        }

        // returns segment with start_address <= address < end_address, looks in segment allocated 
        // by the transaction and the ones already in the STM
        Segment* findSegment(std::size_t address, Transaction* tx);

        // Begin a new transaction on the given shared memory region. Adds transaction to the
        // batcher
        Transaction* begin(bool is_read_only);

        // Read operation in a transaction
        // source is the start address
        // target is output buffer that has to be written
        // size: length to copy in bytes
        //Returns: true: the transaction can continue, false: the transaction has aborted
        bool read(Transaction* tx, void const *  source, std::size_t size, void* target);

        // Write operation in the transaction, source in a private region and target in the shared region.
        // source is the input buffer
        // size: length to copy in bytes
        // target: start address
        // Returns: true: the transaction can continue, false: the transaction has aborted
        bool write(Transaction* tx, void const * source, std::size_t size, void * target);

        // adds start_address of segment (contained in target) to list of segments to free in transaction
        bool free(Transaction* tx, void* target);

        // End the given transaction.
        // returns true if transaction was committed, false if it was aborted
        bool end(Transaction* tx);
        

        // used for debugging, checks that all the words have their state set
        // back to the initial one
        void checkEpochEnd();


};

#endif