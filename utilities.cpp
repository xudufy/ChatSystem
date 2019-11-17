#include "utilities.h"

void MyTimer::setTimer(int ms)
{
    timespec cur;
    clock_gettime(CLOCK_REALTIME, &cur);
    cur.tv_nsec+=ms*1000000;
    if (cur.tv_nsec>=1000000000) {
        cur.tv_sec+=(int)cur.tv_nsec/1000000000;
        cur.tv_nsec%=1000000000;
    }
    {
        std::lock_guard lock(objmutex);
        endTp = cur;
    }
}

bool MyTimer::TimerNotEnd() 
{
    timespec cur;
    clock_gettime(CLOCK_REALTIME, &cur);
    {
        std::lock_guard lock(objmutex);
        return (cur.tv_sec<endTp.tv_sec || 
        (cur.tv_sec==endTp.tv_sec && cur.tv_nsec<endTp.tv_nsec)); 
    }
}

std::string excapeMsg(const std::string &in)
{
    std::string out="";
    for(int i = 0; i<in.size(); i++) {
        switch (in[i]) {
            case ':': 
            case ';': 
            case '\\': 
                out+='\\';
                out+=in[i];
                break;
            default: out+=in[i];
                break;
        }
    }
    return out;
}

std::string unExcapeMsg(const std::string &in)
{
    std::string out="";
    for(int i=0; i<in.size(); i++){
        if (in[i]!='\\') {
            out+=in[i];
        } else if (i<in.size()-1 && in[i+1]=='\\') {
            out+=in[i];
            i++;
        }
    }
    return out;
}

std::string compositeMsg(const std::vector<std::string> & in)
{
    std::string out;
    for (int i = 0; i<in.size()-1; i++) {
        out+=excapeMsg(in[i]);
        out+=":";
    }
    out+=excapeMsg(in.back());
    out+=';';
    return out;
}

std::vector<std::string> deCompositeMsg(const std::string &in)
{
    std::vector<std::string> out;
    int lastsep = 0;
    bool escape = false;
    for (int i=0; i<in.size(); i++) {
        if (!escape) {
            if (in[i]=='\\') {
                escape=true;
            } else if (in[i]==':' || in[i]==';'){
                out.push_back(unExcapeMsg(in.substr(lastsep, i-lastsep)));
                lastsep = i+1;
            }
        } else {
            escape=false;
        }
    }

    return out;
}