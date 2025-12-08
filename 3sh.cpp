#include <iostream>
#include <vector>
#define SH_RL_BUFSIZE 1024

void sh_loop();
void sh_read_line();
void sh_split_line(char *);
void sh_execute(char **);

int main(int argc, char **argv)
{
    sh_read_line();
    return EXIT_SUCCESS;
}

void sh_loop()
{
    char *line;
    char **args;
    int status;

    do {
        std::cout << "> ";
        //line = sh_read_line();
        //args = sh_split_line(line);
        //status = sh_execute(args);

        delete line;
        delete args;
    }
    while (status);
}

void sh_read_line()
{
    std::vector<char> buf;
    std::vector<char> *pbuf = &buf;

    // if we hit EOF, replace it with a null character and return
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

void sh_split_line(char *c)
{
}

void sh_execute(char **c)
{
}
