# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), but make some changes.

## Release tags

- `public` for changes that will be published in release notes.
- `inner` for other changes.

## Platform tags

This project will generate binaries for at least two platforms (Linux/Windows.noarch), so please add platform tag for
your changes, such as:

- `both` for all platforms.
- `win` for Windows.
- `linux` for Linux on amd64 or arm64 arch.
- `linux.arm` for Linux on arm64 arch.
- ....

## Type of changes

- `added` for new features.
- `updated` for changes in existing functionality.
- `deprecated` for soon-to-be removed features.
- `removed` for now removed features.
- `fixed` for any bug fixes.
- `security` in case of vulnerabilities.
- `doc` for doc changes.
- `test` for tests.

## Example

- [public] [both] [updated] add a new feature

## [Unreleased]

- [public] [both] [added] refactoried C++ process pipeline
- [public] [both] [added] support use accelerate processors with go processors
- [public] [both] [added] add new logtail metric module
- [public] [both] [added] ddd JSON flatten protocol, data can be flattened and then brushed into storage such as Kafka and ES.
- [public] [both] [added] use env `LOGTAIL_LOG_LEVEL` to control ilogtail log level
- [public] [both] [updated] support continue/end regex patterns to split multiline log
- [public] [both] [updated] support reader flush timeout
- [public] [both] [updated] Flusher Kafka V2: support send the message with headers to kafka
- [public] [both] [updated] update gcc version to 9.3.1
- [public] [both] [updated] add make flag WITHOUTGDB
- [public] [both] [updated] cache incomplete line in memory to avoid repeated read system call
- [public] [both] [fixed] Add APSARA\_LOG\_TRACE to solve the problem of not being able to find LOG\_TRACE.
- [public] [both] [fixed] fix multiline is splitted if not flushed to disk together
- [public] [both] [fixed] fix line is truncated if \0 is in the middle of line
- [public] [both] [fixed] container cannot exit for file reopened by checkpoint
- [public] [both] [fixed] fix filename being mismatched to the deleted file if the deleted file size is 0 and their inode is same
- [public] [both] [fixed] fix config server panic caused by concurrent read and write shared object
- [public] [both] [fixed] timezone adjust not working with apsara\_log
- [public] [both] [added] support plugin ProcessorParseTimestampNative
- [public] [both] [added] support plugin ProcessorOtelMetric
- [public] [both] [updated] skywalking plugin support to capture `db.connection_string` tag 
- [public] [both] [added] support plugin ProcessorParseApsaraNative
- [public] [both] [added] support plugin ProcessorParseDelimiterNative
- [public] [both] [added] support plugin ProcessorFilterNative
- [public] [both] [added] support plugin ProcessorDesensitizeNative
- [public] [both] [added] added UsingOldContentTag. When UsingOldContentTag is set to false, the tag is now put into the context during cgo.
