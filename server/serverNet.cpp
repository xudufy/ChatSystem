#include "serverNet.h"
#include <string.h>
#include <sys/epoll.h>
#include "../utilities.h"

using namespace std;

#define NPNX_LOG_ERRNO(A) do{ \
    int err = errno; \
    cerr<<(A)<<strerror(err)<<endl; \
  } while (0)

int ServerNet::ListenerLoop()
{
  struct sockaddr_in serverAdd;
  do {  
    //Create socket
    listenfd = socket(AF_INET , SOCK_STREAM , 0);
    if (listenfd == -1)
    {
      NPNX_LOG_ERRNO("socket create failed:");
      break;
    }
      
    //Prepare the sockaddr_in structure
    serverAdd.sin_family = AF_INET;
    serverAdd.sin_addr.s_addr = INADDR_ANY;
    serverAdd.sin_port = htons( SERVER_PORT );
      
    //Bind
    if( bind(listenfd,(struct sockaddr *)&serverAdd, sizeof(serverAdd)) < 0)
    {
      NPNX_LOG_ERRNO("bind failed:");
      break;
    }

    if (listen(listenfd, maxListenQueue)<0) {
      NPNX_LOG_ERRNO("listen failed:");
      break;
    }

    //setup epoll and handler thread here.
    epfd = epoll_create(1);
    if (epfd==-1) {
      NPNX_LOG_ERRNO("epoll_create:");
      break;
    }

    epoll_event ev_stdin;
    ev_stdin.events = EPOLLIN | EPOLLERR; 
    ev_stdin.data.fd = stdinfd;
    
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, stdinfd, &ev_stdin)<0)  {
      NPNX_LOG_ERRNO("epoll add stdin:");
      break;
    }

    epoll_event ev_listener;
    ev_listener.events = EPOLLIN | EPOLLERR;
    ev_listener.data.fd = listenfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev_listener)<0) {
      NPNX_LOG_ERRNO("epoll add listener:");
      break;
    }

    // epoll_event ev_tcp;
    // ev_tcp.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    bool errorHappened = false;
    while(!errorHappened) {

      epoll_event ev_this;
      int suc = epoll_wait(epfd, &ev_this, 1, -1);
      if (suc<0){
        int err = errno;
        if (err == EINTR) {
          continue;
        } else {
          NPNX_LOG_ERRNO("epoll wait:");
          errorHappened = true;
          break;
        }
      } else if (suc=0) {
        continue;
      }

      int thisfd = ev_this.data.fd;
      if (thisfd == listenfd) {

        if (ev_this.events & (EPOLLERR|EPOLLHUP)){
          cerr<<"listenfd err|hup events."<<endl;
          errorHappened = true;
          break;
        }

        if (acceptConnection()<0) {
          errorHappened = true;
          break;
        }

      } else if (thisfd == stdinfd) {

        if (ev_this.events & EPOLLERR){
          cerr<<"stdinfd err events."<<endl;
          errorHappened = true;
          break;
        }

        char buf[4096];
        int len;
        if ((len=recv(stdinfd, buf, 4095, 0))<0) {
          NPNX_LOG_ERRNO("stdin recv");
          errorHappened = true;
          break;
        }
        buf[len] = '\0';
        string cmd(buf);
        if (cmd.substr(0,4)=="exit") {
          cerr<<"exit called."<<endl;
          errorHappened = true;
          break;
        }

      } else { //fd is a client fd.
        
        if (ev_this.events & EPOLLRDHUP) {
          cerr<<"client fd EPOLLRDHUP"<<endl;
          closeClientGently(thisfd);
          continue;
        }

        if (ev_this.events & (EPOLLERR | EPOLLHUP)) {
          cerr<<"client fd EPOLLERR OR EPOLLHUP"<<endl;
          closeClientViolently(thisfd);
          continue;
        }

        char buf[4096];
        int len; 
        while ((len = recv(thisfd, buf, 4095, MSG_DONTWAIT))>0) {
          buf[len]='\0';
          openedfd[thisfd]+=buf;
        }

        if (errno != EAGAIN) {
          NPNX_LOG_ERRNO("recv client error:");
          closeClientViolently(thisfd);
          continue;
        }

        //TODO:process command in openedfd.

      }
    }

    if (errorHappened) {
      break;
    }

  } while (0);

  //TODO:close epoll fd, listen fd, openedfd.
}

int ServerNet::acceptConnection()
{
    sockaddr clinetAdd;
    socklen_t len;
    int newfd = accept(listenfd, &clinetAdd, &len);
    if (newfd<0) {
      newfd = -1;
      //here we don't consider this as a fatal error because the 
      //error can happen in client side.
      //However, if it is a error in server, the loop will not stop.
      //so we check the errno here for local error 
      //and also set a stop if 200 errors happen in a row.
      int curerr = errno;
      switch (curerr)
      {
        case EBADF:
        case EFAULT:
        case EINVAL:
        case EMFILE:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
          std::cerr<<"accept error:"<<curerr<<endl;
          return -1;
          break;

        //this two is not error. 
        case EINTR:
        case EAGAIN:
          numAcceptError=0;
          return 0;
          break;
          
        default:
          numAcceptError++;
          if (numAcceptError>200) {
            std::cerr<<"accept more than 200 error in a row:"<<curerr<<endl;
            return -1;
          }
      }
    } else { // newfd returned by accept.
      numAcceptError=0;
      openedfd[newfd] = "";
      epoll_event ev_tcp;
      ev_tcp.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
      ev_tcp.data.fd = newfd;
      if (epoll_ctl(epfd, EPOLL_CTL_ADD, newfd, &ev_tcp)<0){
        NPNX_LOG_ERRNO("add tcp to epoll error:");
        return -1;
      }
    }
    return 0; 
}

void ServerNet::closeClientGently(int fd)
{
  //remove it from epoll first, otherwise epoll will generate EPOLLHUP
  //when we do shutdown. <-- no need because we dont wait before this 
  //function end.


}