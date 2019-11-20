#ifndef CHATSYSTEM_COMMON_H_
#define CHATSYSTEM_COMMON_H_

#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<errno.h>	

#include <iostream>
#include <mutex>
#include <thread>
#include <functional>
#include <map>
#include <unordered_map>
#include <string>
#include <atomic>
#include <vector>
#include <deque>

#ifdef NDEBUG
    #define NPNX_LOG(A) ((void)0)
#else
    #define NPNX_LOG(S,A) do { \
        std::cerr<<(S)<<":"<<#A<<":"<<(A)<<std::endl;\
    } while(0)
#endif

#ifdef NDEBUG
    #define NPNX_ASSERT(A) ((void)0)
#else
    #define NPNX_ASSERT(A) do{if(!(A)){std::cerr<<"ASSERTION FAILED:"<<#A<<" "<<std::endl;}} while(0)
#endif

#ifndef SERVER_IP
#define SERVER_IP "127.0.0.1"
#endif

#ifndef SERVER_PORT
#define SERVER_PORT 64168
#endif

//custom protocol header
#define CMDHEAD_LOGIN "LOGIN"
#define CMDHEAD_LOGIN_OK "LOGIN_OK"
#define CMDHEAD_LOGIN_FAILED_NICKNAME "LOGIN_FAILED_NICKNAME"
#define CMDHEAD_USER_ADD "USER_ADD"
#define CMDHEAD_USER_DELETE "USER_DELETE"
#define CMDHEAD_SEND "SEND"
#define CMDHEAD_SEND_FAILED "SEND_FAILED"
#define CMDHEAD_RECV "RECV"
#define CMDHEAD_LOGOUT "LOGOUT"

#endif //! CHATSYSTEM_COMMON_H_