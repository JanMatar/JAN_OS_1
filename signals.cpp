#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    std::cout << "smash: got ctrl-C" << std::endl;

    pid_t currpid = SmallShell::getInstance().getcurrFgCmd();
    if (currpid != -1) {
        kill(currpid, SIGKILL);
        std::cout << "smash: process " << currpid << " was killed" << std::endl;
        SmallShell::getInstance().setcurrFgCmd(-1);
    }
}


