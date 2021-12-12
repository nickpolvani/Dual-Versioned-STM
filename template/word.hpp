#ifndef WORD_H
#define WORD_H

#include <atomic>

class Transaction;

class Word{
    private:
    
        char * copy_a;
        char * copy_b;


    public:
        // if is_copy_a_readable = true, then copy_a is readable, otherwise copy_b
        bool is_copy_a_readable = true;

        std::size_t alignment;
        // identifier of the last transaction that accessed the word 
        // valid identifiers range from 1 to n
        std::atomic_ulong last_tx_accessed{0};

        // whether the word was read/written by multiple transactions
        std::atomic_bool accessed_by_many{false};

        std::atomic_bool written{false};

        explicit Word(std::size_t i_alignment);

        ~Word(){
            delete[] copy_a;
            delete[] copy_b;
        }

        bool read(Transaction* tx, void* target);

        bool write(Transaction* tx, void const* source);

        void addToAccessSet(Transaction* tx);

        // called at the end of an epoch to update readable/writable copy
        // reset the access set and written condition
        void updateState();

};


#endif