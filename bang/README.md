# Bang Programming Language

![logo](./logo/logo-256.png)

## Quick Start

```nim
$ cc -o nobuild nobuild.c
$ ./nobuild
$ cat <<EOF >hello.bang
proc main() {
    write("Hello, World\n");
}
EOF
$ ./bin/bang run hello.bang
```

For more information do `./bin/bang help`
