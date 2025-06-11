# -Mav-Shell-msh-A-Mini-Unix-Shell
A fully functional command-line shell written in C — because who doesn't want to build their own terminal from scratch?

#📘 Description
Mav Shell (msh) is a lightweight shell I built to better understand how Unix-style terminals work under the hood. Inspired by bash, csh, and ksh, this project was an experiment in low-level systems programming, process control, and interactive command execution.

It supports basic shell features like running commands, changing directories, piping, redirection, and a limited command history — all written in pure C using fork(), exec(), and wait().

#🔧 Features
Custom prompt: Displays msh> for user input

Command execution: Supports any command in /bin, /usr/bin, /usr/local/bin, and ./

Built-in commands:

cd — Change directories (including cd ..)

history — View the last 50 executed commands

!# — Re-run a command from history (e.g., !3)

exit or quit — Exit the shell cleanly

Command history: Keeps track of the last 50 non-blank commands

Redirection: Supports standard output redirection using >

Pipes: Supports single pipes between processes using |

Signal handling: Blocks SIGINT and SIGTSTP to prevent unwanted interruptions

Input handling: Silently ignores empty lines

#🚀 How to Run

gcc msh.c -o msh
./msh

#🧠 What I Learned
The inner workings of process creation with fork(), execvp(), and wait()

How shells manage redirection and pipelines

Command parsing and memory-safe buffer handling

How to block and handle Unix signals gracefully

Building a command history feature without external libraries

#📌 Notes
Redirection and pipes assume proper spacing (e.g., ls | grep foo, not ls|grep foo)

Appending and stderr redirection are not supported

Built-in commands like cd, exit, and history are handled separately from system calls

Debug output has been removed for clean interaction
