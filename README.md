# Unix command shell

A command shell implemented in plain C using Linux system calls.

## Features:
- Pipes
- Internal commands, like `exit`, `cd`, `source`,  and `echo`.
- Input/output redirection
- Support for `Ctrl + C`
- `;` for command separation
- `&` to put process in background

## Instructions

Build the project using:

```
gcc a.c -Wall -Wextra -o shell
```

Then run the shell with this command:
```
./shell
```

The shell should be up and running!
