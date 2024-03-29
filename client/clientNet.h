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
    ClientNet();
    ~ClientNet();

    //return immediately, run in main thread.
    int Login(const std::string & myNickName,int waitTime = 500);
    int SendMessageTo(const std::string & toName, const std::string & message);
    int Logout();
    int Clear();

    inline ClientChatData * getChatDataPointer() {return &chatData;}

private:
    void listener_main_loop();

    //called in listener thread.
    void loginOK(const std::vector<std::string> & userlist_contain_header);
    void loginNickNameError();
    void oneUserAdded(const std::string & name);
    void oneUserDeleted(const std::string & name);
    void messageReceived(const std::string & name, const std::string & line);
    void messageError(const std::string & name);

public:
    //called everywhere
    int writeLog(const std::string & line);

    //do nothing and return -1 if user not found.
    int writeUserMessage(const std::string &name, const std::string & line);

public:
    //run in all thread.
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
    MyTimer pendingTimer;
    int socketFd = -1;

};

#endif //! CHATSYSTEM_CLIENTNET_H_