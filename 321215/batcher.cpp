#include "batcher.hpp"
#include "transaction.hpp"
#include "dual_stm.hpp"
#include "word.hpp"
#include "debug.hpp"

Batcher::~Batcher(){
    for (auto b : blocked){
        delete b;
    }
    for (auto tx : committed_transactions){
        delete tx;
    }
    for (auto tx : aborted_transactions){
        delete tx;
    }
}


Transaction* Batcher::enter(bool is_read_only){
    std::unique_lock<std::mutex> lock(mutex);
    if (remaining == 0){
        remaining = 1;
        Transaction * tx = new Transaction(is_read_only, 1);
        return tx;
    }
    else{
        b_thread t;
        t.thread_id = std::this_thread::get_id();
        blocked.push_back(&t);
        Transaction * tx = new Transaction(is_read_only, blocked.size());
        while(!t.awake){
            cv.wait(lock);
        }
        return tx;
    }
}

void Batcher::leave(Transaction* tx){
    std::unique_lock<std::mutex> lock(mutex);
    remaining --;
    DEBUG_MSG("Transaction " << tx->tr_num  << " is leaving batcher. Aborted: " << tx->aborted);
    if(tx -> aborted == false){
        committed_transactions.push_back(tx);
    }
    else{
        // keep track of aborted transactions that modified the state of some words
        if (tx->accessed.size() > 0){
            aborted_transactions.push_back(tx);
        }
        // didn't modify anything, deleting..
        else{
            DEBUG_MSG("Transaction " << tx->tr_num << " didn't modify anything. Destroying...");
            delete tx;
        }
    }
    if (remaining == 0){
        DEBUG_MSG("Ending epoch " << counter);
        onEpochEnd();
        counter ++;
        remaining = blocked.size();
        if (remaining > 0){
            DEBUG_MSG("Beginning epoch " << counter << " with " << blocked.size() << " transactions");
            for (std::size_t i = blocked.size(); i > 0; i--){
                blocked.back() -> awake = true;
                blocked.pop_back(); 
            }
            cv.notify_all();
        }
        else{
            DEBUG_MSG("No transactions waiting to enter batcher");
        }
    }
}

// 1) add segments that were allocated by committed transactions to the STM
// 2) update state of words that were accessed in this epoch 
//    (for committed/non-committed transactions, on/off the STM)  
// 3) free (on STM) segments that were freed by committed transactions
// 4) delete all transactions and empty committed/aborted arrays
void Batcher::onEpochEnd(){
    DEBUG_MSG("Number comitted transactions: " << committed_transactions.size());
    DEBUG_MSG("Number aborted transactions: " << aborted_transactions.size());

    // add allocated segments
    for (Transaction* tx : committed_transactions){
        for(auto it = tx->allocated.begin(); it != tx->allocated.end(); it++){
            std::size_t start_addr = it->first;
            Segment* sg = it->second;
            stm->addSegment(start_addr, sg);
        }
    }

    // update state of accessed words
    for(Transaction* tx : committed_transactions){
        //DEBUG_MSG("Updating state for " << tx->accessed.size() << " words accessed by committed tx: " << tx->tr_num << " has written: " << tx->has_written);
        for(Word* word : tx->accessed){
            word -> updateState();
        }
    }
    for(Transaction* tx : aborted_transactions){
       // DEBUG_MSG("Updating state for " << tx->accessed.size() << " words accessed by aborted tx: " << tx->tr_num << " has written: " << tx->has_written);
        for(Word* word: tx->accessed){
            word -> updateState();
        }
    }

    // free segments, delete transactions
    for (Transaction* tx: committed_transactions){
        for (std::size_t start_addr : tx->freed){
            DEBUG_MSG("Freeing segment from committed");
            stm->freeSegment(start_addr);
        }
        delete tx;
    }
    for (Transaction* tx: aborted_transactions){
        for (std::size_t start_addr : tx->freed){
            DEBUG_MSG("Freeing segment from aborted");
            stm->freeSegment(start_addr);
        }
        delete tx;
    }
    // empty arrays
    committed_transactions.clear();
    aborted_transactions.clear();
}