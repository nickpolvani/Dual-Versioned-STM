#ifndef BATCHER_H
#define BATCHER_H

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <atomic>

class Transaction;
class DualStm;

class Batcher{
    private:
        std::condition_variable cv;

        struct b_thread
        {
            std::thread::id thread_id;
            bool awake = false;
        };
        
        DualStm* stm;

        std::mutex mutex;

        // counter for current epoch number
        std::size_t counter = 0;
        // remaining threads in the current epoch
        std::size_t remaining = 0;
        // threads waiting to start
        std::vector<b_thread*> blocked;

        std::vector<Transaction*> committed_transactions;

        std::vector<Transaction*> aborted_transactions;

        // 1) add segments that were allocated by committed transactions to the STM
        // 2) update state of words that were accessed in this epoch 
        //    (for committed/non-committed transactions, on/off the STM)  
        // 3) free (on STM) segments that were freed by committed transactions
        // 4) delete all transactions
        void onEpochEnd();

    public:
        Batcher(DualStm* i_dual_stm):stm(i_dual_stm){};

        // transaction begins
        Transaction* enter(bool is_read_only);

        // transaction ends
        void leave(Transaction * tx);

};


#endif