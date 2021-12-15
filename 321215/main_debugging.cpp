//#define DEBUGGING
#ifdef DEBUGGING
#include "debug.hpp"

#include "tm.hpp"
#include <string>
#include <thread>
#include <vector>
#include "transaction.hpp"
#include <iostream>
#include <string.h>
#include <stdexcept>
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void handler(int sig);

void handler(int sig) {
  void *array[10];
  int size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}



size_t convertToInt(void const * a, size_t size);

void start_transaction(shared_t mem);

size_t readInt(shared_t mem, size_t init_addr, tx_t tx);

bool writeInt(shared_t mem, size_t init_addr, size_t val, tx_t tx);


// converts character array
// to size_t and returns it
size_t convertToInt(void const * a, size_t size){
    size_t i;
    memcpy(&i, a, size);  
    return i;
}



size_t readInt(shared_t mem, size_t init_addr, tx_t tx){
    void const* source = reinterpret_cast<void *>(init_addr); 
    
    void * target = new char[8];
    bool can_continue = tm_read(mem, tx, source, 8, target);
    Transaction* t = reinterpret_cast<Transaction *>(tx);
    if (can_continue){
        return reinterpret_cast<size_t>(target);
    }
    else{
        
        DEBUG_MSG("Transaction " << t->tr_num << " aborted when trying to read address " << init_addr);
        throw std::runtime_error("Transaction aborted");
    }
}


bool writeInt(shared_t mem, size_t init_addr, tx_t tx, size_t val){
    void * target = reinterpret_cast<void*>(init_addr);
    void *  source = new char[8];
    memcpy(source, &val, 8);
    void const * src = source;
    bool can_continue = tm_write(mem, tx, src, 8, target);
    Transaction* t = reinterpret_cast<Transaction *>(tx);
    if (can_continue){
        return can_continue;
    }
    else{
        DEBUG_MSG("Transaction " << t->tr_num << " aborted when trying to write address " << init_addr);
        throw std::runtime_error("Transaction aborted");
    }

}

// converts character array
// to string and returns it
void start_transaction(shared_t mem){
 
        tx_t tx = tm_begin(mem, false);
      
            size_t val = readInt(mem, 1, tx);
    
        Transaction * t = reinterpret_cast<Transaction*>(tx);
        
            bool can_continue = writeInt(mem, 1, tx, t->tr_num);
      
        
  
}

int main(){
    //signal(SIGSEGV, handler);   // install our handler
    shared_t mem = tm_create(64, 8);
    std::vector<std::thread*> threads;
    for (int i = 0; i < 10; i++){
        threads.push_back(new std::thread(start_transaction, mem));
    }
    
    for (auto thread : threads){
        thread -> join();
    }
}

#endif