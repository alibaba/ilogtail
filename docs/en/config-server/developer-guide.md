# Development Guide

## Updating Control Protocol

1. [Propose changes in the discussion forum](https://github.com/alibaba/ilogtail/discussions/404)

2. Once approved, modify the [control protocol file](https://github.com/alibaba/ilogtail/tree/main/config_server/protocol)

3. Generate language-specific versions of the protocol and adapt the code accordingly:

   - C++ part for ilogtail: [agent.proto](https://github.com/alibaba/ilogtail/tree/main/core/config_server_pb)
   - Golang part for ConfigServer: [agent.proto & user.proto](https://github.com/alibaba/ilogtail/tree/main/config_server/service/proto)

## Development

- Adding Interfaces

   Interfaces should be added to the `router` folder in the `router.go` file, and corresponding implementation code should be added in the respective `manager` and `interface` folders.

- Expanding Storage Support

   To extend storage options, such as implementing MySQL storage, create a new implementation of the existing database interface `interface_database` in the `store` folder, and add the desired storage type in the `newStore` function of `store.go`.

## Compiling

Navigate to the `service` folder of Config Server:

```bash
go build -o ConfigServer
```

## Testing

Test files are stored in the `test` folder.

To view test coverage:

```bash
go test -cover github.com/alibaba/ilogtail/config_server/service/test -coverpkg github.com/alibaba/ilogtail/config_server/service/... -coverprofile coverage.out -v

go tool cover -func=coverage.out -o coverage.txt

# Visualization results: go tool cover -html=coverage.out -o coverage.html

cat coverage.txt
```
