#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <unistd.h>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() : prompt("smash"), Previous_Path(nullptr) {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
    if (Previous_Path != nullptr) {
        delete[] Previous_Path;
    }
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord == "pwd") {
        return new GetCurrentDirectory(cmd_line);
    } else if (firstWord == "showpid") {
        return new ShowPidCommand(cmd_line);
    } else if (firstWord == "chprompt") {
        return new ChangePromptCommand(cmd_line, this);
    } else if (firstWord == "cd") {
        return new ChangeDirectoryCommand(cmd_line, this);
    } else if (firstWord == "jobs") {
        return new JobsCommand(cmd_line, Jobs);
    } else if (firstWord == "fg") {
        return new fgcommand(cmd_line, Jobs);
    } else if (firstWord == "quit") {
        return new QuitCommand(cmd_line, Jobs);
    } else if (!firstWord.empty()) { ///external command
        return new ExternalCommand(cmd_line, Jobs);
    }
//  else if ...
//  .....
//  else {
//    return new ExternalCommand(cmd_line);
//  }
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

Command::Command(const char *cmd_line) : cmd_line(cmd_line), arguments() {

    //making an array of arguments to use the given parse function
    char *arg_arr[COMMAND_MAX_ARGS] = {nullptr};

    //filling that array using the parse function
    _parseCommandLine(cmd_line, arg_arr);

    //moving the arguments from the array to the vector
    for (int i = 0; i < COMMAND_MAX_ARGS && arg_arr[i]; i++) {
        string curr_arg(arg_arr[i]);
        arguments.push_back(curr_arg);
    }
}

Command::~Command() {}

ExternalCommand::ExternalCommand(const char *cmd_line, JobsList *jobsList) : Command(cmd_line), jobs_list(jobsList) {
    //also using the base class's constructor
    if (isCommandComplex()) {
        is_complex = false;
    }
    if (_isBackgroundComamnd(cmd_line)) {
        is_background = true;
    }
}

void ExternalCommand::execute() { //TODO test extensively, also some code duplication
    if (!is_complex) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("smash error: fork failed");
            return;
        } else if (pid == 0) { //child
            pid_t grpPid = setpgrp();
            if (grpPid == -1) {
                perror("smash error: setpgrp failed");
                exit(1);
            } else { // non-complex execution

                //turning the arguments from the vector of string into a char* array
                vector<char *> args;
                for (auto &arg: arguments) {
                    args.push_back(strdup(arg.c_str()));
                }

                //execvp is one of the functions of the exec family, and it automatically searches the PATH environment
                execvp(args[0], args.data());
                perror("smash error: execvp failed");
                exit(1);
            }
        } else { //father
            if (isCommandBackground()) {
                jobs_list->addJob(pid, this);
            } else { //TODO may need to add foreground pid for signal handling
                if (waitpid(pid, nullptr, 0) == -1){
                    perror("smash error: waitpid failed");
                }
            }
        }
    } else { //complex execution
        pid_t pid = fork();
        if (pid == -1) {
            perror("smash error: fork failed");
            return;
        } else if (pid == 0) { //child
            pid_t grpPid = setpgrp();
            if (grpPid == -1) {
                perror("smash error: setpgrp failed");
                exit(1);
            } else {
                string cmd_line = this->getCmdLine();
                const char *bash_args[3] = {"-c", cmd_line.c_str(), nullptr};
                execvp("/bin/bash", const_cast<char *const *>(bash_args));
                perror("smash error: execvp failed");
                exit(1);
            }
        } else { //father
            if (isCommandBackground()) {
                jobs_list->addJob(pid, this);
            } else { //TODO may need to add foreground pid for signal handling
                if (waitpid(pid, nullptr, 0) == -1){
                    perror("smash error: waitpid failed");
                }
            }
        }
    }
}

bool ExternalCommand::isCommandComplex() {
    for (auto &argument: arguments) {
        if (argument.find('*') || argument.find('?')) {
            return true;
        }
    }
    return false;
}

