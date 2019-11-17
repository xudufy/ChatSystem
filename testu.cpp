#include "utilities.h"

using namespace std;

int main() {
    string s1="as\\df;::\\;:g\\wgg\\";
    string s2 = ":;ff\\f:;;\\;:\\;";
    
    string s3 = excapeMsg(s1);
    string s4 = excapeMsg(s2);
    cout<<s3<<endl;
    cout<<s4<<endl;
    
    vector<string> asdf = {s3,s4};
    string s5 = compositeMsg(asdf);
    cout<<s5<<endl;
    
    vector<string> out = deCompositeMsg(s5);
    for (auto i:out) {
        cout<<unExcapeMsg(i)<<endl;
    }

    return 0;
}