#include <unistd.h>
#include "clientGui.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
    cerr<<"mypid:"<<getpid()<<endl;
    gui_entry(argc, argv);
    string tem;
    cin>>tem;
    return 0;
}