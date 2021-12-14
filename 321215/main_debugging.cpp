//#define DEBUGGING
#ifdef DEBUGGING
#include "debug.hpp"

#include "tm.hpp"
#include <string>
#include <thread>
#include <vector>
#include "transaction.hpp"

std::string convertToString(char* a, int size);


std::string convertToChar(char* a, std::string s);
void start_transaction(shared_t mem);

// converts character array
// to string and returns it
std::string convertToString(char* a, int size){
    int i;
    std::string s = "";
    for (i = 0; i < size; i++) {
        s = s + a[i];
    }
    return s;
}

std::string convertToChar(char * a, std::string s){
    std::size_t i;
    for (i = 0; i < s.size(); i++) {
       a[i] = s[i];
    }
    return s;
}


void start_transaction(shared_t mem){
    while(true){
        void* first_addr = tm_start(mem);
        //DEBUG_MSG(reinterpret_cast<std::size_t>(first_addr));
        tx_t tx = tm_begin(mem, false);
        char* target = new char[8];
        bool can_continue = tm_read(mem, tx, first_addr, 8, target);
        if (!can_continue){
            std::cout<<(reinterpret_cast<Transaction*>(tx))->tr_num << " can't continue\n";
            break;
        }
        std::string read = convertToString(target, 8);
        DEBUG_MSG("Read bytes are:" << read);
        std::string input = "";
        for (int i = 0; i < 4; i++){
            input += std::to_string((reinterpret_cast<Transaction*>(tx))->tr_num);
        }
        char * source = new char[8];
        convertToChar(source, input);
        char * const src = source;
        can_continue = tm_write(mem, tx, src, 4, first_addr);
        if (!can_continue){
            std::cout<<(reinterpret_cast<Transaction*>(tx))->tr_num << " can't continue\n";
            break;
        }
        can_continue = tm_read(mem, tx, first_addr, 8, target);
        if (!can_continue){
            std::cout<<(reinterpret_cast<Transaction*>(tx))->tr_num << " can't continue\n";
            break;
        }
        read = convertToString(target, 8);
        DEBUG_MSG("Read bytes are:" << read);
        bool committed = tm_end(mem, tx);
        DEBUG_MSG("Transaction " << (reinterpret_cast<Transaction*>(tx))->tr_num  <<   " committed: " << committed);
    }

}

int main(){
    shared_t mem = tm_create(20, 1);
    std::vector<std::thread*> threads;
    for (int i = 0; i < 27; i++){
        threads.push_back(new std::thread(start_transaction, mem));
    }
    
    for (auto thread : threads){
        thread -> join();
    }
}

#endif