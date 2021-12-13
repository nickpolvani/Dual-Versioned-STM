#ifndef DEBUG_H
#define DEBUG_H

//#define DEBUG

#ifdef DEBUG
#include <iostream>
#define DEBUG_MSG(str) do { std::cout << str << "\n"; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif


#endif