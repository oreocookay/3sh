## 3sh: lightweight shell ⚡︎
3sh is a small shell written in C++ and designed for Linux and macOS. it uses the GNU readline library, which offers nice features for navigating the command line using shortcuts and storing command history. you will find many of the same quality-of-life features as in bash, such as `reverse-i-search`  to query recent commands and `ctrl-l` to clear the screen. it currently weighs in at 180KB

*3sh supports the following*:
- built-in commands (`cd`, `help`, `exit`, `history`)
- execution of external programs via `fork()`, `execvp()`, and `wait()`
- special variable expansion (`~`: home directory, `-`: previous directory, `$?`: last command's exit status)
- double-quoted string parsing
- signal handling (`SIGINT`/`^C`, `EOF`/`^D`)
- command history (stored in `~/.3sh_history`)
- automatic color for `ls` and `grep`
- pipes and redirections of the following form: 

    `cmd1 | cmd2`

    `cmd1 > cmd2`

    `cmd1 >> cmd2`

    `cmd1 | cmd2 > outfile`

    `cmd1 | cmd2 >> outfile`

*3sh does not yet support*:
- job control

- wildcards/globbing
- single-quote parsing
- input and standard error redirection
- pipe chaining

## build
clone the repository
```bash
$ git clone https://github.com/oreocookay/3sh.git ~/3sh
$ cd 3sh
```

### macOS
macOS comes with BSD libedit, which is not what we want. instead, install GNU readline and link against it

```bash
$ brew install readline
$ g++ 3sh -Wall -I /opt/homebrew/opt/readline/include 3sh.cpp -o 3sh -L /opt/homebrew/opt/readline/lib -lreadline
$ ./3sh
```
---
### linux
install the `g++` and `libreadline-dev` packages

```bash
$ sudo apt install g++ readline
$ g++ 3sh.cpp -o 3sh -lreadline
$ ./3sh
```

## optional: add to PATH
```bash
PATH=~/3sh:$PATH
```
