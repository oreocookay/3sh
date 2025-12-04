#include <iostream>
#include <vector>
#define SH_RL_BUFSIZE 1024

void sh_loop();
void sh_read_line();
char **sh_split_line(char *);
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
        args = sh_split_line(line);
        //status = sh_execute(args);

        delete line;
        delete args;
    }
    while (status);
}

void sh_read_line()
{
    int bufsize = SH_RL_BUFSIZE;
    std::vector<char> buf(bufsize);
    int pos = 0;
    int c;

    while (true) {
        // read a character
        std::cin >> c;

        // if we hit EOF, replace it with a null character and return
        if (c == EOF || c == '\n') {
            buf.push_back('\0');
            //return buf;
            return;
        }
        else {
            buf.push_back(c);
        }
        std::cout << "buf: " << buf[pos] << '\n';
        pos++;
    }
}

char **sh_split_line(char *c)
{
}

void sh_execute(char **c)
{
}
