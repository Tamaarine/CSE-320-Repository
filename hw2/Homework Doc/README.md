# Homework 2 Debugging and Fixing - CSE 320 - Spring 2021
#### Professor Eugene Stark

### **Due Date: Friday 3/12/2021 @ 11:59pm**

# Introduction

In this assignment you are tasked with updating an old piece of
software, making sure it compiles, and that it works properly
in your VM environment.

Maintaining old code is a chore and an often hated part of software
engineering. It is definitely one of the aspects which are seldom
discussed or thought about by aspiring computer science students.
However, it is prevalent throughout industry and a worthwhile skill to
learn.  Of course, this homework will not give you a remotely
realistic experience in maintaining legacy code or code left behind by
previous engineers but it still provides a small taste of what the
experience may be like.  You are to take on the role of an engineer
whose supervisor has asked you to correct all the errors in the
program, plus add additional functionality.

By completing this homework you should become more familiar
with the C programming language and develop an understanding of:

- How to use tools such as `gdb` and `valgrind` for debugging C code.
- Modifying existing C code.
- C memory management and pointers.
- Working with files and the C standard I/O library.

## The Existing Program

Your goal will be to debug and extend an old program called `notation`,
which was written by Henry Thomas in 1990-1992 and posted to Usenet in early 1992.
The version I am handing out is very close to the original version,
except that I have made a few changes for this assignment.
First of all, I rearranged the source tree and re-wrote the `Makefile`
to conform to what we are using for the other assignments in this course.
I also introduced a few bugs here and there to make things more interesting
and educational for you :wink:.
Aside from these changes and the introduced bugs, which only involve a few
lines, the code is identical to the original, functioning version.

The purpose of the `notation` program is to read an input file containing the
transcript of a chess game in "algebraic" notation, and to write an output file
that consists of that game transcript, but possibly annotated with diagrams
showing the board at specified positions in the game.  The program is also able
to translate the transcript between various European languages (basically what
this means is that the letters that are used to denote the various chess
pieces in the input transcript are substituted with versions appropriate for
the target language).

What you have to do is to first get the program to compile (for the most part,
I did not modify the original code, which requires some changes for it
to compile cleanly with the compiler and settings we are using).
Then, you need to test the program and find and fix the bugs that prevent it
from functioning properly.  Finally, you will make some modifications to the
program.

As you work on the program, limit the changes you make to the minimum necessary
to achieve the specified objectives.  Don't rewrite the program;
assume that it is essentially correct and just fix a few compilation errors and
bugs as described below.  You will likely find it helpful to use `git` for this (I did).
Make exploratory changes first on a side branch (*i.e.* not the master branch),
then when you think you have understood the proper changes that need to be made,
go back and apply those changes to the master branch.  Using `git` will help you
to back up if you make changes that mess something up.

### Getting Started - Obtain the Base Code