bool ExternalCommand::isCommandBackground() const {
    return is_background;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {
    //using the base class's constructor in the initialization list
}

ChangePromptCommand::ChangePromptCommand(const char *cmd_line, SmallShell *shell)
        : BuiltInCommand(cmd_line), shell(shell) {
    prompt = string("smash");
    if (arguments.size() >= 2) {
        prompt = arguments[1];
    }
}

void ChangePromptCommand::execute() {
    shell->setPrompt(prompt);
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    pid_t shell_pid = getpid();
    std::cout << "smash pid is " << shell_pid << std::endl;
}


GetCurrentDirectory::GetCurrentDirectory(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrentDirectory::execute() {
    char cwd[PATH_SIZE];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        string curr_dir(cwd);
        cout << "Current working directory: " << curr_dir << endl;
    } else {
        perror("smash error: getcwd failed");
    }
}

ChangeDirectoryCommand::ChangeDirectoryCommand(const char *cmd_line, SmallShell *shell) : BuiltInCommand(cmd_line) {
    if (arguments.size() > 2) {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }
    if (arguments.size() == 1) {
        return;
    }
    if (shell->GetPreviousPath() == nullptr && arguments[1] == "-") {
        cerr << "smash error: cd: OLDPWD not set" << endl;
        return;
    }

    char *Path = new char[PATH_SIZE];
    if (getcwd(Path, PATH_SIZE) == nullptr) {
        perror("smash error: getcwd failed");
        delete[] Path;
        return;
    }

    if (arguments[1] == "-") {
        {
            char *temp = shell->GetPreviousPath();
            if (chdir(temp) == -1) {
                perror("smash error: chdir failed");
            } else {
                shell->SetPreviousPath(Path);
            }
        }
    } else {
        string temp = arguments[1];
        if (chdir(temp.c_str()) == -1) {
            perror("smash error: chdir failed");
        } else {
            shell->SetPreviousPath(Path);
        }
    }
}

void ChangeDirectoryCommand::execute() {}

JobsList::JobsList() : number_of_jobs(0), MaxId(0) {}

JobsList::~JobsList() {
    for (auto &it: JobList) {
        delete it;
    }
}

void JobsList::addJob(pid_t pid, ExternalCommand *cmd, bool isStopped) {
    removeFinishedJobs();
    JobEntry *temp = new JobEntry(cmd, pid, MaxId);
    JobList.push_back(temp);
    MaxId++;
    number_of_jobs++;
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    for (auto &a: JobList) {
        cout << "[" << a->JobId << "]" << a->cmd->getCmdLine() << endl;
    }
}

int JobsList::getNumOfJobs() const {
    return number_of_jobs;
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for (auto a: JobList) {
        if (a->JobId == jobId) {
            return a;
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for (auto a = JobList.begin(); a < JobList.end();) {
        if ((*a)->JobId == jobId) {
            delete (*a);
            a = JobList.erase(a);
            number_of_jobs -= 1;
            if (number_of_jobs == 0) {
                MaxId = 0;
            } else {
                MaxId = JobList[number_of_jobs - 1]->JobId;
            }
        } else {
            ++a;
        }
    }
}

void JobsList::printAllJobsForQUIT() {
    for (auto a: JobList) {
        cout << a->pid << ": " << a->cmd << endl;
    }
}

void JobsList::killAllJobs() {
    for (auto It = JobList.begin(); It != JobList.end(); ++It) {
        kill((*It)->pid, SIGKILL);
        JobEntry *job = *It;
        (*It) = nullptr;
        delete job;
    }
}

void JobsList::removeFinishedJobs() {
    int num;
    for (auto a = JobList.begin(); a != JobList.end();) {
        num = waitpid((*a)->pid, NULL, WNOHANG);
        if (num == -1) {
            perror("smash error: waitpid failed");
            ++a;
        } else if (num == 0) {
            ++a;
        } else {
            a = JobList.erase(a);
            number_of_jobs -= 1;
            if (number_of_jobs == 0) {
                MaxId = 0;
            } else {
                MaxId = JobList[number_of_jobs - 1]->JobId;
            }
        }
    }
}

int JobsList::getMaxId() { return MaxId; }

JobsCommand::JobsCommand(const char *cmd_line, JobsList *Jobs) : BuiltInCommand(cmd_line), m_JobsList(Jobs) {}

void JobsCommand::execute() { m_JobsList->printJobsList(); }

fgcommand::fgcommand(const char *cmd_line, JobsList *Jobs) : BuiltInCommand(cmd_line), Jobs(Jobs) {
    JobId = 0;
    if (arguments.size() <= 2) {
        if (arguments.size() == 1 && Jobs->getNumOfJobs() == 0) {
            cerr << "smash error: fg: jobs list is empty";
        } else if (arguments.size() == 1) {
            pid = Jobs->getJobById(Jobs->getMaxId())->pid;
            JobId = Jobs->getMaxId();
        } else {
            JobId = stoi(arguments[1]);
            if (JobId <= 0) {
                cerr << "smash error: fg: invalid arguments" << endl;
            }
            JobsList::JobEntry *temp = Jobs->getJobById(JobId);
            if (temp != nullptr) {
                pid = temp->pid;
            } else {
                cerr << "smash error: fg: job-id" << JobId << "does not exist";
            }

        }
    } else {
        cerr << "smash error: fg: invalid arguments" << endl;
    }
}

void fgcommand::execute() {
    if (pid == 0) {
        return;
    }
    cout << Jobs->getJobById(JobId)->cmd->getCmdLine() << " " << pid << endl;
    waitpid(pid, nullptr, 0);
}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), Jobs(Jobs) {}

void QuitCommand::execute() {
    Jobs->removeFinishedJobs();
    if (arguments[1] == "kill") {
        cout << "sending SIGKILL signal to" << Jobs->getNumOfJobs() << "jobs:" << endl;
        Jobs->printAllJobsForQUIT();
        Jobs->killAllJobs();
    }
    exit(0);
}