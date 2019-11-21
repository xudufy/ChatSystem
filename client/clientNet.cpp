#include "clientNet.h"
#include <signal.h>

using namespace std;

ClientNet::ClientNet()
{
    status=NetStatus::LOGGED_OUT; 
}

ClientNet::~ClientNet()
{
    Clear();
}

int ClientNet::Login(const std::string & myNickName, int waitTime)
{
    if (status.load() == NetStatus::LOGOUT_PENDING) {
        Clear();
    }

    NetStatus temp = NetStatus::LOGGED_OUT;
    if (!status.compare_exchange_strong(temp, NetStatus::LOGIN_PENDING)) {
        cerr<<"warning: Login when not logged out."<<endl;
        return -1;
    }

    do {
        int soc = socket(AF_INET, SOCK_STREAM, 0);
        if (soc == -1) {
            writeLog("Failed: create socket.");
            break;
        }
        socketFd = soc;

        sockaddr_in serverAdd;
        serverAdd.sin_family = AF_INET;
        serverAdd.sin_addr.s_addr=inet_addr(SERVER_IP);
        serverAdd.sin_port = htons(SERVER_PORT);
        if (connect(socketFd, (sockaddr *)&serverAdd, sizeof(serverAdd))) {
            writeLog("Failed:connect to server");
            break;
        }

        string logincmd = compositeMsg({CMDHEAD_LOGIN,myNickName});
        if (send(socketFd, logincmd.c_str(), logincmd.size()*sizeof(char), 0)<0) {
            writeLog("Failed:send login message.");
            break;
        }

        pendingTimer.setTimer(waitTime);
        mainLoopThread = new std::thread([this](){this->listener_main_loop();});
        return 0;

    } while (0);
    
    Clear();
    writeLog("Connection Error when login.");
    if (loginErrorCB) loginErrorCB();    
    return 0;
}

int ClientNet::SendMessageTo(const std::string & toName, const std::string & line)
{
    if (status.load() != NetStatus::LOGGED_IN) {
        cerr<<"warning: send message while logged out"<<endl;
        return -1;
    }

    string msgline = "Me:"+line;
    if (writeUserMessage(toName, msgline)<0) {
        writeLog("You can't send message to offline user.");
        if (messageErrorCB) messageErrorCB();
    };
    
    string cmdline = compositeMsg({CMDHEAD_SEND, toName, line});
    if (send(socketFd, cmdline.c_str(), cmdline.size()*sizeof(char),0)<0) {
        status = NetStatus::LOGOUT_PENDING;
        writeLog("Connection Error Occured.");
        if (connectionErrorCB) connectionErrorCB();
        return -1;
    }

    return 0;
}

int ClientNet::Logout()
{
    if (status.load() != NetStatus::LOGGED_IN) {
        cerr<<"warning: logout while not logged in"<<endl;
        return -1;
    }

    string cmdline = compositeMsg({CMDHEAD_LOGOUT});
    if (send(socketFd, cmdline.c_str(), cmdline.size()*sizeof(char),0)<0) {
        status = NetStatus::LOGOUT_PENDING;
        writeLog("Connection error when logged out");
        if (connectionErrorCB) connectionErrorCB();
        return -1;
    }

    Clear();
    writeLog("Successfully logged out");

    return 0;
}

int ClientNet::Clear()
{
    status = NetStatus::LOGOUT_PENDING;

    if (mainLoopThread!=NULL && mainLoopThread->joinable()) {
        mainLoopThread->join();
        delete mainLoopThread;
        mainLoopThread = NULL;
    }

    if (socketFd!=-1) {
        string cmdline = compositeMsg({CMDHEAD_LOGOUT});
        shutdown(socketFd, SHUT_WR);
        char buf[4096];
        while (recv(socketFd, buf, 4096, 0)>0) {}
        close(socketFd);
        socketFd = -1;
    }

    {   //actually no need to lock because we joined the listener thread.
        lock_guard<mutex> lock(chatData.msgmutex);
        lock_guard<mutex> lock2(chatData.logsmutex);
        lock_guard<mutex> lock3(chatData.mynamemutex);
        chatData.userMessages.clear();
        chatData.logs.clear();
        chatData.myNickName.clear();
    }

    status = NetStatus::LOGGED_OUT;
    return 0;
}



