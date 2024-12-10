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
- CMake 3.16.3 or higher.
- conan

## Build Instructions

1. Clone the repository
2. Create a build directory and navigate to it:
    ```bash
    mkdir build
    cd build
    ```
3. You may choose to use Conan or build directly by installing openssl and zlib on a linux machine
    1. On Linux
        1. Install the following packages:
         ```bash
           sudo apt update
           sudo apt install libssl-dev zlib1g-dev
         ```
        2. Run cmake
         ```bash
         cmake ..
         ```
    2. Using Conan:
        1. Make sure Conan is installed in your system and correctly set in your path
        2. Detect the project profile using conan:
         ```bash
          conan profile detect --force
         ```
        3. Get out of build directory using `cd` to the project's directory
        4. Install the packages; openssl and zlib using the following command:
         ```bash
         conan install . --output-folder=build --build=missing
         ```
        5. Navigate to the build directory
         ```bash
         cd build
         ```
        6. Build the project:
           For Linux:
         ```bash
         cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
         ```
       For Windows:
         ```bash
          cmake .. -G "Visual Studio 15 2017" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
         ```
4. Now that build files are correctly set up by cmake using any of the above two methods, let's build the executable:

```bash
cmake --build .
```

**A file `Jit` will be created in the build directory. This is the executable. You can reference it as a path while
running commands or set it up in your path variable.**

## Assumptions and compromises

1. Ignored branch creation via `Jit branch <branch_name>` and only implemented creation vis
   `Jit checkout -b <branch_name>`
2. Stashing changes using `Jit stash` is not implemented.
3. For `Jit clone` only path-based and branch-based clones were implemented. `--depth` not implemented.
4. Merging is only implemented for branches not commits.
5. To ignore files, the file `.jitignore is used`.

## Usage

The `jit` command is used to interact with the repository. Below are the available commands:

### `init`

Initializes a new JIT repository in the current directory.

```bash
Jit init
```

### `add <filename>`

Adds a file to the JIT repository for tracking.

```bash
Jit add <filename>
```

### `commit <message>`

Commits the changes with a provided message.

```bash 
Jit commit "Your commit message"
```

### `checkout -b <branch-name>`

Creates a new branch with the specified name and checks it out.

```bash
Jit checkout -b <branch-name>
```

### `checkout <commit/branch>`

Checks out a specific commit or branch.

```bash
Jit checkout <commit-id/branch-name>
```

### `status`

Displays the status of the JIT repository, including tracked files and changes.

```bash
Jit status
```

### `log`

Displays the commit log of the JIT repository.

```bash
Jit log
```

### `merge <branch-name>`

Merges the specified branch into the current branch.

```bash
Jit merge <branch-name>
```

### `branch`

Lists all branches in the JIT repository.

```bash
Jit branch
```

### `diff [<branch-name>]`

```bash
diff branch1..branch2
```

Shows the difference between the current commit and the specified branch or the latest commit.

```bash
Jit diff
Jit diff <branch-name>
```

### `clone`
Clones an existing repository
usage:
```bash
Jit clone <repository_to_be_cloned>
Jit clone <repository_to_be_cloned> <target_directory>
Jit clone --branch <branch_name> <repository_to_be_cloned>
Jit clone --branch <branch_name> <repository_to_be_cloned> <target_directory>
```

## Project Structure

- DirectoryManagement/: Contains the `DirManager` class responsible for managing the directory and initializing `Jit`.
  Updating the repository in case of a checkout, keeping track of the current directory, and getting a list of files to
  be tracked while ignoring those in `jitignore`.
  Repository.
- ChangesManagement/: This is where all changes including commits, branching, merging, diff, tracking repository status,
  performing `jit add`, Handling the index file, etc.
- CommitManagement/: The commit graph is stored here.
- JitUtility/: contains functions necessary in the project such as compressing files, decompressing them, date
  conversion, etc. Functions that are independent of any class.
- main.cpp: The main entry point of the program, which processes commands and interacts with the `DirManager` and
  `JitActions` classes.

## Working

- The main function initializes the two main classes `DirManager` and `JitActions`.
- It also handles the calling of the respective function based on the action that needs to be carried out.
- Both classes are initialized with the root directory.
- In case of an init, `DirManager` is used to initialize the jit. This is done by creating the necessary directories i.e
  log, refs, objects as well as the head file.
- The ref's directory stores all the branch files containing the head of each branch.
- The logs directory contains a log of activities in the branches.
- The `HEAD` file contains the global repository head.
- The index file is used to track changes in the repository.
- It is created upon running the first `jit add` function.
- The `IndexFileParser` is responsible for the creation and serialization of this file.
- Upon commit, the index file is checked for any tacked changes, if none is present, the commit fails, if changes are
  there,
  the commit begins.
- The commit involves converting all tracked files to compressed binary files using zlib and copying them to the objects
  file.
- To store them, a checksum of the file is calculated using the openssl `SHA1` algorithm and it is used to generate the
  committed file.
- A copy of the index file is also compressed, its SHA1 is calculated and stored in the objects directory too. This
  eases
  the extraction of files later.
- The commit tree is updated with the parents being set from the previous commit, if any.
- The commit address i.e. the sha, is the same as that commits index file sha. This eases the retrieval of the index
  file.
- On checkout, the branch is first checked to ensure it is not dirty. If it is clean, the application gets the files
  tracked, deletes all, reads the commits' index file, decompresses all files referenced there, and writes them in the
  directory.
- When calculating the diff, the application takes the two commits, checks which files differ in the checksum, and uses
  the Longest Common Subsequence (LCS) algorithm to get the difference between the files.
- This also helps to check for conflicts.
- While cloning, if it is a full clone, the whole `.jit` directory is copied to the current directory, and the status of
  the repository is updated based on the head file. If it is a branch clone, commits for that branch are extracted from
  the log file and only the files in those commits are copied to the objects folder.