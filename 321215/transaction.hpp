#ifndef TRANSACTION_h
#define TRANSACTION_h

#include "segment.hpp"
#include <map>
#include <vector>
#include "debug.hpp"

class Segment;
class Word;

class Transaction{

    public:
        bool has_written = false;

        // allocated[start_address] returns the corresponding Segment
        std::map<std::size_t, Segment*> allocated;

        // allocated_end_addresses[end_address] returns the start_address of the corresponding
        // Segment
        std::map<std::size_t, size_t> allocated_end_addresses;

        // start address of freed vectors
        std::vector<std::size_t> freed;

        bool is_read_only;

        // identifier of the transaction, unique for each transaction in one epoch
        std::size_t tr_num;

        std::vector<Word*> accessed;

        bool aborted = false;


        void addSegment(Segment* segment, std::size_t start_address){
            allocated[start_address] = segment;
            std::size_t end_address = start_address + (segment->num_words * segment->alignment);
            allocated_end_addresses[end_address] = start_address;
        }

        // return Segment that has start_addr < address < end_address
        // NULL if the transaction did not allocate such Segment
        Segment* findSegment(std::size_t addr){
            // finds lowest end address that is greater than addr
            auto iterator = allocated_end_addresses.upper_bound(addr);
            if (iterator == allocated_end_addresses.end()){
                return NULL;
            }
            std::size_t start_addr = iterator -> second;


            if (addr >= start_addr){
                return allocated[start_addr];
            }
            else{
                return NULL;
            }

        }

        Transaction(bool is_read_only, std::size_t tr_num): 
            is_read_only(is_read_only), tr_num(tr_num){};

        // if transaction was committed don't destroy allocated segments, as they are added to the STM
        // if transaction was not committed (aborted = true), destroy allocated segments
        // do not destroy written words, because some of them may be already allocated in the STM
        // and the ones that are not are destroyed with allocated segments
        ~Transaction(){
            DEBUG_MSG("Inside destructor of transaction: " << tr_num);
            if (aborted){
                for (auto it = allocated.begin(); it != allocated.end(); it++){
                    delete it -> second;
                }
            }
        }

};

#endif