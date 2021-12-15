#ifndef SEGMENT_H
#define SEGMENT_H
#include <cstddef>

class Word;
class Transaction;

class Segment{

    private:
        Word** words;

    public:
        std::size_t alignment;
        std::size_t num_words;
        std::size_t start_address;

        Segment(std::size_t i_alignment, std::size_t i_num_words, std::size_t i_start_address);

        ~Segment();

        // Returns: true: the transaction can continue, false: the transaction has aborted
        bool read(std::size_t start_word_idx, std::size_t num_words, Transaction* tx, void* target);
        
        // Returns: true: the transaction can continue, false: the transaction has aborted
        bool write(std::size_t start_word_idx, std::size_t num_words, Transaction* tx, void const* source);
    
        
        void checkEpochEnd();

};

#endif