#ifndef CHATSYSTEM_CLIENTNET_H_
#define CHATSYSTEM_CLIENTNET_H_

#include "../common.h"
#include "../utilities.h"
#include "clientChatData.h"

enum class NetStatus:int{
    LOGGED_OUT,
    LOGIN_PENDING,
    LOGGED_IN,
    LOGOUT_PENDING
};

class ClientNet {
public:
    explicit ClientNet();

    //return immediately, run in main thread.
    int Login(const std::string & myNickName);
    int SendMessageTo(const std::string & toName, const std::string & message);
    int Logout();
    int Clear();

    inline ClientChatData * getChatDataPointer() {return &chatData;}

private:
    void listener_main_loop();

    //called in listener thread.
    void connectionError();
    int loginOK();
    int loginNickNameError();
    void oneUserAdded(const std::string & name);
    void oneUserDeleted(const std::string & name);
    void messageReceived(const std::string & name, const std::string & message);
    void messageError(const std::string & name);

    //called everywhere
    int checkUserName(const std::string & name);
    int writeLog(const std::string & line);
    
public:
    //run in listener thread.
    std::function<void(void)> 
        connectionErrorCB = NULL,
        loginOKCB = NULL,
        loginErrorCB = NULL,
        oneUserAddedCB = NULL,
        oneUserDeletedCB = NULL,
        messageReceivedCB = NULL,
        messageErrorCB = NULL;
        

private:
    ClientChatData chatData;
    std::thread *mainLoopThread = NULL;
    std::atomic<NetStatus> status;
    MyTimer timeoutTimer;
    int socketFd;

};

#endif //! CHATSYSTEM_CLIENTNET_H_