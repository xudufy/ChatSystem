#ifndef SERVER_SERVERNET_H_
#define SERVER_SERVERNET_H_

#include "../common.h"

#include <sys/epoll.h>

class ServerNet {
public:
  ServerNet();
  ~ServerNet() = default;

  //join the thread and clear all fd at the endpoint.
  int ListenerLoop();

private:
  int handlerLoop();

private:
  int epfd=-1, listenfd=-1, stdinfd=STDIN_FILENO;
  int maxListenQueue = 10;
  std::thread *handler = NULL;
  std::atomic<bool> shouldStop;//this is for handler notify listener loop.

  //access only in handler thread, so no need for mutex. 
  //But semantically, any modify to this three data structures must be atomic as a whole, because of the redundence.
  std::unordered_map<std::string, int> name2Fd;
  std::unordered_map<int, std::string> fd2name;
  std::unordered_map<std::string, std::string> name2remainCmd;
}

#endif //! SERVER_SERVERNET_H_