#include "serverNet.h"
#include <string.h>
#include <sys/epoll.h>
#include <signal.h>
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

    cerr<<"listener loop start."<<endl;
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
      } else if (suc==0) {
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

        //This is a workaround to avoid possible SIGTTIN signal when runs in background. 
        //Problem may happen if the program switch from foreground to background between this and next read.
        //The right thing to do is correctly handling SIGTTIN.
        if (getpgrp() == tcgetpgrp(STDIN_FILENO)) { 
          char buf[4096];
          int len;
          if ((len=read(stdinfd, buf, 4095))<0) { 
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

        //process command in openedfd.
        vector<string> cmds = separateMsg(openedfd[thisfd]);
        string errorcmd  = "";
        for (auto cmd:cmds) {
          vector<string> cols = deCompositeMsg(cmd);

          if (cols[0] == CMDHEAD_LOGIN) {
          
            if (cols.size()<2) {
              errorcmd = cmd;
              closeClientViolently(thisfd);
              break;
            }
            oneLogIn(thisfd, cols[1]);
          
          } else if (cols[0] == CMDHEAD_LOGOUT) {
          
            oneLogOut(thisfd);
          
          } else if (cols[0] == CMDHEAD_SEND) {
          
            if (cols.size()<3) {
              errorcmd = cmd;
              closeClientViolently(thisfd);
              break;
            }
            oneSendMessage(thisfd, cols[1], cols[2]);
          
          } else {
              errorcmd = cmd;
              closeClientViolently(thisfd);
              break;
          }

        } // end : for each cmds in openfd[thisfd].
        
        if (errorcmd!="") {
          cerr<<"custom command error:"<<errorcmd<<endl<<"cols:";
          vector<string> checkColumn = deCompositeMsg(errorcmd);
          for (auto col:checkColumn) {
            cerr<<col<<",";
          }
          cerr<<endl;
        }

      } // end : if thisfd is a client fd.
    
    } //end: listener loop.

    if (errorHappened) {
      break;
    }

  } while (0);

  //close epoll fd, listen fd, openedfd.
  for (auto it: openedfd) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, it.first, NULL);
    close(it.first);
  }
  close(epfd);
  close(listenfd);
  
  epfd = -1;
  listenfd = -1;
  openedfd.clear();
  name2Fd.clear();
  fd2name.clear();

  return 0;
}

int ServerNet::acceptConnection()
{
    sockaddr_in clinetAdd;
    socklen_t len = sizeof(clinetAdd);
    int newfd = accept(listenfd, (sockaddr *)&clinetAdd, &len);
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
  oneLogOut(fd);
  epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);  
  shutdown(fd, SHUT_WR);
  char buf[4096];
  int timelimit = 10;
  while (timelimit>0 && recv(fd, buf, 4095, MSG_DONTWAIT)>0){
    timelimit--;
  } //use time limit when client send flood after fin.
  openedfd.erase(fd);
  close(fd);
  cerr<<"fd "<<fd<<" closed gently."<<endl;
}

void ServerNet::closeClientViolently(int fd)
{
  oneLogOut(fd);
  epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);  
  openedfd.erase(fd);
  close(fd);
  cerr<<"fd "<<fd<<" closed violently."<<endl;
}

void ServerNet::oneLogOut(int fd)
{
  if (fd2name.count(fd)>0) {
    string name = fd2name[fd];
    fd2name.erase(fd);
    name2Fd.erase(name);
    cerr<<"fd "<<fd<<" "<<name<<" logged out."<<endl;


    string cmd = compositeMsg({CMDHEAD_USER_DELETE, name});
    broadcast(cmd, fd);
  }
}

void ServerNet::oneLogIn(int fd, const std::string & nickname)
{
  if (fd2name.count(fd)==1) {
    return;
    // already logged in. do nothing.
  }

  if (name2Fd.count(nickname)==1) {

    string cmd = compositeMsg({CMDHEAD_LOGIN_FAILED_NICKNAME});
    if (send(fd, cmd.c_str(), cmd.size(), 0)<0) {
      closeClientViolently(fd);
    }
    cerr<<"log:login_failed_nickname: "<<nickname<<endl;
    return;

  } 

  std::vector<string> cols({CMDHEAD_LOGIN_OK, nickname});
  for (auto username:name2Fd) {
    cols.push_back(username.first);
  }

  string cmd = compositeMsg(cols);
  if (send(fd, cmd.c_str(), cmd.size(), 0)<0) {
    closeClientViolently(fd);
    return;
  }

  name2Fd[nickname] = fd;
  fd2name[fd] = nickname;

  cerr<<nickname<<" logged in."<<endl;

  cmd = compositeMsg({CMDHEAD_USER_ADD, nickname});
  broadcast(cmd, fd);

}

void ServerNet::oneSendMessage(int in_fd, const std::string & to_name, const std::string & msg)
{
  if (fd2name.count(in_fd) == 0) {
    closeClientViolently(in_fd);
    cerr<<"fd "<<in_fd<<" send message while not logged in."<< endl;
    return;
  }

  if (name2Fd.count(to_name) > 0) {
  
    string cmd = compositeMsg({CMDHEAD_RECV, fd2name[in_fd], msg});
    if (send(name2Fd[to_name], cmd.c_str(), cmd.size(), 0)>0) {
      return;
    } else {
      closeClientViolently(name2Fd[to_name]);  
    }
    
  }

  string cmd = compositeMsg({CMDHEAD_SEND_FAILED, to_name});
  if (send(in_fd, cmd.c_str(), cmd.size(), 0)<0) {
    closeClientViolently(in_fd);
  }
  
}

void ServerNet::broadcast(const std::string & cmd, int exceptfd){

    std::vector<int> failedfds;
    failedfds.clear();

    for (auto userfd: fd2name) {
      if (userfd.first==exceptfd) continue;
      if (send(userfd.first, cmd.c_str(), cmd.size(), 0)<0) {
        failedfds.push_back(userfd.first);
      }
    }

    for (auto failedfd: failedfds) {
      closeClientViolently(failedfd); 
      //even the failed fd has sent a fin between last epoll_wait to this,
      //it should be able to receive data.
    }

}
