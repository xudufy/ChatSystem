#ifndef CHATSYSTEM_CLIENTCHATDATA_H_
#define CHATSYSTEM_CLIENTCHATDATA_H_

#include "../common.h"

class ClientChatData{
public:
  std::mutex msgmutex, logsmutex, mynamemutex;
  std::unordered_map<std::string, std::string> userMessages;
  std::deque<std::string> logs;
  std::string myNickName;
};


#endif //! CHATSYSTEM_CLIENTCHATDATA_H_


