# Checking File Licenses

**LogtailPlugin** adheres to the standard Apache 2.0 license, requiring all files to have a license statement. Therefore, contributors should verify that contributed files carry this license.

## Checking Licenses

To scan the entire project's files for licenses, run the following command. Any files without a license will be identified and saved in the `license_coverage.txt` file.

```makefile
   make check-license
```

When targeting a specific scope, append `SCOPE` to the command.

```makefile
SCOPE=xxx make check-license
```

Replace `xxx` with the targeted directory name.

## Adding Licenses

Once files with a license are created, you can add the license to them using the following command. Note that this will fix any missing license statements in the files.

```makefile
     make license
```

Similarly, to limit the scope of the changes, include `SCOPE` in the command.

```makefile
SCOPE=xxx make license
```

Again, replace `xxx` with the designated directory name.
