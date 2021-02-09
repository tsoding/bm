# Contributing guidelines

### Formatting

The code is expected to be formatted with [astyle](http://astyle.sourceforge.net/) with the following flags:

```console
$ astyle --style=kr <file>
```

The style is automatically checked on the CI of the project. Use `./nobuild fmt` to format all the source code according to the expected style. Make sure `astyle` is available via `$PATH` environment variable.
