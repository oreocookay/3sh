#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>
#include <unistd.h>

void cmd_loop();
std::string read_line();
std::vector<std::string> split_line(std::string);
int execute(std::vector<std::string> tokens);
int launch_ps(std::vector<std::string> args);

// prototypes for built-in shell commands
int cd(char **args);
int help(char **args);
int exit(char **args);

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
        std::cout << "> ";
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
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

int launch_ps(std::vector<std::string> args)
{
    // convert vector of strings into vector of char* for execvp
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);

    for (const auto& s: args) {
        argv.push_back(const_cast<char*>(s.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // child process
        if (execvp(argv[0], argv.data()) == -1) {
            std::cerr << "3sh" << '\n';
        }
        exit(1);
    }
    else if (pid < 0) {
        // error forking
        std::cerr << "3sh" << '\n';
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

std::vector<std::string> builtin_str = {"cd", "help", "exit"};

int (*builtin_func[]) (char **) = {
    &cd,
    &help,
    &exit
};

int builtins_size()
{
    return sizeof(builtin_str) / sizeof(char *);
}

int cd(char **args)
{
    if (args[1] == NULL) {
        std::cerr << "3sh usage: cd [path]" << '\n';
    }
    else {
        if (chdir(args[1]) != 0) {
            std::cerr << "lsh" << '\n';
        }
    }
    return 1;
}

int help(char **args)
{
    int i;
    std::cout << "3sh: lightweight shell " << '\n';
    std::cout << "usage: [command] [argument(s)] " << '\n';
    std::cout << "built-in commands:" << '\n';

    for (int i = 0; i < builtins_size(); i++) {
        std::cout << '\n' << builtin_str[i];
    }

    return 1;
}

int exit(char **args)
{
    return 0;
}

int execute(std::vector<std::string> tokens)
{
    // empty command
    if (tokens.empty()) {
        return 1;
    }

    for (const auto& s: builtin_str) {
        if (s == tokens[0]) {
            // run builtin_func for the given command
        }
    }
    
    return launch_ps(tokens);
}
