# yback, a CUI-based backup tool wrapping rsync

## Index

1. [Introduction](#introduction)
    1. [What can `yback` do in a nutshell?](#what-can-yback-do-in-a-nutshell)
    2. [Example](#syntax_file_example)
2. [Usage](#usage)
    1. [Command Usage](#command-usage)
    2. [Options](#options)
3. [File Management](#file-management)
4. [Config File Syntax](#config-file-syntax)
    1. [Example](#example_of_config_file_syntax)
    2. [Syntax](#syntax)
    3. [Globbing](#globbing)
5. [Installation](#installation)
    1. [Requirements](#requirements)
    2. [Build & Installation](#build_and_installation)
6. [Contributions](#contributions)

## Introduction

### What can `yback` do in a nutshell?

`yback` is a CUI-based backup tool which wraps [`rsync`](https://rsync.samba.org/). `yback` reads the specified config file(s) and dynamically creates commands to be executed. In the config file, you can write

- sources

- destinations

- destination specific options passed to `rsync`

- arbitrary shell commands

- variable definitions/references

- comments

- etc.

which lets you write large backup rules efficiently.

<a id="syntax_file_example"></a>
### Example

Assume you have the config file `config.txt` whose contents are as follows.

```bash
#Shell Commands
$ echo hello

#Variable Definitions
% source = < ~/Downloads/important
% option0 = -au
% option1 = --info=name
% option2 = --dry-run
% option3 = --backup
% option4 = --suffix=".$(date +%Y%m%d_%H%M%S)"

#Backup Unit
> ~/Desktop/backups
< ~/Downloads/important
% option0 option1 option3 option4

#Backup Unit
> /media/Disk/backups
% source
% option0 option3 option4
--info=name0,progress2

#Backup Unit
> ${USER}@${IP}:./sent/backups
% source
% option0 option1 option2
--info=stats
```

Let's just print the generated commands but not execute them.
```bash
~ $ export USER=mike
~ $ export IP=192.168.24.41
~ $ back --bkrc config.txt --show
$ echo hello

-> "/home/user/Desktop/backups"
["rsync", "-au", "--info=name", "--backup", "--suffix=.20200305_205823", "/home/user/Downloads/important", "/home/user/Desktop/backups"]
-> "/media/Disk/backups"
["rsync", "-au", "--backup", "--suffix=.20200305_205823", "--info=name0,progress2", "/home/user/Downloads/important", "/media/Disk/backups"]
-> "mike@192.168.24.41:./sent/backups"
["rsync", "-au", "--info=name", "--dry-run", "--info=stats", "/home/user/Downloads/important", "mike@192.168.24.41:./sent/backups"]
```

Simply removing `--show` actually executes the commands.
```bash
~ $ back --bkrc config.txt

[File Management]

$ echo hello
hello

-> "/home/user/Desktop/backups"
important/1.txt
important/2.txt
important/3.txt

-> "/media/Disk/backups"
              0 100%    0.00kB/s    0:00:00 (xfr#3, to-chk=0/4)

-> "mike@192.168.24.41:./sent/backups"
important/
important/1.txt
important/2.txt
important/3.txt

sent 135 bytes  received 29 bytes  328.00 bytes/sec
total size is 0  speedup is 0.00 (DRY RUN)
```

## Usage

### Command Usage

```bash
~ $ back --help
Usage
  back [<option(s)>] [-- [<rsync option(s)>]]

Options
  --bkrc,-b <file>              read <file> as a config file
  --file,-f <file>              copy <file> to `/tmp` before any other operations
  --tmp-directory <dir>         use <dir> instead of `/tmp` as the tmp directory
  --add <file>                  add <file> to the managed file list before any other operations
  --only-add <file>             add <file> to the managed file list and exit right away
  --managed-file-list <file>    use <file> instead of the default value as the managed file list
  --show,-s                     dry-run mode (i.e. just print but never execute commands)
  --dry-run,-n                  pass `--dry-run` option to `rsync`
  --help,-h                     show this help
  --                            pass all of the trailing arguments to `rsync` as options
```

### Options

- `-b/--bkrc <file>`

`<file>` is read by `yback` as a config file. See [Config File Syntax](#config-file-syntax) for the syntax of config files. **This option should be specified at least once.**

- `-f/--file <file>`

`<file>` is copied to the tmp directory before any other operations. The tmp directory is by default `/tmp`.

- `--tmp-directory <dir>`

`<dir>` is used as the tmp directory instead of the default `/tmp`.

- `--add <file>`

`<file>` is added to the managed file list `~/.yback_managed_file_list` before any other operations. See [File Management](#file-management).

- `--only-add <file>`

This is similar to `--add <file>` but terminates `yback` right after the addition. See [File Management](#file-management).

- `--managed-file-list <file>`

`<file>` is used as the managed file list instead of the default `~/.yback_managed_file_list`. See [File Management](#file-management).

- `-s/--show`

This turns on dry-run mode. In dry-run mode, everything is just printed but never actually executed.

- `-n/--dry-run`

This option is bypassed to `rsync`.

- `-h/--help`

This option shows the help message and exits.

- `--`

All of the command-line arguments after `--` are passed to `rsync`.

## File Management

*File Management* is the generalization of `-f` option. If a file `<file>` is *manage*d, it is equivalent to specify `-f <file>` every time you launch `yback`. File management works as below.

1. You can add files to the list of managed files only by using `--add` or `--only-add` option. Unless you specify `--managed-file-list` option, the list is stored in the file `~/.yback_managed_file_list`.

2. Every time you execute `yback`, the list is read and the files in it are automatically copied to the tmp directory. The tmp directory is `/tmp` by default, or the directory specified via `--tmp-directory` option.

## Config File Syntax

<a id="example_of_config_file_syntax"></a>
### Example

See [Example](#syntax_file_example).

### Syntax

In this section we use [EBNF](https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form) to explain the config file syntax.

```bash
rule = { question-spec | backup-unit-spec | load-spec | shell-command-spec | variable-spec | comment | empty-line } ;
```

`rule` is the most parent [production rule](https://en.wikipedia.org/wiki/Production_(computer_science)).

```bash
question-spec = '? ', message, newline-symbol ;
message = string ;
```

When `question-spec` is read, `message` is printed and a user input is requested. `yback` is cancelled unless you type "y".

```bash
backup-unit-spec = destination-spec {, ( rule - backup-unit-spec | source-spec | option-spec ) } ;

destination-spec = '> ', destination, newline-symbol ;
destination = non-empty-string ;

source-spec = '< ', source, newline-symbol ;
source = non-empty-string ;

option-spec = '-', non-empty-string, newline-symbol ;
```

`backup-unit-spec` is the most important. This specifies a destination, sources and options passed to `rsync`. You must specify at least one `source-spec` explicitly, or implicitly as the result of `variable-reference` (see below). Note, for example, you should specify `-verbose` as `non-empty-string` in `option-spec` to pass `--verbose` option to `rsync`.

```bash
load-spec = '+ ', filename, newline-symbol ;
filename = non-empty-string ;
```

`load-spec` specifies a config file which is started to be read at the very moment the `load-spec` is parsed. After reading `filename`, the control (the parser) goes back to the position right after the `load-spec`.

```bash
shell-command-spec = '$ ', shell-command, newline-symbol ;
shell-command = non-empty-string ;
```

`shell-command-spec` specifies an arbitrary shell command which is executed at the very moment the `shell-command-spec` is parsed. Internally, `shell-command` is executed using `system()`.

```bash
variable-spec = variable-definition | variable-reference ;
variable-definition = '% ', variable-name, ' = ', variable-value, newline-symbol ;
variable-reference = '% ', variable-name {, ' ', variable-name }, newline-symbol ;
variable-name = non-empty-string ;
variable-value = non-empty-string ;
```

`variable-spec` defines or references *variable*s. Variables here are specific to `yback` and totally irrelevant to shell (environment) variables. When you reference a variable `variable-name` via `variable-reference`, the corresponding `variable-value` is parsed as if it were explicitly written there.

```bash
comment = '#', string, new-line-symbol ;
```

A `comment` line is ignored.

```bash
empty-line = white-space-symbol, string, newline-symbol ;
```

An `empty` line is ignored.

```bash
white-space-symbol = ' ' | '\t' ;

newline-symbol = '\n' ;

string = (* any string *) ;

empty-string = '' ;

non-empty-string = string - empty-string ;

```

### Globbing

In the following contexts, shell patterns such as tilde expansion, parameter expansion, command substitution, and so on are properly interpreted. Quote an entity with single quotes if you don't want to perform shell expansions at all, or with double quotes if the result would contain whitespaces<sup>†</sup>. Note some kinds of expansions are not performed in double quotes (e.g. tilde expansion).

- `destination-spec`

- `source-spec`

- `option-spec`

- `load-spec`

- `shell-command-spec` (trivial)

<sub>†: Currently, except in the context of `shell-command-spec`, when an entity is expanded to multiple words, only the first one is used as the result. This means, for example, you cannot write `< /usr/local/*` to specify `/usr/local/bin`, `/usr/local/lib`, etc. as sources.</sub>

## Installation

### Requirements

- Linux

- [rsync](https://rsync.samba.org/)

- [g++](https://gcc.gnu.org/)

- [GNU Make](https://www.gnu.org/software/make/)

<a id="build_and_installation"></a>
### Build & Installation

- `make`

Build the program. 

- `make debug`

Same as `make` but with debug options.

- `make clean`

Remove all files created in the process of building the program. This operation doesn't remove the installed binary file. So doing this doesn't mean you cannot use `yback` any longer.

- `make prefix=<prefix> install`

Install the program under `<prefix>`. For the safety, `<prefix>` cannot be omitted.

- `make prefix=<prefix> uninstall`

Uninstall the installed program. This is completely irrelevant to `make clean`.

## Contributions

Contributions are all welcome.

When you would like to suggest a new feature or have found a bug, if you have a GitHub :octocat: account, please send a pull request or open an issue. If you don't, please send a mail :email: to [takahashi.manatsu@gmail.com](mailto:takahashi.manatsu@gmail.com).

Also, translating this document is very much appreciated.

<!-- vi: set spell: -->

