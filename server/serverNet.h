#ifndef SERVER_SERVERNET_H_
#define SERVER_SERVERNET_H_

#include "../common.h"

//one thread server.
class ServerNet {
public:
  int ListenerLoop();

private:
  //return -1 if we need to shutdown the server.
  //return 0 if connected or connection error(no need to handle).
  int acceptConnection();

  void closeClientGently(int fd);
  void closeClientViolently(int fd);
  void oneLogOut(int fd);
  void oneLogIn(int fd, const std::string & nickname);
  void oneSendMessage(int in_fd, const std::string & to_name, const std::string & msg);
  void broadcast(const std::string & cmd, int exceptfd = -1);

  int epfd=-1, listenfd=-1, stdinfd=STDIN_FILENO;
  std::unordered_map<int, std::string> openedfd; 
  
  int maxListenQueue = 10;
  int numAcceptError = 0;

  //redundant login sessions.
  std::unordered_map<std::string, int> name2Fd;
  std::unordered_map<int, std::string> fd2name;
  
  
};

#endif //! SERVER_SERVERNET_H_