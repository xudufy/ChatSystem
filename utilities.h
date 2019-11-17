#ifndef CHATSYSTEM_UTILITIES_H_
#define CHATSYSTEM_UTILITIES_H_

#include "common.h"
#include "time.h"

class MyTimer {
public:
    void setTimer(int ms);
    bool TimerNotEnd();

private:
    std::mutex objmutex; 
    timespec endTp;

};

std::string excapeMsg(const std::string &in);

std::string unExcapeMsg(const std::string &in);

std::string compositeMsg(const std::vector<std::string> & in);

std::vector<std::string> deCompositeMsg(const std::string &in);

std::vector<std::string> separateMsg(std::string & in);

#endif //! CHATSYSTEM_UTILITIES_H_