using namespace std;
#include <iostream>
#include <string>
#include <vector>
void do_fork() {
    //fork();
    cout << "do fork" << endl;
    return;
}
void do_cat(string arg) {
    //exec(cat, arg);
    cout << "do cat" << endl;
    return;
}
void do_ls(string arg) {
    //exec(cat, arg);
    cout << "do cat" << endl;
    return;
}
int main() {
    vector<string> cmd;
    string cur_cmd;
    while (1) {
        cout << "% ";
        getline(cin, cur_cmd);
        //stringstream ss(cur_cmd);
        while (getline(cin, cur_cmd, ' ')) {
            cmd.push_back(cur_cmd);
        }
        if (cmd.size() == 0 || cmd[0] == "exit") {
            break;
        }
        for (int i = 0; i < cmd.size(); i++) {
            cout << cmd[i] << endl;
        }

        
    }
    

    return 0;
}



