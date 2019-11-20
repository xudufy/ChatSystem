#include "clientChatData.h"
#include "clientNet.h"


using namespace std;

int main()
{
    
    cerr<<"Usage: login <nickname>\n"
    "  send <to_name> <message>\n"
    "  logout\n"
    "  clear\n"
    "  exit\n"<<endl;

    ClientNet netcore;
    ClientChatData * dat = netcore.getChatDataPointer();

#define defCB(A) netcore. A ## CB = [dat](){ \
        cerr<<#A<<"CB has been called"<<endl;\
        lock_guard<mutex> lock(dat->logsmutex);\
        if (!dat->logs.empty()) { \
        cerr<<dat->logs.back()<<endl;\
        }\
    }

    defCB(connectionError);
    defCB(loginOK);
    defCB(loginError);
    defCB(oneUserAdded);
    defCB(oneUserDeleted);
    defCB(messageReceived);
    defCB(messageError);

    while (true){
        std::string cmd;
        std::getline(cin, cmd);
        if(cmd.size()<3) continue;
        if(cmd.back()=='\n') cmd.pop_back();
        if(cmd == "exit") break;
        std::vector<string> cols;
        cols.clear();
        size_t la = 0;
        for (size_t i = 0; i<cmd.size(); i++) {
            if (cmd[i]==' '){
                cols.push_back(cmd.substr(la, i-la));
                la = i+1;
            }
        }
        if (la<cmd.size()) {
            cols.push_back(cmd.substr(la));
        }

        if (cols[0]=="login" && cols.size()>=2) {
            netcore.Login(cols[1]);
        } else if (cols[0]=="send" && cols.size()>=3) {
            netcore.SendMessageTo(cols[1], cols[2]);
        } else if (cols[0]=="logout") {
            netcore.Logout();
        } else if (cols[0]=="clear") {
            netcore.Clear();
        }
    }

    return 0;    
}