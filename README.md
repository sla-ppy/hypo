## Usage:

Note#1: using -std=gnu99 proved to be necessary, as -std=c99 has issues with POSIX
Note#2: -g => generates debug info and -O0 => genereate unoptimized code (optimization flag)

```
Compile with: clang src/server.c -o server -Wall -Wextra -pedantic -std=gnu99 -g -O0
```
