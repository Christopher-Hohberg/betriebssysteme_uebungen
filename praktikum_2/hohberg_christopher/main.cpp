#include <iostream>
#include <sstream>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>
using namespace std;

//Header für Fork function
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>

//Header für signal
#include <signal.h>

pid_t mainPid;
vector<pid_t> kids;
pid_t kidpid;

void handle_SIGTSTP(int signum) {
    if(kill(kidpid, SIGTSTP) == 0) {
        cout << "prozess: " << kidpid << " gestoppt" << endl;
    }
}

void handle_SIGCONT(int signum) {

}

void handle_SIGCHLD(int signum) {
    int wstat;
	pid_t pid = wait3(&wstat, WNOHANG, NULL );
    for (int i = 0; i < kids.size(); i++) {
        if (kids[i] == pid) {
            kids.erase(kids.begin()+i);
        }
    }
}


void read_command(char** arguments, bool background) {
    int status {0};

    if ( (kidpid = fork()) < 0 ) {
        cerr << "ERROR: fork failed!\n";
        exit(1);
    }
    setpgid(kidpid, kidpid);

    if ( kidpid == 0 ) {
        if (execvp(*arguments, arguments) < 0) {
            cerr << "ERROR: execute failed !\n";
            exit(1);
        }
        exit(0);
    }
    else if ( background ) {
        kids.push_back(kidpid);
        cout << "[Background] : ";
        for (auto kid : kids) {
            cout << kid << ", ";
        }
        cout << endl;
    }
    else {
        if ( waitpid( kidpid, &status, 0 ) < 0 ) {
            cerr << "ERROR: cannot wait for child.\n" ;
            exit (1);
        }
    }
}


int main() {
    signal(SIGTSTP, handle_SIGTSTP);
    signal(SIGCHLD, handle_SIGCHLD);
    signal(SIGCONT, handle_SIGCONT);
    mainPid = getpid();
    while (true) {
        string cmd {};
        bool execute_background = false;
        cout << "myshell> ";
        getline(cin, cmd);

        if(cmd.size()!= 0) {
            if(cmd == "logout") {
                if(!kids.empty()) {
                    cout << "logout nicht möglich. Es laufen noch Hintergrundprozesse mit pids: ";
                    for (auto kid : kids) {
                        cout << kid << ", ";
                    }
                    cout << endl;
                }
                else {
                    char input {};
                    cout << "Möchten Sie die Shell wirklich beenden ? <Y/N> ";
                    cin >> input;
                    if ( input == 'y' || input == 'Y') {
                        exit(0);
                    }
                }
            }
            else {
                istringstream iss(cmd);
                vector<string> args( (istream_iterator < string > (iss)) , istream_iterator < string > () );

                if(args[args.size()-1] == "&") {
                    args.pop_back();
                    execute_background = true;
                }
            
                if(args.at(0) == "cont") {
                    pid_t kpid = stoi(args.at(1));
                    int ka = kill(kpid, SIGCONT);
                }
                else if(args.at(0) == "stop") {
                    pid_t kpid = stoi(args.at(1));
                    int ka = kill(kpid, SIGTSTP);
                }
                else {
                    char *arguments[20] = {};

                    int i {};
                    for (i = 0; i < args.size() ; i++) {
                        const char *currentArg = strdup(args.at(i).c_str());
                        arguments[i] = const_cast < char *> (currentArg);
                    }
                    arguments[i+1] = nullptr;
                    read_command(arguments, execute_background);
                }
            }
        }
    }
    return 0;
}
