#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
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

SmallShell::SmallShell() : prompt("smash"), Previous_Path(nullptr){
// TODO: add your implementation
}

SmallShell::~SmallShell(){
    if(Previous_Path != nullptr){
        delete[] Previous_Path;
    }
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrentDirectory(cmd_line.c_str);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line.c_str);
  }
  else if (firstWord.compare("chprompt") == 0) {
      return new ChangePromptCommand(cmd_line.c_str, this);
  }
  else if (firstWord.compare("cd") == 0) {
      return new ChangeDirectoryCommand(cmd_line.c_str, this);
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

Command::Command(const char *cmd_line) : cmd_line(cmd_line) {

    //making an array of arguments to use the given parse function
    char* arg_arr[COMMAND_MAX_ARGS] = {nullptr};

    //filling that array using the parse function
    _parseCommandLine(cmd_line, arg_arr);

    //moving the arguments from the array to the vector
    for (int i = 0; i < COMMAND_MAX_ARGS && arg_arr[i]; i++) {
        string curr_arg(arg_arr[i]);
        arguments.push_back(curr_arg);
    }
}

Command::~Command() {}

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

void ShowPidCommand::execute(){
    pid_t shell_pid = getpid();
    std::cout << "smash pid is " << shell_pid << std::endl;
}


GetCurrentDirectory::GetCurrentDirectory(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrentDirectory::execute() {
    char cwd[PATH_SIZE];
    if(getcwd(cwd, sizeof(cwd)) != nullptr){
        string curr_dir(cwd);
        cout << "Current working directory: " << curr_dir << endl;
    } else {
        perror("smash error: getcwd failed");
    }
}

ChangeDirectoryCommand::ChangeDirectoryCommand(const char *cmd_line, SmallShell* shell) : BuiltInCommand(cmd_line) {
    if(arguments.size() > 2){
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }
    if(arguments.size() == 1){
        return;
    }
    if(shell->GetPreviousPath() == nullptr && arguments[1] == "-"){
        cerr << "smash error: cd: OLDPWD not set" << endl;
        return;
    }

    char* Path = new char[PATH_MAX];
    if(getcwd(Path, PATH_MAX) == nullptr){
        perror("smash error: getcwd failed");
        delete[] Path;
        return;
    }

    if(arguments[1] == "-"){
        {
            char* temp = shell->GetPreviousPath();
            if(chdir(temp) == -1){
                perror("smash error: chdir failed");
            } else {
                shell->SetPreviousPath(Path);
            }
        }
    } else {
        string temp = arguments[1];
        if(chdir(temp.c_str()) == -1){
            perror("smash error: chdir failed");
        } else {
            shell->SetPreviousPath(Path);
        }
    }
}

void ChangeDirectoryCommand::execute() {}

