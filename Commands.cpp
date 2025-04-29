#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <unistd.h>
#include "Commands.h"
#include <regex>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <iomanip>
#include <cstdlib>
#include <sys/syscall.h>


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

void _removeBackgroundSign(const char *cmd_line) { //TODO revert to normal if not used
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
    const_cast<char *> (cmd_line)[idx] = ' ';
    // truncate the command line string up to the last non-space character
    const_cast<char *> (cmd_line)[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

static const char *removeBackgroundAndCopy(const char *cmd_line) { // TODO need to free the duplicate in the destructor
    std::string cleaned(cmd_line);
    size_t bg_pos = cleaned.find('&');
    if (bg_pos != std::string::npos) {
        cleaned.erase(bg_pos);
    }
    return strdup(cleaned.c_str());
}

SmallShell::SmallShell() : prompt("smash"), Previous_Path(nullptr), Jobs(new JobsList()) {
}

SmallShell::~SmallShell() {
    if (Previous_Path != nullptr) {
        delete[] Previous_Path;
    }
    delete Jobs;
}

map<string, string> &SmallShell::getaliases() {
    return Aliases;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    //need to clean the first word if added & for built in commands
    string cleaned_cmd_for_built_in = cmd_s.substr(0, cmd_s.find_first_of('&'));

    //checking aliases starts here
    string command_alias = cmd_line;
    if (!getaliases().empty()) {
        if (getaliases().find(firstWord) != getaliases().end()) {
            command_alias = getaliases()[firstWord];
        }
    }

    string first_of_command_alias = command_alias.substr(0, command_alias.find_first_of(" \n"));
    if (cmd_s.find(">") != string::npos || cmd_s.find(">>") != string::npos) {
        return new RedirectionCommand(cmd_line, this);
    } else if (cleaned_cmd_for_built_in == "pwd" || firstWord == "pwd" || first_of_command_alias == "pwd") {
        return new GetCurrentDirectory(command_alias.c_str());
    } else if (cleaned_cmd_for_built_in == "showpid" || firstWord == "showpid" || first_of_command_alias == "showpid") {
        return new ShowPidCommand(command_alias.c_str());
    } else if (cleaned_cmd_for_built_in == "chprompt" || firstWord == "chprompt" ||
               first_of_command_alias == "chprompt") {
        return new ChangePromptCommand(command_alias.c_str(), this);
    } else if (cleaned_cmd_for_built_in == "cd" || firstWord == "cd" || first_of_command_alias == "cd") {
        return new ChangeDirectoryCommand(command_alias.c_str(), this);
    } else if (cleaned_cmd_for_built_in == "jobs" || firstWord == "jobs" || first_of_command_alias == "jobs") {
        return new JobsCommand(command_alias.c_str(), Jobs);
    } else if (cleaned_cmd_for_built_in == "fg" || firstWord == "fg" || first_of_command_alias == "fg") {
        return new fgCommand(command_alias.c_str(), Jobs);
    } else if (cleaned_cmd_for_built_in == "quit" || firstWord == "quit" || first_of_command_alias == "quit") {
        return new QuitCommand(command_alias.c_str(), Jobs);
    } else if (cleaned_cmd_for_built_in == "kill" || firstWord == "kill" || first_of_command_alias == "kill") {
        return new QuitCommand(command_alias.c_str(), Jobs);
    }else if (cleaned_cmd_for_built_in == "du" || firstWord == "du" || first_of_command_alias == "du") {
        return new DiskUsageCommand(command_alias.c_str());
    } else if (cleaned_cmd_for_built_in == "alias" || firstWord == "alias" || first_of_command_alias == "alias") {
        return new AliasCommand(command_alias.c_str(), this);
    } else if (cleaned_cmd_for_built_in == "unalias" || firstWord == "unalias" || first_of_command_alias == "unalias") {
        return new UnAliasCommand(command_alias.c_str(), this);
    } else if (cleaned_cmd_for_built_in == "watchproc" || firstWord == "watchproc" || first_of_command_alias == "watchproc"){
        return new WatchProcCommand(command_alias.c_str());
    } else if(cleaned_cmd_for_built_in == "whoami" || firstWord == "whoami" || first_of_command_alias == "whoami"){
        return new WhoAmICommand(command_alias.c_str());
    } else if (!firstWord.empty()) { ///external command
        return new ExternalCommand(command_alias.c_str(), Jobs);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command *cmd = CreateCommand(cmd_line);
    if (cmd) {
        cmd->execute();
    }
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


ExternalCommand::ExternalCommand(const char *cmd_line, JobsList *jobsList) :
        Command(removeBackgroundAndCopy(cmd_line)),
        original_cmd_line(cmd_line),
        is_complex(isCommandComplex()),
        is_background(_isBackgroundComamnd(cmd_line)),
        jobs_list(jobsList) {
    //also using the base class's constructor
//    if (isCommandComplex()) {
//        is_complex = true;
//    }
//    if (_isBackgroundComamnd(cmd_line)) {
//        is_background = true;
//    }
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
                SmallShell::getInstance().setcurrFgCmd(pid);
                if (waitpid(pid, nullptr, 0) == -1) {
                    perror("smash error: waitpid failed");
                }
                SmallShell::getInstance().setcurrFgCmd(-1);
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
                //needed to also pass "bash" as the first argument when running the external commands
                const char *bash_arguments[4] = {"bash", "-c", strdup(cmd_line.c_str()), nullptr};
                execvp("/bin/bash", const_cast<char *const *>(bash_arguments));
                perror("smash error: execvp failed");
                exit(1);
            }
        } else { //father
            if (isCommandBackground()) {
                jobs_list->addJob(pid, this);
            } else { //TODO may need to add foreground pid for signal handling
                if (waitpid(pid, nullptr, 0) == -1) {
                    perror("smash error: waitpid failed");
                }
            }
        }
    }
}

bool ExternalCommand::isCommandComplex() {
    for (auto &argument: arguments) {
        if (argument.find('*') != string::npos || argument.find('?') != string::npos) {
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

JobsList::JobsList() : MaxId(1), number_of_jobs(0), job_entries_vec_in_jobslist{} {}

JobsList::~JobsList() {
    for (auto &it: job_entries_vec_in_jobslist) {
        delete it;
    }
}

void JobsList::addJob(pid_t pid, ExternalCommand *cmd, bool isStopped) {
    removeFinishedJobs();
    JobEntry *temp = new JobEntry(cmd, MaxId, pid);
    job_entries_vec_in_jobslist.push_back(temp);
    MaxId++;
    number_of_jobs++;
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    for (auto &job: job_entries_vec_in_jobslist) {
        cout << "[" << job->JobId << "]" << job->cmd->getOriginalCmdLine() << endl;
    }
}

int JobsList::getNumOfJobs() const {
    return number_of_jobs;
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for (auto a: job_entries_vec_in_jobslist) {
        if (a->JobId == jobId) {
            return a;
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for (auto a = job_entries_vec_in_jobslist.begin(); a < job_entries_vec_in_jobslist.end();) {
        if ((*a)->JobId == jobId) {
            delete (*a);
            a = job_entries_vec_in_jobslist.erase(a);
            number_of_jobs -= 1;
            if (number_of_jobs == 0) {
                MaxId = 0;
            } else {
                MaxId = job_entries_vec_in_jobslist[number_of_jobs - 1]->JobId;
            }
        } else {
            ++a;
        }
    }
}

void JobsList::printAllJobsForQUIT() {
    for (auto a: job_entries_vec_in_jobslist) {
        cout << a->pid << ": " << a->cmd->getOriginalCmdLine() << endl;
    }
}

void JobsList::killAllJobs() {
    for (auto It = job_entries_vec_in_jobslist.begin(); It != job_entries_vec_in_jobslist.end(); ++It) {
        kill((*It)->pid, SIGKILL);
        JobEntry *job = *It;
        (*It) = nullptr;
        delete job;
    }
}

void JobsList::removeFinishedJobs() {
    int num;
    if (!this || job_entries_vec_in_jobslist.empty()) {
        return;
    }
    for (auto a = job_entries_vec_in_jobslist.begin(); a != job_entries_vec_in_jobslist.end();) {
        num = waitpid(a.operator*()->pid, NULL, WNOHANG);
        if (num == -1) {
            perror("smash error: waitpid failed");
            ++a;
        } else if (num == 0) {
            ++a;
        } else {
            a = job_entries_vec_in_jobslist.erase(a);
            number_of_jobs -= 1;
            if (number_of_jobs == 0) {
                MaxId = 0;
            } else {
                MaxId = job_entries_vec_in_jobslist[number_of_jobs - 1]->JobId;
            }
        }
    }
}

int JobsList::getMaxId() { return MaxId; }

JobsCommand::JobsCommand(const char *cmd_line, JobsList *Jobs) : BuiltInCommand(cmd_line), m_JobsList(Jobs) {}

void JobsCommand::execute() { m_JobsList->printJobsList(); }

fgCommand::fgCommand(const char *cmd_line, JobsList *Jobs) : BuiltInCommand(cmd_line),
                                                             Jobs(Jobs) { //TODO may need to try and catch
    JobId = 0;
    if (arguments.size() <= 2) {
        if (arguments.size() == 1 && Jobs->getNumOfJobs() == 0) {
            cerr << "smash error: fg: jobs list is empty";
        } else if (arguments.size() == 1) {
            pid = Jobs->getJobById(Jobs->getMaxId())->pid;
            JobId = Jobs->getMaxId();
        } else {
            JobId = stoi(arguments[1]);
            if (JobId < 0) {
                cerr << "smash error: fg: invalid arguments" << endl;
                return;
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

void fgCommand::execute() { //TODO also may need fgPid for signal handling
    if (pid == 0) {
        return;
    }
    cout << Jobs->getJobById(JobId)->cmd->getCmdLine() << " " << pid << endl;
    SmallShell::getInstance().setcurrFgCmd(pid);
    waitpid(pid, nullptr, 0);
    SmallShell::getInstance().setcurrFgCmd(-1);
    Jobs->removeJobById(JobId);
}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), Jobs(jobs) {}

void QuitCommand::execute() {
    Jobs->removeFinishedJobs();
    if (arguments.size() > 1 && arguments[1] == "kill") {
        cout << "sending SIGKILL signal to " << Jobs->getNumOfJobs() << " jobs:" << endl;
        Jobs->printAllJobsForQUIT();
        Jobs->killAllJobs();
    }
    exit(0);
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), Jobs(jobs) {
    Jobs->removeFinishedJobs();
    if (arguments.size() != 3) {
        cerr << "smash error: kill: invalid arguments" << endl;
    }
    int jobid = 0;
    int signal = 0;
    try {
        signal = stoi(arguments[1].substr(1));
        jobid = stoi(arguments[2]);
    } catch (...) {
        cerr << "smash error: kill: invalid arguments" << endl;
    }

    JobsList::JobEntry *Job = Jobs->getJobById(jobid);
    if (arguments[1][0] != '-') {
        cerr << "smash error: kill: invalid arguments" << endl;
    } else if (Job == nullptr) {
        cerr << "smash error: kill: job-id " << stoi(arguments[2]) << " does not exist" << endl;
    } else {
        if (kill(Job->pid, signal) == -1) {
            perror("smash error: kill failed");
        } else {
            cout << "signal number " << signal << " was sent to pid " << Job->pid << endl;
        }
    }
}

void KillCommand::execute() {}

AliasCommand::AliasCommand(const char *cmd_line, SmallShell *shell) : BuiltInCommand(cmd_line), shell(shell) {
// if(arguments.size() > 2){
//     cerr << "smash error: alias: invalid alias format" << endl;
// }
    if (arguments.size() == 1) {
        return;
    } else {
        if (arguments.size() == 2) {
            name = arguments[1].substr(0, arguments[1].find_first_of("="));
            command = arguments[1].substr(arguments[1].find_first_of("=") + 2,
                                          arguments[1].size() - arguments[1].find_first_of("=") - 3);
        } else {
            //this is for when the command has multiple arguments
            name = arguments[1].substr(0, arguments[1].find_first_of("="));

            //this is to take the command without the apostrophe in the first argument
            command = arguments[1].substr(arguments[1].find_first_of("=") + 2,
                                          arguments[1].size() - 1);

            //adding the rest of the arguments
            for (int i = 2; i <= arguments.size() - 2; i++) {
                command += " " + arguments[i];
            }

            //taking the last argument from the command without the apostrophe
            command += " " + arguments[arguments.size() - 1].substr(0, findEndOfAlias());
        }
        string cmdl = string(cmd_line);
        smatch alias_regex;
        regex format(R"(^alias ([a-zA-Z0-9]+)='([^']*)'$)");
        if (!(regex_match(cmdl, format))) {
            cerr << "smash error: alias: invalid alias format" << endl;
            return;
        }
        for (auto &e: reserved_in_bash) {
            if (name == e) {
                cerr << "smash error: alias: " << name << " already exists or is a reserved command" << endl;
                return;
            }
        }
        for (auto &e: shell->getaliases()) {
            if (name == e.first) {
                cerr << "smash error: alias:" << name << "already exists or is a reserved command" << endl;
                return;
            }
        }


//     shell->getaliases()[name] = command;
//     if(arguments.size() == 1){
//         for(auto& e : shell->getaliases()){
//             cout << e.first << "='" << e.second << "'" << endl;
//         }
//     }
    }

    //this is already in execute
}

void AliasCommand::execute() {
    if (this->arguments.size() == 1) {
        for (auto &name: shell->getaliases()) {
            cout << name.first + "='" + name.second + "'" << endl;
        }
    } else {
//        shell->getaliases().insert({name, command});
        shell->getaliases()[name] = command;
    }
}

UnAliasCommand::UnAliasCommand(const char *cmd_line, SmallShell *shell) : BuiltInCommand(cmd_line), shell(shell) {
    if (arguments.size() == 1) {
        cerr << "smash error: unalias: not enough arguments" << endl;
    }
    for (unsigned int i = 1; i < arguments.size(); i++) {
        if (shell->getaliases().erase(arguments[i]) == 0) {
            cerr << "smash error: unalias: " << string(arguments[i]) << " alias does not exist" << endl;
            return;
        }
    }
}

WatchProcCommand::WatchProcCommand(const char *cmd_line) : BuiltInCommand(cmd_line), pid(arguments[1]) {
}

void WatchProcCommand::execute() {

    cout << "PID: " + pid + " | ";

    //getting the cpu usage
    ifstream file("/proc/"+ this->pid +"/stat");
    string line;
    getline(file, line);

    istringstream iss(line);
    string temp;
    long utime, stime, cutime, cstime;
    long start_time;
    int cpu;

    //for skipping unwanted data
    for (int i = 0; i < 13; i++) iss >> temp;
    iss >> utime >> stime >> cutime >> cstime >> start_time;

    int total_time = utime + stime + cutime + cstime;

    //reading the total system time to calculate presentage
    ifstream cpu_file("/proc/stat");
    getline(cpu_file, line);
    istringstream cpu_stream(line);
    string processor;
    long user, nice, system, idle, iowait, irq, softirg, steal;

    cpu_stream >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirg>> steal;

    long total_system_time = cpu + nice + system + idle + iowait + irq + softirg + steal;
    long idle_time = idle + iowait;

    double cpu_usage = ((static_cast<double>(total_time) / total_system_time) * 100);
    cout << "CPU Usage: ";
    cout << fixed << setprecision(1) << cpu_usage;
    cout << " % | ";


    //getting the memory usage values
    string line_2, mem_key;
    long mem_value = 0;
    ifstream mem_file("/proc/" + pid + "/status");
    while (getline(mem_file, line_2)){
        istringstream iss(line_2);
        iss >> mem_key >> mem_value;
        if (mem_key == "VmRSS:") break;
    }

    double mem_usage_in_mb = mem_value / 1024;
    cout << "Memory Usage: ";
    cout << fixed << setprecision(1) << mem_usage_in_mb;
    cout << " MB" << endl;
}


void UnAliasCommand::execute() {}

RedirectionCommand::RedirectionCommand(const char *cmd_line, SmallShell *shell) : Command(cmd_line), shell(shell) {
    //this is to find the index of the I/O redir arrow
    int i = 0;
    for (auto &argument: arguments) {
        if (arguments[i] == ">") {
            append = false;
            break;
        }
        if (arguments[i] == ">>") {
            append = true;
            break;
        }
        i++;
    }
    left_side_command = getCmdLine().substr(0, getCmdLine().find_first_of(">") - 1);
    file_name = arguments[i + 1];
    command = shell->CreateCommand(left_side_command.c_str());
}

void RedirectionCommand::execute() { //TODO needs more testing
    pid_t pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
    }
    if (pid == 0) {
        int fd;
        close(1);
        if (append) {
            fd = open(file_name.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);
        } else {
            fd = open(file_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
        }
        if (fd == -1) {
            perror("smash error: open failed");
        }
        command->execute();
        exit(0);
    } else {
        if (waitpid(pid, nullptr, 0) == -1) {
            perror("smash error: waitpid failed");
        }
    }
}

DiskUsageCommand::DiskUsageCommand(const char *cmd_line) : Command(cmd_line) {//TODO ask around how people did this
}

void DiskUsageCommand::execute() {
    if (arguments.size() > 2) {
        cerr << "smash error: du: too many arguments" << endl;
        return;
    }

    string path = (arguments.size() == 1) ? "." : arguments[1];
    long long total_size = calculatePathSize(path);
    cout << "Total disk usage: " << total_size / 1024 << " KB" << endl;
}

WhoAmICommand::WhoAmICommand(const char *cmd_line) : Command(cmd_line) {
}

void WhoAmICommand::execute() {
    uid_t uid = getuid();

    // Open /etc/passwd
    int fd = open("/etc/passwd", O_RDONLY);
    if (fd == -1) {
        perror("smash error: open failed");
        return;
    }

    char buffer[1024];
    ssize_t bytesRead;
    bool found = false;
    size_t bufferPos = 0;

    while (!found && (bytesRead = read(fd, buffer + bufferPos, sizeof(buffer) - bufferPos - 1)) > 0) {
        // Ensure null-termination for string operations
        buffer[bufferPos + bytesRead] = '\0';
        char *line = buffer;

        // Process each complete line in buffer
        char *lineEnd;
        while ((lineEnd = strchr(line, '\n')) != nullptr) {
            *lineEnd = '\0'; // Null-terminate the line

            // Parse passwd line (username:password:uid:gid:gecos:home:shell)
            char *saveptr;
            char *username = strtok_r(line, ":", &saveptr);
            strtok_r(nullptr, ":", &saveptr); // skip password
            char *uidStr = strtok_r(nullptr, ":", &saveptr);
            strtok_r(nullptr, ":", &saveptr); // skip gid
            strtok_r(nullptr, ":", &saveptr); // skip gecos
            char *homeDir = strtok_r(nullptr, ":", &saveptr); // home directory (6th field)
            // We don't need the shell field (7th field)

            if (username && uidStr && homeDir && atoi(uidStr) == uid) {
                std::cout << username << " " << homeDir << std::endl;
                found = true;
                break;
            }

            line = lineEnd + 1; // Move to next line
        }

        // Handle remaining partial line (if any)
        if (!found) {
            bufferPos = strlen(line);
            if (bufferPos > 0) {
                memmove(buffer, line, bufferPos);
            }
        }
    }

    // Handle read errors
    if (bytesRead == -1) {
        perror("smash error: read failed");
    }

    // Handle user not found
    if (!found) {
        std::cerr << "smash error: user not found" << std::endl;
    }

    close(fd);
}




//static string fetchIpAddress(const string &networkInterface) {
//    struct ifaddrs *interfaceList = nullptr;
//    void *addressPointer = nullptr;
//
//    if (getifaddrs(&interfaceList) == -1) {
//        cerr << "smash error: netinfo: unable to get IP address for interface " << networkInterface << endl;
//        return "";
//    }
//
//    // Loop through the network interfaces
//    for (struct ifaddrs *currentInterface = interfaceList; currentInterface != nullptr; currentInterface = currentInterface->ifa_next) {
//        if (currentInterface->ifa_addr->sa_family == AF_INET) {
//            if (networkInterface == currentInterface->ifa_name) {
//                addressPointer = &((struct sockaddr_in *)currentInterface->ifa_addr)->sin_addr;
//                char ipBuffer[INET_ADDRSTRLEN];
//                inet_ntop(AF_INET, addressPointer, ipBuffer, INET_ADDRSTRLEN);
//                freeifaddrs(interfaceList);
//                return string(ipBuffer);
//            }
//        }
//    }
//    freeifaddrs(interfaceList);
//    cerr << "smash error: netinfo: interface " << networkInterface << " does not have an IP address" << endl;
//    return "";
//}
//
//static string fetchSubnetMask(const string &networkInterface) {
//    struct ifaddrs *interfaceList = nullptr;
//    void *addressPointer = nullptr;
//
//    if (getifaddrs(&interfaceList) == -1) {
//        cerr << "smash error: netinfo: unable to get subnet mask for interface " << networkInterface << endl;
//        return "";
//    }
//
//    // Loop through the network interfaces
//    for (struct ifaddrs *currentInterface = interfaceList; currentInterface != nullptr; currentInterface = currentInterface->ifa_next) {
//        if (currentInterface->ifa_addr->sa_family == AF_INET) {
//            if (networkInterface == currentInterface->ifa_name) {
//                addressPointer = &((struct sockaddr_in *)currentInterface->ifa_netmask)->sin_addr;
//                char subnetBuffer[INET_ADDRSTRLEN];
//                inet_ntop(AF_INET, addressPointer, subnetBuffer, INET_ADDRSTRLEN);
//                freeifaddrs(interfaceList);
//                return string(subnetBuffer);
//            }
//        }
//    }
//    freeifaddrs(interfaceList);
//    cerr << "smash error: netinfo: interface " << networkInterface << " does not have a subnet mask" << endl;
//    return "";
//}
//
//static string parseGatewayInfo(const string &networkInterface, char *lineContent) {
//    char *lineFields[16];
//    while (lineContent != NULL) {
//        int fieldCount = _parseCommandLine(lineContent, lineFields);
//
//        if (fieldCount < 3 || strcmp(lineFields[0], "Iface") == 0) {
//            lineContent = strtok(NULL, "\n");
//            continue;
//        }
//        if (networkInterface == lineFields[0]) {
//            if (strcmp(lineFields[1], "00000000") == 0) {
//                unsigned long gatewayAddress = strtoul(lineFields[2], nullptr, 16);
//                struct in_addr gatewayAddr;
//                gatewayAddr.s_addr = gatewayAddress;
//                return inet_ntoa(gatewayAddr);
//            }
//        }
//        lineContent = strtok(NULL, "\n");
//    }
//    return "";
//}
//
//static string fetchDefaultGateway(const string &networkInterface) {
//    int fileDesc = open("/proc/net/route", O_RDONLY);
//    if (fileDesc == -1) {
//        perror("smash error: netinfo");
//        return "";
//    }
//    char buffer[PATH_MAX];
//    ssize_t bytesRead = read(fileDesc, buffer, sizeof(buffer) - 1);
//    if (bytesRead == -1) {
//        perror("smash error: netinfo");
//        close(fileDesc);
//        return "";
//    }
//    buffer[bytesRead] = '\0';
//    close(fileDesc);
//
//    char *lineContent = strtok(buffer, "\n");
//
//    return parseGatewayInfo(networkInterface, lineContent);
//}
//
//static vector<string> extractDnsServers(char *bufferContent) {
//    char *lineContent = strtok(bufferContent, "\n");
//    char *lineFields[16];
//    vector<string> dnsServers;
//    while (lineContent != NULL) {
//        int fieldCount = _parseCommandLine(lineContent, lineFields);
//        if (fieldCount < 2) {
//            lineContent = strtok(NULL, "\n");
//            continue;
//        }
//        if (strcmp(lineFields[0], "nameserver") == 0) {
//            dnsServers.push_back(string(lineFields[1]));
//        }
//        lineContent = strtok(NULL, "\n");
//    }
//    return dnsServers;
//}
//
//static vector<string> fetchDnsServers() {
//    int fileDesc = open("/etc/resolv.conf", O_RDONLY);
//    vector<string> dnsServers;
//    if (fileDesc == -1) {
//        cerr << "smash error: netinfo: unable to open /etc/resolv.conf" << endl;
//        return dnsServers;
//    }
//    char buffer[PATH_MAX];
//    ssize_t bytesRead = read(fileDesc, buffer, sizeof(buffer) - 1);
//    if (bytesRead == -1) {
//        perror("smash error: netinfo");
//        if(close(fileDesc) == -1){
//            perror("smash error: close failed");
//        }
//        return dnsServers;
//    }
//    buffer[bytesRead] = '\0';
//    close(fileDesc);
//    dnsServers = extractDnsServers(buffer);
//    return dnsServers;
//}
//
//
//NetInfo::NetInfo(const char *cmd_line) { interfaceFound = false;  // No need for toIgnore flag anymore
//
//    if (_isBackgroundComamnd(cmd_line)) {
//        _removeBackgroundSign(cmd_line);  // Clean background symbol if present
//    }
//
//    char *argumentsArray[COMMAND_MAX_ARGS];
//    _parseCommandLine(cmd_line, argumentsArray);
//
//    if (argumentsArray[1] == nullptr) {
//        cerr << "smash error: netinfo: no interface specified" << endl;
//        interfaceFound = false;
//        return;
//    }
//
//    string networkInterface = argumentsArray[1];
//
//    // Retrieve IP address for the interface
//    ipAddress = fetchIpAddress(networkInterface);
//    if (ipAddress.empty()) {
//        interfaceFound = false;
//        return;
//    }
//
//    // Retrieve Subnet Mask
//    subnetMask = fetchSubnetMask(networkInterface);
//    if (subnetMask.empty()) {
//        interfaceFound = false;
//        return;
//    }
//
//    // Retrieve Default Gateway
//    defaultGateway = fetchDefaultGateway(networkInterface);
//    if (defaultGateway.empty()) {
//        interfaceFound = false;
//        return;
//    }
//
//    // Retrieve DNS Servers
//    dnsServers = fetchDnsServers();
//    if (dnsServers.empty()) {
//        interfaceFound = false;
//        return;
//    }
//
//    interfaceFound = true;  // Set flag to true if everything is retrieved successfully
//}
//
//void NetInfo::execute() {
//    if (interfaceFound) {
//        // Display the retrieved network information
//        cout << "IP Address: " << ipAddress << endl;
//        cout << "Subnet Mask: " << subnetMask << endl;
//        cout << "Default Gateway: " << defaultGateway << endl;
//        cout << "DNS Servers: ";
//        for (size_t i = 0; i < dnsServers.size(); ++i) {
//            cout << dnsServers[i];
//            if (i != dnsServers.size() - 1) {
//                cout << ", ";
//            }
//        }
//        cout << endl;
//    }
//}

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {}

void PipeCommand::execute() {
    string command = _trim(getCmdLine());
    int cutoff;
    int type;
    cutoff = command.find("|&");
    if(cutoff != -1){
        type = 2;
    } else {
        cutoff = command.find("|");
        if(cutoff != -1){
            type = 1;
        } else {
            std::cerr << "smash error: invalid pipe command" << std::endl;
            return;
        }
    }

    string Command2 = "";
    string Command1 = command.substr(0, cutoff);
    if(type == 1){
        Command2 = command.substr(cutoff + 1);
    } else {
        Command2 = command.substr(cutoff + 2);
    }

    int fd[2];
    if(pipe(fd) == -1){
        perror("smash error: pipe failed");
        return;
    }

    pid_t pid1 = fork();
    pid_t pid2 = fork();
    if(pid2 < 0 || pid1 < 0){
        perror("smash error: fork failed");
    }

    if(pid1 == 0){
        setpgrp();
        close(fd[0]);
        if(type == 1){
            dup2(fd[1], STDOUT_FILENO);
        } else {
            dup2(fd[1], STDERR_FILENO);
        }
        close(fd[1]);
        SmallShell::getInstance().executeCommand(Command1.c_str());
        exit(0);
    }

    if(pid2 == 0){
        setpgrp();
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        SmallShell::getInstance().executeCommand(Command2.c_str());
        exit(0);
    }

    close(fd[0]);
    close(fd[1]);
    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);
}