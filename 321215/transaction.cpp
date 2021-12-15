#include "transaction.hpp"
#include "segment.hpp"
#include "word.hpp"


// called at the end of an epoch for committed transaction
// update content of written words, reset control structure of read/written words
void Transaction::commit(){
    assert(aborted == false);
    for (auto it = written.begin(); it != written.end(); it++){
        it -> second -> updateWritten();
    }
    for (auto it = read.begin(); it != read.end(); it++){
        it -> second -> resetState();
    }
}

// reset control variables of written/read words
void Transaction::abort(){
    assert(aborted == true);
    for (auto it = written.begin(); it != written.end(); it++){
        it -> second -> resetState();
    }
    for (auto it = read.begin(); it != read.end(); it++){
        it -> second -> resetState();
    }
}