#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>

// main function declarations
void cmd_loop();
std::string read_line();
std::vector<std::string> split_line(std::string line);
int execute(std::vector<std::string> args);
int launch_ps(std::vector<std::string> args);

// prototypes for built-in shell commands
int cd(std::vector<std::string>& args);
int help(std::vector<std::string>& args);
int exit(std::vector<std::string>& args);

const std::vector<std::string> builtins = {"cd", "help", "exit"};

int main(int argc, char **argv)
{
    cmd_loop();
    return 0;
}

void cmd_loop()
{
    std::string line;
    std::vector<std::string> args;
    int status;

    do {
        std::cout << "3sh> ";
        line = read_line();
        args = split_line(line);
        status = execute(args);

        //delete line;
        //delete args;
    }
    while (status);
}

std::string read_line()
{
    std::string line;
    std::getline(std::cin, line);
    return line;
}

std::vector<std::string> split_line(std::string line)
{
    std::istringstream iss(line);
    std::string arg;
    std::vector<std::string> args;

    while (iss >> arg) {
        args.push_back(arg);
    }
    return args;
}

int launch_ps(std::vector<std::string> args)
{
    // convert our command args into a vector of char* for execvp() to handle
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);

    for (const std::string& s: args) {
        argv.push_back(const_cast<char*>(s.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // child process
        if (execvp(argv[0], argv.data()) == -1) {
            std::cerr << "3sh: command not found" << '\n';
        }
        exit(1);
    }
    else if (pid < 0) {
        // error forking
        std::cerr << "3sh: fork() error" << '\n';
    }
    else {
        // parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        }
        while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

// table of function pointers to handle builtins
std::vector<int(*)(std::vector<std::string>&)> builtin_func = {
    &cd,
    &help,
    &exit
};

int cd(std::vector<std::string>& args)
{
    if (chdir(args[1].c_str()) != 0) {
        std::cerr << "3sh: no such directory" << '\n';
        return -1;
    }
    return 1;
}

int help(std::vector<std::string>& args)
{
    int i;
    std::cout << "3sh: lightweight shell, version 0.9" << '\n';
    std::cout << "usage: [command] [argument(s)]" << '\n';
    std::cout << "built-in commands: ";
    for (int i = 0; i < builtins.size() - 1; i++) {
        std::cout << builtins[i] << ", ";
    }
    std::cout << builtins[builtins.size() - 1] << '\n';
    return 1;
}

int exit(std::vector<std::string>& args)
{
    std::cout << "3sh: <exiting>" << '\n';
    return 0;
}

int execute(std::vector<std::string> args)
{
    if (args.empty()) {
        // continue
        return 1;
    }
    // execute builtin
    for (int i = 0; i < builtins.size(); i++) {
        if (args[0] == builtins[i]) {
            return (*builtin_func[i])(args);
        }
    }
    return launch_ps(args);
}
