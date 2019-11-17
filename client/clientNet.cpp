#include "clientNet.h"

using namespace std;

ClientNet::ClientNet()
{
    status=NetStatus::LOGGED_OUT; 
}

int ClientNet::Login(const std::string & myNickName)
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

        string logincmd = compositeMsg({"LOGIN",myNickName});
        if (send(socketFd, logincmd.c_str(), logincmd.size()*sizeof(char), 0)<0) {
            writeLog("Failed:send login message.");
            break;
        }

        mainLoopThread = new std::thread([this](){this->listener_main_loop();});
        return 0;

    } while (0);
    
    if (loginErrorCB) loginErrorCB();
    Clear();
    
}

int ClientNet::SendMessageTo(const std::string & toName, const std::string & message)
{
}

int ClientNet::Logout()
{
}

int ClientNet::Clear()
{
}



void ClientNet::listener_main_loop()
{
}


//called in listener thread.
void ClientNet::connectionError()
{
}

int ClientNet::loginOK()
{
}

int ClientNet::loginNickNameError()
{
}

void ClientNet::oneUserAdded(const std::string & name)
{
}

void ClientNet::oneUserDeleted(const std::string & name)
{
}

void ClientNet::messageReceived(const std::string & name, const std::string & message)
{
}

void ClientNet::messageError(const std::string & name)
{
}


//called everywhere
int ClientNet::checkUserName(const std::string & name)
{
}

int ClientNet::writeLog(const std::string & line)
{
}

