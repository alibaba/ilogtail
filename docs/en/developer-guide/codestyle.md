# Code Style

iLogtail's C++ code follows the style guide based on [Google's C++ Style Guide](https://google.github.io/styleguide/cppguide.html), with detailed formatting constraints defined in the [.clang-format](https://github.com/alibaba/ilogtail/blob/main/.clang-format) file.

Go code adheres to the [Effective Go](https://go.dev/doc/effective_go) style.

Markdown follows a style based on [markdownlint](https://github.com/DavidAnson/markdownlint/blob/main/doc/Rules.md), with exclusions for [MD033](https://github.com/DavidAnson/markdownlint/blob/main/doc/Rules.md#md033---inline-html) to maintain compatibility with [GitBook](https://docs.gitbook.com/editing-content/rich-content/with-command-palette#hints-and-callouts).

## Formatting C++ Code

Use the VSCode Clang-Format (by xaver) extension.

Alternatively, format the code using the command line:

```bash
find core/ -type f -iname '*.h' -o -iname '*.cpp' | xargs clang-format -i
```

## Formatting Go Code

Employ the VSCode Go (by Go Team at Google) extension.

Alternatively, format the code with the command line:

```bash
find ./ -type f -iname '*.go' -not -iname '*.pb.go' | grep -E -v '/external/' | xargs gofmt -w
```

## Formatting Markdown

Utilize the VSCode Markdownlint (by David Anson) extension.
