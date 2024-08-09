# Code Style Check

## Checking Go Code Style

The `LogtailPlugin` uses [golangci-lint](https://golangci-lint.run/) for code style checks. The `SCOPE` definition is:

**Optional** with a default of the root directory.

```makefile
SCOPE=xxx make lint
```

Here, `xxx` represents the directory to be checked.
