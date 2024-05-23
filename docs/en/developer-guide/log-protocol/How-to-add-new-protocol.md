# Adding a New Log Protocol

If iLogtail currently doesn't support the log protocol you need, you can add it by implementing the corresponding protocol converter. Here's a step-by-step guide:

1. **Create Protocol Directory Structure**:
   - If your protocol supports Protobuf or any other format that can be defined with a mode and generates a corresponding memory structure, create a new folder in `./pkg/protocol` with the protocol name. Inside this folder, create a subfolder with the encoding name and place the mode definition file there. Also, add the generated Go code file from the code generator tool in the parent directory. The directory structure should look like this:

   ```plaintext
   ./pkg/protocol/
   ├── <protocol_name>
   │   ├── <encoding>
   │       └── mode definition file
   │   └── generated Go files
   └── converter
       ├── converter.go
       ├── <protocol>_log.go
       └── test files
   ```

2. **Implement Conversion Functions**:
   - In the `./pkg/protocol/converter` directory, create a file named `<protocol>_log.go` and implement the following functions:

   ```Go
   func (c *Converter) ConvertToXXXProtocolLogs(logGroup *sls.LogGroup, targetFields []string) (logs interface{}, values [][]string, err error)
   
   func (c *Converter) ConvertToXXXProtocolStream(logGroup *sls.LogGroup, targetFields []string) (stream interface{}, values [][]string, err error)
   ```

   The function names should have "XXX" as the protocol name (shortened if needed), with `logGroup` as the sls LogGroup, `targetFields` as the field names to extract, and the return values representing the data structure array for the protocol, a byte stream for the log group, and error. The functions must:
   - Convert input logs appropriately
   - Rename LogTag fields' keys in `c.TagKeyRenameMap`
   - Locate values for `targetFields`
   - Optionally rename protocol field keys with `c.ProtocolKeyRenameMap`

   For the second and third points, iLogtail provides helper functions:

   ```Go
   func convertLogToMap(log *sls.Log, logTags []*sls.LogTag, src, topic string, tagKeyRenameMap map[string]string) (contents map[string]string, tags map[string]string)
   
   func findTargetValues(targetFields []string, contents, tags, tagKeyRenameMap map[string]string) (values map[string]string, err error)
   ```

   - `convertLogToMap` converts logs and metadata to maps (`contents` and `tags`) and renames LogTag keys using `tagKeyRenameMap`.
   - `findTargetValues` finds values for `targetFields` in the maps, considering renamed tag keys.

3. **Update `converter.go`**:
   - Add a `protocolXXX` constant with the protocol name, and an `encodingXXX` constant for the encoding (if applicable).
   - Add the encoding to the `supportedEncodingMap`.
   - In the `c.DoWithSelectedFields` method's `switch` statement, add a new `case` for the protocol name, calling the respective function: `return c.ConvertToXXXProtocolLogs(logGroup, targetFields)`.
   - Similarly, add a new `case` in the `c.ToByteStreamWithSelectedFields` method.

4. **Document the Protocol**:
   - Update the `./doc/en/developer-guide/log-protocol/converter.md` appendix and `README.md` with the new protocol details.
   - Create a new file named `<protocol>.md` in the `./doc/en/developer-guide/log-protocol/protocol-spec` folder to describe the protocol format in detail.

Now you've successfully added support for your new log protocol to iLogtail.
