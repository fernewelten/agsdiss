# agsdiss
Quick-and-dirty AGS Bytecode disassembler, for use in AGS compiler Googletests 

The “new” AGS compiler features a lot of Googletests where snippets of AGS code are compiled and the resulting code is recorded in `C++` arrays, as a sequence of integers.
Whenever the compiler changes in such a way that those snippets compile differently, the Googletests will fail and thus alert about this fact.

It is difficult to make sense of the bytecode if you just see the bytes, so here is a disassembler that takes bytes as standard input and writes assembly to standard output.

Here's a typical excerpt of such a Googletest:
```
    int32_t code[] = {
      36,    6,   38,    0,           36,    7,   51,    0,    // 7
      63,  200,    1,    1,          200,   36,    8,   51,    // 15
       0,   63,    1,    1,            1,    1,   36,    9,    // 23
       6,    2,    4,    3,            2,    3,    2,    1,    // 31
     201,    5,  -999
    };
```

Feed the text **excluding the first line** as standard input into the disassembler.

The disassembler will stop at the first error it encounters. Usually, this happens to be the very last `-999`, because that isn't a legal opcode. Don't mind this, because the `-999` is a sentinel value that's at the end of the array intentionally. 

When a Bytecode Googletest fails, the old code that was previously generated is part of the googletest. You can call the Googletest in such a way that the new code is generated to a file. Convert the old code and the new code to assembly format and use an editor such as Notepad++ to highlight the assembly instructions that have changed. 