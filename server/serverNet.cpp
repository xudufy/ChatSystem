#include "serverNet.h"
#include <string.h>

using namespace std;

#define NPNX_LOG_ERRNO(A) do{ \
    int err = errno; \
    cerr<<(A)<<strerror(err)<<endl; \
  } while (0)

ServerNet::ServerNet()
{
  shouldStop = false;
}

int ServerNet::ListenerLoop()
{
  struct sockaddr_in serverAdd;
  bool errorHappened = false;
  do {  
    //Create socket
    listenfd = socket(AF_INET , SOCK_STREAM , 0);
    if (listenfd == -1)
    {
      NPNX_LOG_ERRNO("socket create failed:");
      errorHappened = true;
      break;
    }
      
    //Prepare the sockaddr_in structure
    serverAdd.sin_family = AF_INET;
    serverAdd.sin_addr.s_addr = INADDR_ANY;
    serverAdd.sin_port = htons( SERVER_PORT );
      
    //Bind
    if( bind(listenfd,(struct sockaddr *)serverAdd, sizeof(serverAdd)) < 0)
    {
      NPNX_LOG_ERRNO("bind failed:");
      errorHappened = true;
      break;
    }

    if (listen(listenfd, maxListenQueue)<0) {
      NPNX_LOG_ERRNO("listen failed:");
      errorHappened = true;
      break;
    }
  
  } while (0);

  if (errorHappened) {
    if (listenfd>0) close(listenfd);
    listenfd=-1;
    errorHappened = false;
    return -1;
  }

  errorHappened=false;
  //setup epoll and handler thread here.
  

  while(!errorHappened) {
    


  }

  //close epoll fd, listen fd and join&delete thread.
  
}

int ServerNet::handlerLoop()
{

}