Fetch base code for `hw2` as you did for the previous assignments.
You can find it at this link:
[https://gitlab02.cs.stonybrook.edu/cse320/hw2](https://gitlab02.cs.stonybrook.edu/cse320/hw2).

Once again, to avoid a merge conflict with respect to the file `.gitlab-ci.yml`,
use the following command to merge the commits:

<pre>
  git merge -m "Merging HW2_CODE" HW2_CODE/master --strategy-option=theirs
</pre>

  > :nerd: I hope that by now you would have read some `git` documentation to find
  > out what the `--strategy-option=theirs` does, but in case you didn't :angry:
  > I will say that merging in `git` applies a "strategy" (the default strategy
  > is called "recursive", I believe) and `--strategy-option` allows an option
  > to be passed to the strategy to modify its behavior.  In this case, `theirs`
  > means that whenever a conflict is found, the version of the file from
  > the branch being merged (in this case `HW2_CODE/master`) is to be used in place
  > of the version from the currently checked-out branch.  An alternative to
  > `theirs` is `ours`, which makes the opposite choice.  If you don't specify
  > one of these options, `git` will leave conflict indications in the file itself
  > and it will be necessary for you to edit the file and choose the code you want
  > to use for each of the indicated conflicts.

Here is the structure of the base code:

<pre>
.
├── .gitlab-ci.yml
└── hw2
    ├── doc
    │   ├── CHANGES
    │   ├── chesssymb.tex
    │   ├── convsymb.tex
    │   ├── convsymb.txt
    │   ├── COPYING
    │   ├── Makefile.orig
    │   ├── notation.doc
    │   ├── notation.n
    │   ├── README
    │   ├── symboles.tex
    │   ├── symboles.txt
    │   ├── symb.tex
    │   └── TODO
    ├── hw2.sublime-project
    ├── include
    │   ├── chesssymb.def
    │   ├── chesstype.h
    │   ├── debug.h
    │   ├── drivers.h
    │   ├── lexer.h
    │   └── notation.h
    ├── lib
    │   ├── Footer.ps
    │   ├── Header.ps
    │   ├── notation.hlp
    │   └── notation.tex
    ├── Makefile
    ├── rsrc
    │   ├── algebric.ntn
    │   ├── boudy.ntn
    │   ├── keywords.ntn
    │   └── shortened.ntn
    ├── src
    │   ├── drivers.c
    │   ├── lexer.c
    │   ├── lexer.l
    │   ├── main.c
    │   └── notation.c
    └── tests
        ├── hw2_tests.c
        ├── rsrc
        │   ├── quick.err
        │   ├── quick.out
        │   ├── valgrind_leak.err
        │   ├── valgrind_leak.out
        │   ├── valgrind_uninitialized.err
        │   └── valgrind_uninitialized.out
        ├── test_common.c
        └── test_common.h

</pre>

The `doc` directory included with the assignment basecode contains various files
that were distributed with the program.  One of these is a Unix-style `man` page
(`notation.n`).  You can format and read the `man` page using the following command:

<pre>
nroff -man doc/notation.n | less
</pre>

  > :nerd:  Since time immemorial, Unix `man` pages have been written in
  > a typesetting language called `roff` (short for "run-off").
  > The `nroff` program processes `roff` source and produces text formatted
  > for reading in a terminal window.

I corrected some errors in the original formatting commands to allow the document
to be processed more or less correctly with `groff` (the GNU re-engineered
version of `nroff`).  The file `notation.doc` contains what is presumably the
original result of running the then-current version of `nroff` on `notation.n`.

Other files in the `doc` directory are `Makefile.orig` (the original `Makefile`)
and various other files that were in the original distribution.  They should
not be necessary for doing the assignment.

The source files for the `notation` program are in the `include` and `src` directories.
The `include` directory contains the following:

- `chesssymb.def` -- Defines macros used in `src/drivers.c` and `src/notation.c`
in a clever way to construct tables of standard annotations that are used in
chess literature.
- `chesstype.h` -- Defines various macros and types that are mostly related to
the representation of chess boards, moves, and pieces in the program.
- `debug.h` -- Defines macros for debugging printout.  This was not in the
original distribution; it is the one we are using in this course.
- `drivers.h` -- Defines the interface to the output driver code found in
`src/drivers.c`, in a kind of object-oriented style.
- `lexer.h` -- `extern` declarations used by the lexical analyzer in `src/lexer.c`
to interface with the `flex` lexical analyzer generator package (more about this
later).
- `notation.h` -- Defines constants and function prototypes used by the code in
`src/notation.c` (the main program).

The `src` directory contains the following:

- `drivers.c` -- Various "output drivers" that are called to emit output
in various formats, such as plain text, LaTeX, Postscript, *etc*.
- `lexer.c` -- Generated lexical analyzer that is the result of processing
`src/lexer.l` with the `flex` lexical analyzer generator.
- `lexer.l` -- `flex` source code for the lexical analyzer. Basically gives
regular expressions for each kind of input "token" that the program will
recognize, and specifies actions to be performed each time a token is seen.
- `main.c` -- A file I added which contains only the `main()` function.
It calls `notation_main()` (the original `main()` function) in
`src/notation.c`.  This was done so that we can use Criterion tests with
the program (recall that Criterion uses its own `main()`).
- `notation.c` -- Contains the main body of the program.

The `lib` directory contains:

- `notation.hlp` -- Contains the help message that is printed out when the
program is invoked with the `-h` option.
- `Header.ps`, `Footer.ps`, `notation.tex` -- "boilerplate" header and footer
files inserted by the output drivers for the TeX and Postscript output formats.

The `rsrc` directory contains some files that can be used as test input
to the program:

- `algebric.ntn` -- Game using "algebraic" notation, no text.
- `boudy.ntn` -- Game using "shortened algebraic" notation, with variations, no text.
- `keywords.ntn` -- "Nonsense" file (no game), just demonstrating the use of keyword
commands to show the board and reconfigure the position.
- `shortened.ntn` -- Game using "shortened algebraic" notation, with interspersed text.

The `tests` directory contains C source code (in file `hw2_tests.c`) for some Criterion
tests that can help guide you toward bugs in the program.  These are not necessarily complete
or exhaustive.  The `test_common.c` and `test_common.h` contain auxiliary code used
by the tests.  The subdirectory `tests/rsrc` contains files that are used by the tests.

Before you begin work on this assignment, you should read the rest of this
document.  In addition, we additionally advise you to read the
[Debugging Document](DebuggingRef.md).

### Background Information - Chess Move Parsing

The core function of the program is to read a transcript of a chess game,
interpret the moves, and keep track of the current board position.
To simplify this process, the program makes use of a
*lexical analyzer generator* tool called `flex`.
A lexical analyzer is used to identify "tokens" in its input.
The `flex` tool is a GNU-re-engineered version of a traditional Unix
tool that was originally called `lex`.  The `flex` program reads an
input file that contains a set of *regular expressions* that define
the various forms that tokens can take, and it generates transition tables
for a finite automaton that can scan for occurrences of these patterns
in the input.  The core lexical analyzer code is always the same: it just
uses the table to simulate the finite automaton.  Each time a token is
identified in the input, an associated *action* is taken.
For example, if a token is identified that matches the form of a
chess move, then the associated action would be to update the current
board position.

In the `notation` program, the `flex` input file is `src/lexer.l`.
The file `src/lexer.c` is the output of `flex`, which contains the
generated lexical analyzer.  The `flex` tool is not installed by default
on your Linux Mint VM, and though it would be possible to install it,
it will not be necessary, because you will not need to make any changes
to `lexer.l` that would require regenerating the `lexer.c` file.
This information is being provided so you understand better the structure
of the program and you know you don't have to worry about what is going
on in `lexer.c`.

Actually, the regular expression given for a chess move in `src/lexer.l`
only defines things that *might be* a move.  Once a token matching
this pattern has been identified, the lexical analyzer calls the function
`parse_move()` in `src/notation.c`.  This function also uses a table-driven
scheme (see `transit` and `action`) to determine if the token really does
legitimately represent a chess move.  You might have to look at the
`parse_move()` function, but you do not need to understand the contents
of the tables that it uses.

# Part 1: Debugging and Fixing

The command line arguments and expected operation of the program are described
by the following "Usage" message, which is printed within `notation_main()`
in `notation.c` (the actual text of the message is read from the file
`lib/notation.hlp`):

```
Command line options: 
<inputfile>	: specify the input file. If none, keyboard is assumed
-a		: algebraic move notation output
-s		: shortened move notation output
-f <language>	: specify the chess symbol input language: french,
	english, italian, spanish, german, dutch, czech, hungarian, polish,
	romanian, FIDE. (for portuguese, use spanish)  
-t <language>	: specify the chess symbol output language (same
options as input language).
-o <outputfile>	: specify the output file. If none, screen is assumed
-c <number>[,<number]	: specify the number of the moves to display
	the board. if none, board is diplayed at end of file
-e <number>     : display the board at the move number and then stops
-b		: display only the board, not the moves
-d <drivername>	: specify the output driver: ascii,postscript, tex
(latex), roff, xchess (xchess save format), gnu (gnuan input format) 
-i              : suppress headers/footer in output (useful
		for tex/ps drivers)
-h		: shows this help
-v		: shows version number
```

  > :anguished: What is now the function `notation_main()` was simply `main()` in the original code.
  > I have changed it so that `main()` now resides in a separate file `main.c` and
  > simply calls `notation_main()`.  This is to make the structure conform to what is
  > needed in order to be able to use Criterion tests with the program.  **Do not make
  > any modifications to `main.c`.**

There are a few options that the program understands:

- If `-a` is specified, then the output of the program will use "long" algebraic notation;
- If `-s` is specified, then the output of the program will use "short" algebraic notation;
- If `-f` is specified, followed by an argument `<language>`, then the program will
use the specified language to interpret the letters used in the input to represent the chess pieces;
- If `-t` is specified, followed by an argument `<language>`, then the program will
use the specified language to determine the letters used in the output to represent the chess pieces;
- If `-o` is specified, followed by a filename, then the program will print output to
the specified file instead of the default `stdout`.
- If `-c` is specified, then the next argument gives a comma-separated list of moves
after which the board should be displayed.
- If `-e` is specified, then the next argument gives a move number after which to display
the board and exit.
- If `-d` is specified, then the next argument gives the name of the output driver to
be used in generating the output.
- If `-i` is specified, then header and footer "boilerplate" that would normally be inserted
at the beginning and end of output files is omitted.
- If `-h` is specified, then a help message is printed;
- If `-v` is specified, then the program exits after printing the version number;

You are to complete the following steps:

1. Clean up the code; fixing any compilation issues, so that it compiles
   without error using the compiler options that have been set for you in
   the `Makefile`.
   Use `git` to keep track of the changes you make and the reasons for them, so that you can
   later review what you have done and also so that you can revert any changes you made that
   don't turn out to be a good idea in the end.

2. Fix bugs.

    Run the program, exercising the various options, and look for cases in which the program
    crashes or otherwise misbehaves in an obvious way.  We are only interested in obvious
    misbehavior here; don't agonize over program behavior that might just have been the choice
    of the original author.  You should use the provided Criterion tests to help point the way,
	though they are not guaranteed to be exhaustive.

3. Use `valgrind` to identify any memory leaks or other memory access errors.
   Fix any errors you find.

    Run `valgrind` using a command of the following form:

    <pre>
      $ valgrind --leak-check=full --show-leak-kinds=all [NOTATION PROGRAM AND ARGS]
    </pre>

    Note that the bugs that are present will all manifest themselves in some way
    either as incorrect output, program crashes or as memory errors that can be
	detected by `valgrind`.  It is not necessary to go hunting for obscure issues
	with the program output.
    Also, do not make gratuitous changes to the program output, as this will
    interfere with our ability to test your code.

   > :scream:  Note that we are not considering memory that is "still reachable"
   > to be a memory leak.  This corresponds to memory that is in use when
   > the program exits and can still be reached by following pointers from variables
   > in the program.  Although some people consider it to be untidy for a program
   > to exit with "still reachable" memory, it doesn't cause any particular problem.

   > :scream: You are **NOT** allowed to share or post on PIAZZA
   > solutions to the bugs in this program, as this defeats the point of
   > the assignment. You may provide small hints in the right direction,
   > but nothing more.

# Part 2: Changes to the Program

## Rewrite/Extend Options Processing

The base code performs options processing as part of the function `notation_main()`
(which was the `main()` function in the original program).  It uses *ad hoc*
techniques to process the arguments, which are typical of what many C programs
might do.  However, as options processing is a common function that is performed
by most programs, and it is desirable for programs on the same system to be
consistent in how they interpret their arguments,
there have been more elaborate standardized libraries that have been written
for this purpose.  In particular, the POSIX standard specifies a `getopt()` function,
which you can read about by typing `man 3 getopt`.  A significant advantage to using a
standard library function like `getopt()` for processing command-line arguments,
rather than implementing *ad hoc* code to do it, is that all programs that uses
the standard function will perform argument processing in the same way
rather than having each program implement its own quirks that the user has to
remember.

For this part of the assignment, you are to replace the original argument-processing
code in `notation_main()` by code that uses the GNU `getopt` library package.
In addition to the POSIX standard `getopt()` function, the GNU `getopt` package
provides a function `getopt_long()` that understands "long forms" of option
arguments in addition to the traditional single-letter options.
In your revised program, `notation_main()` should use `getopt_long()` to traverse the
command-line arguments, and it should understand the following alternative forms
for the various options (as well as the original short forms):


  - `--long-algebraic` as equivalent to `-a`
  - `--short-algebraic` as equivalent to `-s`
  - `--input-language` as equivalent to `-f`
  - `--output-language` as equivalent to `-t`
  - `--output-file` as equivalent to `-o`
  - `--show-after` as equivalent to `-c`
  - `--end-after` as equivalent to `-e`
  - `--board-only` as equivalent to `-b`
  - `--driver` as equivalent to `-d`
  - `--no-headers` as equivalent to `-i`
  - `--help` as equivalent to `-h`
  - `--version` as equivalent to `-v`

You will probably need to read the Linux "man page" on the `getopt` package.
This can be accessed via the command `man 3 getopt`.  If you need further information,
search for "GNU getopt documentation" on the Web.

> :scream: You MUST use the `getopt_long()` function to process the command line
> arguments passed to the program.  Your program should be able to handle cases where
> the (non-positional) flags are passed IN ANY order.  Make sure that you test the
> program with prefixes of the long option names, as well as the full names.

## Free Memory

The original program uses `malloc()` to allocate memory for things like
the game board, the list of moves in the main line of play, and lists of
moves in sub-variations.  However, the program does not always free this memory.
Your job is to modify the program so that all the memory it allocates is freed
before the program terminates.  This should be true whether or not the program
terminates normally or with an error.  In order to accomplish this, you will
have to study how the program uses the memory that it allocates, and determine
where to introduce calls to `free()` in order to free this memory.
You will probably not find it very easy to get this right, so consider it
a challenge.  You will also need to call the function `yylex_destroy()` before
exiting, in order to free memory that was allocated by `flex`.
If you are successful with these modifications, when you run the program with
`valgrind` it will not report any memory that is "lost" or any memory that
is "still reachable" at the end of execution.

# Part 3: Testing the Program

For this assignment, you have been provided with a basic set of
Criterion tests to help you debug the program.

In the `tests/hw2_tests.c` file, there are five test examples.
You can run these with the following command:

<pre>
    $ bin/notation_tests
</pre>

To obtain more information about each test run, you can supply the
additional option `--verbose=1`.

The tests have been constructed so that they will point you at most of the
problems with the program.
Each test has one or more assertions to make sure that the code functions
properly.  If there was a problem before an assertion, such as a "segfault",
the test will print the error to the screen and continue to run the
rest of the tests.
Two of the tests use `valgrind` to verify that no memory errors are found.
If errors are found, then you can look at the log file that is left behind
(in the `tests.out` directory) by the test code.
Alternatively, you can better control the information that `valgrind` provides
if you run it manually.

The tests included in the base code are not true "unit tests", because they all
run the program as a black box using `system()`.
You should be able to follow the pattern to construct some additional tests of
your own, and you might find this helpful while working on the program.
You are encouraged to try to write some of these tests so that you learn how
to do it.  Note that in the next homework assignment unit tests will likely
be very helpful to you and you will be required to write some of your own.
Criterion documentation for writing your own tests can be found
[here](http://criterion.readthedocs.io/en/master/).

  > :scream: Be sure that you test non-default program options to make sure that
  > the program does not crash or otherwise misbehave when they are used.

# Hand-in Instructions

Ensure that all files you expect to be on your remote repository are committed
and pushed prior to submission.

This homework's tag is: `hw2`

<pre>
$ git submit hw2
</pre>
