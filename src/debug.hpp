#ifndef DEBUG_HPP
#define DEBUG_HPP

// Comment/uncomment this line to enable/disable debug output
//#define DEBUG_MODE

#ifdef DEBUG_MODE
    #include <iostream>
    #define DEBUG_LOG(x) std::cout << x
#else
    #define DEBUG_LOG(x)
#endif

#endif
