# Processing Plugins

Processing plugins are used to parse, filter, and process collected content, dividing them into native and extended plugins:

* **Native Plugins**: Offer better performance and are suitable for most scenarios; recommended for优先 use.

* **Extended Plugins**: Provide broader functionality. If your business logs are too complex for native plugins, you can consider using extended plugins for log parsing, but performance may be compromised.

## Limitations

* Native plugins are currently limited to processing [text logs](../input/input-file.md) and [container standard output (native plugins)](../input/input-container-stdlog.md).

* Native plugins can be used in conjunction with extended plugins, but native plugins must precede them. The supported modes are:

  * **Native Mode**: Only use native plugins.
  
  * **Extended Mode**: Only use extended plugins.
  
  * **Sequential Mode**: Use native plugins followed by extended plugins.

---

Remember to maintain the Markdown structure and syntax in the translation, such as headers, bullet points, and code blocks.
