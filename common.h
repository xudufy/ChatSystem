#ifndef CHATSYSTEM_COMMON_H_
#define CHATSYSTEM_COMMON_H_

#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>	

#include <iostream>
#include <mutex>
#include <thread>
#include <functional>
#include <map>
#include <string>
#include <atomic>
#include <vector>
#include <deque>

#ifdef NDEBUG
    #define NPNX_LOG(A) ((void)0)
#else
    #define NPNX_LOG(S,A) do { \
        std::cerr<<(S)<<":"<<#A<<":"<<(A)<<endl;\
    } while(0)
#endif

#ifdef NDEBUG
    #define NPNX_ASSERT(A) ((void)0)
#else
    #define NPNX_ASSERT(A) do{if(!(A)){std::cerr<<"ASSERTION FAILED:"<<#A<<" "<<endl;}} while(0)
#endif

#ifndef SERVER_IP
#define SERVER_IP "127.0.0.1"
#endif

#ifndef SERVER_PORT
#define SERVER_PORT 64168
#endif

#endif //! CHATSYSTEM_COMMON_H_