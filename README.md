# Lambda Programming Language

It's _Lisp_ like programming language with REPL mode.

## Quick Start

Load repositry and
```console 
$ ./buildInit.sh
$ ./bin/build
$ ./bin/lambda
```
## Api 

All language constrcutions begins and ends from `()` - _S-expresions_ or _Context_. Repl mode can send back objecst: _Integers_, _Floats_ and _Strings_. Also it can evaluate arethmetic expressions (only `+ - * /`).

Example:

``` lisp

> (12)
12

> (12.9)
12.9

> ("Hello, World!")
Hello, World!

> (* (/ 342 2) (* 2 (+ 1 4)))
1710

```
