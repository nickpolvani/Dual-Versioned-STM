#include "segment.hpp"
#include "word.hpp"
#include "transaction.hpp"
#include <string.h>
#include <assert.h>

Segment::Segment(std::size_t i_alignment, std::size_t i_num_words, std::size_t i_start_address):
        alignment(i_alignment), num_words(i_num_words), start_address(i_start_address)
{
    words = new Word* [num_words];
    std::size_t addr = start_address;
    for (std::size_t i = 0; i < num_words; i++){
        words[i] = new Word(alignment, addr);
        addr += alignment;
    }
}

Segment::~Segment(){
    for(std::size_t i = 0; i < num_words; i++){
        delete words[i];
    }
    delete[] words;
}


bool Segment::read(std::size_t start_word_idx, std::size_t num_words, Transaction* tx, void* target){
    char* out_buffer = static_cast<char*>(target);
    for (std::size_t i = 0; i < num_words; i++){
        std::size_t offset = i * alignment;
        bool result = words[start_word_idx + i]->read(tx, out_buffer + offset);
        if (result == false){
            return false;
        }
    }
    return true;
}

bool Segment::write(std::size_t start_word_idx, std::size_t num_words, Transaction* tx, void const * source){
    char* in_buffer = new char[num_words * alignment];
    memcpy(in_buffer, source, num_words*alignment);
    for (std::size_t i = 0; i < num_words; i++){
        std::size_t offset = i * alignment;
        bool result = words[start_word_idx + i]->write(tx, in_buffer + offset);
        if (result == false){
            return false;
        }
    }
    return true;
}


void Segment::checkEpochEnd(){
    for (size_t i = 0; i < num_words; i++){
        Word * cur_word = words[i];
        if ((cur_word->last_tx_accessed != 0) || 
                (cur_word->accessed_by_many == true) || 
                (cur_word->written == true)){
            
            DEBUG_MSG("Word at address " << cur_word->addr << " has last_tx_accessed: " << cur_word->last_tx_accessed << " accessed by many: " << cur_word->accessed_by_many << " written: " << cur_word->written);
            exit(2);
        }
    }
}