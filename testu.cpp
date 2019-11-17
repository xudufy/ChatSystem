#include "utilities.h"

using namespace std;

int main() {
    string s1="as\\df;::\\;:g\\wgg\\";
    string s2 = ":;ff\\f:;;\\;:\\;";
    
    string s3 = excapeMsg(s1);
    string s4 = excapeMsg(s2);
    cout<<s3<<endl;
    cout<<s4<<endl;
    
    vector<string> asdf = {s1,s2};
    string s7 = compositeMsg(asdf);
    cout<<s7<<endl;

    string s5 = s7+s7+s7.substr(0,40);
    vector<string> sep = separateMsg(s5);
    for (auto i:sep) {
        cout<<i<<endl;
        vector<string> out = deCompositeMsg(i);
        for (auto j:out) {
            cout<<j<<endl;
        }
    }
    cout<<s5<<endl<<endl;

    s5 = s7+s7+s7;
    sep = separateMsg(s5);
    for (auto i:sep) {
        cout<<i<<endl;
        vector<string> out = deCompositeMsg(i);
        for (auto j:out) {
            cout<<j<<endl;
        }
    }
    cout<<s5<<endl<<endl;

    return 0;
}