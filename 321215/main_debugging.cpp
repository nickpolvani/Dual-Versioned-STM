#include "debug.hpp"
//#define DEBUGGING
#ifdef DEBUGGING

#include "tm.hpp"

std::string convertToString(char* a, int size);

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

int main(){
    shared_t mem = tm_create(10, 1);
    void* first_addr = tm_start(mem);
    DEBUG_MSG(reinterpret_cast<std::size_t>(first_addr));
    tx_t tx = tm_begin(mem, false);
    char* target = new char[8];
    bool can_continue = tm_read(mem, tx, first_addr, 8, target);
    std::string read = convertToString(target, 8);
    DEBUG_MSG("Read bytes are:" << read);
    std::string input = "abc";
    char const * source = input.c_str();
    can_continue = tm_write(mem, tx, source, 3, first_addr);
    can_continue = tm_read(mem, tx, first_addr, 8, target);
    read = convertToString(target, 8);
    DEBUG_MSG("Read bytes are:" << read);
    bool committed = tm_end(mem, tx);
    DEBUG_MSG("Transaction committed:" << committed);
}

#endif