void ClientNet::listener_main_loop()
{
    string cmdbuffer;

    //indecate whether the break of the loop should call connectionErrorCB.
    bool errorHappened = false; 
    
    while (true) {
        if (status==NetStatus::LOGOUT_PENDING) {
            break;
        }

        char buf[4096];
        int suc = 0;
        do {
            suc = recv(socketFd, buf, 4095, MSG_DONTWAIT);
            if (suc>0) {
                buf[suc] = '\0';
                cmdbuffer+=buf;
            }
        } while (suc == 4095);
        
        if (suc==0) {
            writeLog("Connection error: FIN received.");
            errorHappened=true;
            break;
        } else if (suc<0 && errno!=EAGAIN) {
            writeLog("Connection error on receiving.");
            errorHappened=true;
            break;
        } 

        bool breakflag = false;
        std::vector<std::string> cmds = separateMsg(cmdbuffer);
        for (auto cmd: cmds) {
            NPNX_LOG("received",cmd);
            std::vector<std::string> cols = deCompositeMsg(cmd);
            NPNX_ASSERT(cols.size()>0 && "cols is empty");
            if (status == NetStatus::LOGIN_PENDING) {
                if (cols[0] == CMDHEAD_LOGIN_OK) {
                    NPNX_ASSERT(cols.size()>=2 && "LOGIN_OK size not right");
                    loginOK(cols);
                } else if (cols[0] == CMDHEAD_LOGIN_FAILED_NICKNAME) {
                    loginNickNameError();
                    breakflag = true;
                    break;
                } else if (!pendingTimer.TimerNotEnd()) {
                    writeLog("Login Timeout.");
                    if (loginErrorCB) loginErrorCB();
                    breakflag = true;
                    break;
                }
            } else if (status == NetStatus::LOGGED_IN){
                if (cols[0] == CMDHEAD_USER_ADD){
                    NPNX_ASSERT(cols.size()==2 && "USER_ADD size not right");
                    oneUserAdded(cols[1]);
                } else if (cols[0] == CMDHEAD_USER_DELETE) {
                    NPNX_ASSERT(cols.size()==2 && "USER_DELETE size not right");
                    oneUserDeleted(cols[1]);
                } else if (cols[0] == CMDHEAD_SEND_FAILED) {
                    NPNX_ASSERT(cols.size()==2 && "SEND_FAILED size not right");
                    messageError(cols[1]);
                } else if (cols[0] == CMDHEAD_RECV) {
                    NPNX_ASSERT(cols.size()==3 && "RECV size not right");
                    messageReceived(cols[1], cols[2]);
                }
            }
        }
        if (breakflag) break;

        std::this_thread::sleep_for(std::chrono::duration<int, std::ratio<1,1000>>(100));
    }

    status = NetStatus::LOGOUT_PENDING;
    if (errorHappened) {
        writeLog("Unexpected Connection Error");
        if (connectionErrorCB) connectionErrorCB();
    }
}

void ClientNet::loginOK(const std::vector<std::string> & fake_userlist)
{
    {
        lock_guard<mutex> lock(chatData.msgmutex);
        lock_guard<mutex> lock2(chatData.logsmutex);
        lock_guard<mutex> lock3(chatData.mynamemutex);
        chatData.myNickName = fake_userlist[1];
        chatData.logs.clear();
        chatData.userMessages.clear();
        //myself is a valid msg receiver, so start from 1.
        for (size_t i = 1; i<fake_userlist.size(); i++) {
            if (chatData.userMessages.count(fake_userlist[i])==0){
                chatData.userMessages[fake_userlist[i]] = {};
            }
        }
    }
    status=NetStatus::LOGGED_IN;
    if (loginOKCB) loginOKCB();
}

void ClientNet::loginNickNameError()
{
    writeLog("Nickname has been used.");
    if (loginErrorCB) loginErrorCB();
}

void ClientNet::oneUserAdded(const std::string & name)
{
    writeLog(name+" logged in.");
    {
        lock_guard<mutex> lock(chatData.msgmutex);
        if (chatData.userMessages.count(name)==0){
            chatData.userMessages[name] = {};
        } else {
            chatData.userMessages[name].push_back(name+" logged in.");
        }
    }

    if (oneUserAddedCB) oneUserAddedCB();
}

void ClientNet::oneUserDeleted(const std::string & name)
{
    writeLog(name+" logged out.");
    {
        lock_guard<mutex> lock(chatData.msgmutex);
        if (chatData.userMessages.count(name)==1){
            chatData.userMessages[name].clear();
            chatData.userMessages.erase(name);
        }
    }

    if (oneUserDeletedCB) oneUserDeletedCB();
}

void ClientNet::messageReceived(const std::string & name, const std::string & message)
{
    writeUserMessage(name, name+":"+message);
    writeLog("New message: " + name);
    if (messageReceivedCB) messageReceivedCB();
}

void ClientNet::messageError(const std::string & name)
{
    writeLog("Server can't send message to "+name);
    if (messageErrorCB) messageErrorCB();
}

int ClientNet::writeLog(const std::string & line)
{
    lock_guard<mutex> lock(chatData.logsmutex);
    chatData.logs.push_back(line);
    int n = chatData.logs.size();
    if (n>20) {
        for (size_t i = 0; i<n-10; i++) {
            chatData.logs.pop_front();
        }
    }
    return 0; 
}

int ClientNet::writeUserMessage(const std::string &name, const std::string & line)
{
    lock_guard<mutex> lock(chatData.msgmutex);
    if (chatData.userMessages.count(name)==0) {
        return -1;
    } else {
        chatData.userMessages[name].push_back(line);
        int n = chatData.userMessages[name].size();
        if (n>100) {
            for (size_t i = 0; i<(size_t)n-40; i++) {
                chatData.userMessages[name].pop_front();
            }
        }
    }

    return 0;
}

