# shell24

`shell24` is a custom shell program written in C that goes into an infinite loop waiting for user commands. It assembles and executes each command using `fork()`, `exec()`, and other system calls as required.

## Features

- **Infinite Loop**: The shell waits for user commands indefinitely.
- **Command Execution**: Executes user commands using system calls.
- **Special Characters Handling**:
  - **Text File Concatenation (#)**: Concatenate up to 5 text files.
  - **Piping (|)**: Supports up to 6 piping operations.
  - **Redirection (>, <, >>)**: Supports input/output redirection.
  - **Conditional Execution (&&, ||)**: Supports up to 5 conditional execution operators.
  - **Background Processing (&)**: Execute commands in the background and bring them to the foreground.
  - **Sequential Execution (;)**: Execute up to 5 commands sequentially.

## Rules and Conditions

### Rule 1
The program/command `newt` (`shell24$newt`) must create a new copy of `shell24`. There is no upper limit on the number of new `shell24` terminal sessions that can be opened.

### Rule 2
The `argc` (includes the name of the executable/command) of any command/program should be `>=1` and `<=5`.

#### Examples
- `shell24$ date` (argc = 1)
- `shell24$ ls -1 -l -t ~/chapter5/dir1` (argc = 5)
- `shell24$ cat input1.txt input2.txt` (argc = 3)

### Rule 3
The `argc` of individual commands or programs that are used along with the special characters listed below should be `>=1` and `<=5`.

#### Examples
- `shell24$ ls -l -t | wc` 
  - The first command has `argc = 3` and the second command has `argc = 1`.

## Special Characters

- **# Text File Concatenation**: 
  - Example: `shell24$ check.txt # new.txt # new1.txt # sample.txt`
  - Files are concatenated in the listed order, and the final result is displayed on stdout.
- **| Piping**: 
  - Example: `shell24$ ls | grep *.c | wc | wc -w`
  - Supports up to 6 piping operations.
- **>, <, >> Redirection**: 
  - Example: `shell24$ cat new.txt >> sample.txt`
- **&& Conditional Execution**: 
  - Example: `shell24$ ex1 && ex2 && ex3 && ex4`
  - Example: `shell24$ c1 && c2 || c3 && c4`
- **|| Conditional Execution**: 
  - Similar usage as `&&`.
- **& Background Processing**: 
  - Example: `shell24$ ex1 &` (runs `ex1` in the background)
  - Example: `shell24$ fg` (brings the last background process to the foreground)
- **; Sequential Execution**: 
  - Example: `shell24$ ls -l -t ; date ; ex1 ;`
  - Supports up to 5 sequential commands.

## Installation

1. Clone the repository:
    ```sh
    git clone https://github.com/yourusername/shell24.git
    ```
2. Navigate to the project directory:
    ```sh
    cd shell24
    ```
3. Compile the program:
    ```sh
    gcc -o shell24 shell24.c
    ```

## Usage

Run the shell:
```sh
./shell24
