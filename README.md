# JIT Repository Manager

This project implements a simple JIT (Just-In-Time) repository management system inspired by Git. It allows you to
initialize a repository, manage commits, branches, and diffs, and perform other Git-like operations through the command
line.

## Features

- Initialize a JIT repository in a directory.
- Add files to the JIT repository.
- Commit changes with a message.
- Create and checkout branches.
- View commit logs and status.
- Merge branches.
- Diff between commits or branches.

## Prerequisites

- C++20 or later.
- Standard C++ libraries.
- CMake for building the project.

## Build Instructions

1. Clone the repository.
2. Create a build directory and navigate to it:
    ```bash
    mkdir build
    cd build
    ```
3. Run CMake to generate build files:
    ```bash
    cmake ..
    ```
4. Build the project:
    ```bash
    make
    ```

## Usage

The `jit` command is used to interact with the repository. Below are the available commands:

### `init`

Initializes a new JIT repository in the current directory.

```bash
jit init
```

### `add <filename>`

Adds a file to the JIT repository for tracking.

```bash
jit add <filename>
```

### `commit <message>`

Commits the changes with a provided message.

```bash 
jit commit "Your commit message"
```

### `checkout -b <branch-name>`

Creates a new branch with the specified name and checks it out.

```bash
jit checkout -b <branch-name>
```

### `checkout <commit/branch>`

Checks out a specific commit or branch.

```bash
jit checkout <commit-id/branch-name>
```

### `status`

Displays the status of the JIT repository, including tracked files and changes.

```bash
jit status
```

### `log`

Displays the commit log of the JIT repository.

```bash
jit log
```

### `merge <branch-name>`

Merges the specified branch into the current branch.

```bash
jit merge <branch-name>
```

### `branch`

Lists all branches in the JIT repository.

```bash
jit branch
```

### `diff [<branch-name>]`

```bash
diff branch1..branch2
```

Shows the difference between the current commit and the specified branch or the latest commit.

```bash
jit diff
jit diff <branch-name>
```

## Project Structure

- DirectoryManagent/: Contains the DirManager class responsible for managing the directory and initializing the JIT
  repository.
- ChangesManagement/: Contains the JitActions class responsible for handling Git-like actions such as commits, branches,
  diffs, and merges.
- main.cpp: The main entry point of the program, which processes commands and interacts with the DirManager and
  JitActions classes.