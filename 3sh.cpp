#include <iostream>
#include <vector>

void cmd_loop();
void read_line();
void split_line(char *);
void execute(char **);

int main(int argc, char **argv)
{
    read_line();
    return EXIT_SUCCESS;
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

void read_line()
{
    std::vector<char> buf;
    std::vector<char> *pbuf = &buf;

    // if we hit EOF or nl, replace it with a null character and return
    int c = std::cin.get();
    while (c != EOF && c != '\n') {
        buf.push_back(c);
        c = std::cin.get();
    }
    buf.push_back('\0');

    // print contents of buf
    std::cout << "buf: [";
    for (int i = 0; i < buf.size(); i++) {
        int ch = buf[i];
        std::cout << ch;
        if (i != buf.size()-1) {
            std::cout << ", ";
        }
    }
    std::cout << "]\n";

}

void split_line(char *c)
{
}

void execute(char **c)
{
}
