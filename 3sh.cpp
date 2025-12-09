#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>

void cmd_loop();
std::string read_line();
std::vector<std::string> split_line(std::string);
void execute(char **);
int launch_ps(char **args);
// prototypes for built-in shell commands
int cd(char **args);
int help(char **args);
int exit(char **args);

int main(int argc, char **argv)
{
    std::string line = read_line();
    split_line(line);
    return 0;
}

void cmd_loop()
{
    char *line;
    char **args;
    int status;

    do {
        std::cout << "> ";
        //line = read_line();
        //args = split_line(line);
        //status = execute(args);

        delete line;
        delete args;
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

int launch_ps(char **args)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // child process
        if (execvp(args[0], args) == -1) {
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

void execute(char **c)
{
}

