# BasicAuth Authentication Extension

## Introduction

The `ext_basicauth` extension plugin implements the [extensions.ClientAuthenticator](https://github.com/alibaba/ilogtail/blob/main/pkg/pipeline/extensions/authenticator.go) interface, allowing it to be referenced within the `http_flusher` plugin to add a basic auth header to requests.

## Version

[Alpha](../stability-level.md)

## Configuration Parameters

| Parameter       | Type     | Required | Description  |
|----------|--------|------|-----|
| Username | String | Yes    | User name |
| Password | String | Yes    | Password  |

## Example

Collect all files with names matching the pattern `*.log` in the `/home/test-log/` directory and submit the collected results in a `custom_single` protocol and `json` format to `http://localhost:8086/write`. Use the Basic Auth authentication extension for the submission.

```yaml
enable: true

inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
flushers:
  - Type: flusher_http
    RemoteURL: "http://localhost:8086/write"
    Convert:
      Protocol: custom_single
      Encoding: json
    Authenticator:
      Type: ext_basicauth
extensions:
  - Type: ext_basicauth
    Username: user1
    Password: pwd1
```

## Using Named Extensions

When using multiple extensions of the same type within a single pipeline, you can assign a name to each instance, allowing you to reference a specific instance by its full name.

For example, by changing `Type: ext_basicauth` to `Type: ext_basicauth/user1` and adding the name `user1`, you can reference the `user1` instance in the `http_flusher` plugin like this:

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
flushers:
  - Type: flusher_http
    RemoteURL: "http://localhost:8086/write"
    Convert:
      Protocol: custom_single
      Encoding: json
    Authenticator:
      Type: ext_basicauth/user1
extensions:
  - Type: ext_basicauth/user1
    Username: user1
    Password: pwd1
  - Type: ext_basicauth/user2
    Username: user2
    Password: pwd2
```
