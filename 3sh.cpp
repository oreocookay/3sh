#include <iostream>
#include <vector>
#include <string>
#include <sstream>

void cmd_loop();
std::string read_line();
void split_line(std::string);
void execute(char **);

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

void split_line(std::string line)
{
    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token) {
        tokens.push_back(token);
    }

    for (auto& t: tokens) {
        std::cout << t << '\n';
    }
}

void execute(char **c)
{
}

