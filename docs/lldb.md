# LLDB

The choice of LLDB is arbitrary, one may very well use GDB. I use LLDB simply because it comes with my macbook, and I really like the vscode extension lldb-dap.

First and foremost, this article is an exploration of using the debugger for visualizing data structures, algorithms, and their interplay. Record-and-replay, snapshots, time travelling, hot reload, etc. are also of interest but will be pushed to the bottom of this article.

According to my experience, 3 classes of languages have the best debugging experience:
1. Languages that get compiled to native code: C, C++, D, Ada, Rust, Zig
    - The CPU and OS kernel is your debugger; CPU might as well be short for the "C processing unit". Arguably the best debugging support.
2. Well-defined interpreted languages: Java, Python, JavaScript, Julia?
    - Java: Intellij stop-the-world debugger.
    - Python: pdb with post-mortem debugging.
    - JavaScript: browsers and VSCode built-in.
    - Julia: never used it, but LLVM-JIT might be very interesting.
3. (Almost-purely) functional programming languages: Erlang/BEAM
    - Erlang/BEAM: the whole runtime is designed with debugging in mind.

## [`.lldbinit`](https://lldb.llvm.org/man/lldb.html#configuration-files)

- [LLDB Custom Commands](https://gist.github.com/brennanMKE/88ec8d07ddb148456bb3619e2e494cdd)
    - [facebook/chisel](https://github.com/facebook/chisel)

## [Variable Formatting](https://lldb.llvm.org/use/variable.html)

### [Data Formatters](https://lldb.llvm.org/resources/dataformatters.html)

### [Formatter Bytecode](https://lldb.llvm.org/resources/formatterbytecode.html)
> To use custom data formatters, developers need to edit the global `~/.lldbinit` file to make sure they are found and loaded. In addition to this rather manual workflow, developers or library authors can ship ship data formatters with their code in a format that allows LLDB automatically find them and run them securely.

## [Python Scripting](https://lldb.llvm.org/use/python.html)

## [Scripting Bridge API](https://lldb.llvm.org/resources/sbapi.html)

## [On Demand Symbols](https://lldb.llvm.org/use/ondemand.html)

Load only necessary debug symbols on demand.

> This feature works by selectively enabling debug information for modules that the user focuses on. It is designed to be enabled and work without the user having to set any other settings and will try and determine when to enable debug info access from the modules automatically.

> Since most users set breakpoints by file and line, this is an easy way for people to inform the debugger that they want focus on this module.

## [Symbolication](https://lldb.llvm.org/use/symbolication.html)

> LLDB can be used to symbolicate your crash logs and can often provide more information than other symbolication programs:
>
> 1. Inlined functions
> 2. Variables that are in scope for an address, along with their locations

## References

- [hediet/vscode-debug-visualizer](https://marketplace.visualstudio.com/items?itemName=hediet.debug-visualizer)
- StackOverflow, [Non blocking pyplot GUI for GDB python pretty printer](https://stackoverflow.com/q/24924357)
    - [Analyzing C/C++ matrix in the gdb debugger with Python and Numpy](https://www.codeproject.com/Articles/669606/Analyzing-C-Cplusplus-matrix-in-the-gdb-debugger-w)
- [What a good debugger can do](https://werat.dev/blog/what-a-good-debugger-can-do/)
    - Breakpoints
        - [LLDB Column Breakpoints](https://jonasdevlieghere.com/post/lldb-column-breakpoints/)
        - Conditional breakpoints
        - [Tracing breakpoints (or tracepoints)](https://devblogs.microsoft.com/visualstudio/tracepoints/)
        - Data breakpoints
    - Data visualization
        - Visual Studio with Image Watch.
    - Expression evaluation
    - Concurrency and multithreading
        - [How to debug deadlocks in Visual Studio](https://werat.dev/blog/how-to-debug-deadlocks-in-visual-studio/)
        - Freeze and unfreeze threads.
        - Visual Studio Parallel Stacks to visualize thread dependency and states.
    - Hot reload
    - Time travel
        - [rr](https://rr-project.org)
        - [Tomorrow Corporation Tech Demo](https://www.youtube.com/watch?v=72y2EC5fkcE)
    - Omniscient debugging
        - Kyle Huey, [Debugging by querying a database of all program state](https://www.hytradboi.com/2022/debugging-by-querying-a-database-of-all-program-state)
        - Robert O'Callahan (author of rr), [The State Of Debugging in 2022](https://www.youtube.com/watch?v=yCK0-vWmAsk)
        - Bil Lewis, [Debugging Backwards in Time (2003)](http://www.cs.kent.edu/~farrell/mc08/lectures/progs/pthreads/Lewis-Berg/odb/AADEBUG_Mar_03.pdf)
        - [Pernosco](https://pernos.co/)
        - [WhiteBox](https://whitebox.systems/)

## Ideas

- Change memory/registers live and branch.
- cling, llvm-repl
- dlopen, dlsym
