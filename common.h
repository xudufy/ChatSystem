#ifndef CHATSYSTEM_COMMON_H_
#define CHATSYSTEM_COMMON_H_

#include <iostream>

#ifdef NDEBUG
    #define NPNX_LOG(A) ((void)0)
#else
    #define NPNX_LOG(S,A) do { \
        std::cerr<<(S)<<":"<<#A<<":"<<(A)<<endl;\
    } while(0)
#endif


#endif //! CHATSYSTEM_COMMON_H_