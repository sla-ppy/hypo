## Usage:

```
1. Compile with: clang src/server.c src/request.c src/util.c -o server -Wall -Wextra -pedantic -std=gnu99 -g -O0
```

```
2. Run with: ./server <port>
Where port is: 1024-65535
Refer to terminal if met with problems
```

Using -std=gnu99 proved to be necessary, as -std=c99 has issues with POSIX\
-g => generates debug info\
-O0 => genereate unoptimized code (optimization flag)

## Main status code table

| RC  | Meaning             |
| --- | ------------------- |
| -1  | initServer() failed |
