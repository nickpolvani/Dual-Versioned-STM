#ifndef DEBUG_H
#define DEBUG_H

//#define DEBUG

#ifdef DEBUG
#include <iostream>
#include <mutex>
static std::mutex debug_mutex;
#define DEBUG_MSG(str) do {debug_mutex.lock(); std::cout << str << "\n"; debug_mutex.unlock();} while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif


#endif