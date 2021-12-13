#include "word.hpp"
#include "transaction.hpp"
#include <string.h>

Word::Word(std::size_t i_alignment): 
alignment(i_alignment){

    copy_a = new char[alignment]; 
    copy_b = new char[alignment]; 
    memset(copy_a, 0, alignment);
    memset(copy_b, 0, alignment);
}

// add transaction to "access set" if not already in
void Word::addToAccessSet(Transaction* tx, bool writing){
    std::unique_lock<std::mutex> lock(access_set_mutex);
    if (writing){
        written = true;
    }
    if (last_tx_accessed == 0){  // first transaction accessing       
        last_tx_accessed = tx -> tr_num;
        tx -> accessed.push_back(this);
    }
    else{
        if (last_tx_accessed != tx->tr_num){
            accessed_by_many = true;
            last_tx_accessed = tx -> tr_num;
            tx -> accessed.push_back(this);
        }
    }
}


// if readable=True read readable copy
// else read the writable one
void Word::readCopy(void* target, bool readable){
    if (readable){
        if(is_copy_a_readable){ // read readable copy
            memcpy(target, copy_a, alignment);
        }
        else{
            memcpy(target, copy_b, alignment);
        }
    }
    else{   // read writable copy
        if(!is_copy_a_readable){
            memcpy(target, copy_a, alignment);
        }
        else{
            memcpy(target, copy_b, alignment);
        }
    }
}


// write content of buffer source into writable copy
void Word::writeCopy(void const* source){
    if(!is_copy_a_readable){
        memcpy(copy_a, source, alignment);
    }else{
        memcpy(copy_b, source, alignment);
    }
}


bool Word::read(Transaction* tx, void* target){
    if (tx -> is_read_only){
        readCopy(target, true);
        return true;
    }
    // tx is not read_only
    else{
        if (written){
            if (last_tx_accessed == tx->tr_num){
                // read writable copy into target
                readCopy(target, false);
                return true;
            }
            else{
                tx->aborted = true;
                return false;
            }
        }
        // word not written, non read-only transaction
        else{
            // read readable copy into target
            addToAccessSet(tx, false);
            readCopy(target, true);
            return true;
        }

    }
}


bool Word::write(Transaction* tx, void const* source){
    if (written){
        if (last_tx_accessed == tx -> tr_num){
            // write content at source into the writable copy
            writeCopy(source);
            return true;
        }
        else{
            tx -> aborted = true;
            return false;
        }
    }
    else{
        if (accessed_by_many){
            tx -> aborted = true;
            return false;
        }
        else{
            // write content at source into the writable copy
            addToAccessSet(tx, true);
            writeCopy(source);
            return true;
        }
    }
}


// called at the end of an epoch to update readable/writable copy
// reset the access set and written condition, swap readable/writable copy if written
void Word::updateState(){
    if (written){
        is_copy_a_readable = !is_copy_a_readable;
    }
    written = false;
    last_tx_accessed = 0;
    accessed_by_many = false;
}