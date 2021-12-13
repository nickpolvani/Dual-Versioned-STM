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
void Word::addToAccessSet(Transaction* tx){
    std::unique_lock lock(access_set_mutex);
    if (last_tx_accessed == 0){
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



bool Word::read(Transaction* tx, void* target){
    if (tx -> is_read_only){
        if(is_copy_a_readable){
            memcpy(target, copy_a, alignment);
        }else{
            memcpy(target, copy_b, alignment);
        }
        return true;
    }
    // tx is not read_only
    else{

        if (written){
            if (last_tx_accessed == tx->tr_num){
                // read writable copy into target
                if(!is_copy_a_readable){
                    memcpy(target, copy_a, alignment);
                }else{
                    memcpy(target, copy_b, alignment);
                }
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
            if(is_copy_a_readable){
                memcpy(target, copy_a, alignment);
            }else{
                memcpy(target, copy_b, alignment);
            }
            addToAccessSet(tx);
            return true;
        }

    }
}


bool Word::write(Transaction* tx, void const* source){
    if (written){
        if (last_tx_accessed == tx -> tr_num){
            // write content at source into the writable copy
            if(!is_copy_a_readable){
                memcpy(copy_a, source, alignment);
            }else{
                memcpy(copy_b, source, alignment);
            }
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
            if(!is_copy_a_readable){
                memcpy(copy_a, source, alignment);
            }else{
                memcpy(copy_b, source, alignment);
            }
            addToAccessSet(tx);
            written = true;
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