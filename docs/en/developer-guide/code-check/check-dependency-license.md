# Checking Dependency Licenses

iLogtail is open-sourced under the Apache 2.0 license, and developers must ensure that all dependencies have compatible licenses. The license information for these dependencies is located in the `licenses` folder at the root directory.

## Checking Dependency Licenses

Developers can use the following command to scan for dependency licenses:

```makefile
make check-dependency-licenses
```

- If you see "DEPENDENCIES CHECKING IS PASSED," it means the dependency check has passed.

- If you see "FOLLOWING DEPENDENCIES SHOULD BE ADDED to LICENSE_OF_ILOGTAIL_DEPENDENCIES.md," it indicates that additional dependencies need to be added (ensuring compatibility with the Apache 2.0 license). You can check the `find_licenses/LICENSE-{license type}` files for the required dependencies.

- If you see "FOLLOWING DEPENDENCIES IN LICENSE_OF_ILOGTAIL_DEPENDENCIES.md SHOULD BE REMOVED," it means there are多余的 dependencies. Follow the prompt to remove them.

## Forked Code Repositories Management

Due to unsupported features or the need to simplify dependencies, iLogtail may use some forked code repositories. All forks are managed within the [iLogtail organization](https://github.com/iLogtail). For licensing reasons, private forks are not allowed.

## go.mod Management

1. Forked Repositories: For forked repositories, it's essential to respect the original author and avoid modifying the `go.mod` file's module address, as shown in the [example](https://github.com/iLogtail/go-mysql/blob/master/go.mod).

2. iLogtail Repository: For forked code, iLogtail requires using the `replace` directive to introduce the code. This ensures that the package import address in the code files remains the original repository address.

```go
require (
    github.com/VictoriaMetrics/metrics v1.23.0
)

replace (
    github.com/VictoriaMetrics/metrics => github.com/iLogtail/metrics v1.23.0-ilogtail
)
```

## License Declaration

Please execute the `make check-dependency-licenses` command, and the script will automatically generate a markdown description in the `find_licenses` folder. Place this description at the end of the [LICENSE_OF_ILOGTAIL_DEPENDENCIES.md](../../../../licenses/LICENSE_OF_ILOGTAIL_DEPENDENCIES.md) file, like this:

```go
# iLogtail uses or modifies source code from these projects

- [github.com/iLogtail/VictoriaMetrics fork from github.com/VictoriaMetrics/VictoriaMetrics](http://github.com/iLogtail/VictoriaMetrics) based on Apache-2.0

- [github.com/iLogtail/metrics fork from github.com/VictoriaMetrics/metrics](http://github.com/iLogtail/metrics) based on MIT
```

## Recommendations

If the forked features complement the original codebase and are not for specific scenarios like simplifying dependencies, it's recommended to submit a Pull Request to the original repository. If the original repository accepts the PR, update the iLogtail repository's dependency address to the original repository's and remove the forked repository.
