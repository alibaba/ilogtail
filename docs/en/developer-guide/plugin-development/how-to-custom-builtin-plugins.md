# How to Customize the Plugins Included in the Build Product

## Plugin Reference Mechanism

iLogtail uses the [plugin reference configuration file](https://github.com/alibaba/ilogtail/blob/main/plugins.yml) to define the plugins that should be included in the build output. By default, it contains all plugins from the iLogtail repository.

Additionally, iLogtail supports including external private plugins using a similar mechanism. For instructions on developing external plugins, refer to [How to Write External Plugins](how-to-write-external-plugins.md). iLogtail will automatically look for an `external_plugins.yml` file in the root directory of the repository for external plugin definitions.

When executing build commands like `make all`, the configuration file is parsed and generates Go import files in the [plugins/all](https://github.com/alibaba/ilogtail/tree/main/plugins/all) directory.

The format of the plugin reference configuration file is as follows:

```yaml
plugins:    # Plugins to be registered, grouped by platform
    common:
        - gomod: code.private.org/private/custom_plugins v1.0.0  # Optional, plugin module, only for external plugins
          import: code.private.org/private/custom_plugins        # Optional, package path to import in code
          path: ../custom_plugins                                # Optional, local path override for debugging
    windows:
    ...
    linux:
    ...
    project:
        replaces:       # Optional, array, resolves dependency conflicts between multiple plugin modules
        go_envs:        # Optional, map, sets environment variables like GOPRIVATE for private plugins
            GOPRIVATE: "*.code.org"
        git_configs:    # Optional, map, adjusts git URLs for private plugin repos that require authentication
            url.https://user:token@github.com/user/.insteadof: https://github.com/user/
```

## Customizing Plugins in the Build Product

### Method 1: Modify the Default `plugins.yml` or `external_plugins.yml` File

As mentioned earlier, you can directly modify the default [plugin reference configuration file](https://github.com/alibaba/ilogtail/blob/main/plugins.yml) to select the plugins you want to include in the build.

### Method 2: Custom Configuration File

Alternatively, you can create a custom `plugin reference configuration file` to guide the build. This file can be a local file or a remote file URL, and its content should follow the same format as the default `plugins.yml` file.

For example, if your custom plugin reference configuration file is named `/tmp/custom_plugins.yml`, you can set the `PLUGINS_CONFIG_FILE` environment variable to the file's path to guide the build, like this:

```shell
PLUGINS_CONFIG_FILE=/tmp/custom_plugins.yml make all
```

Note: `PLUGINS_CONFIG_FILE` supports multiple configuration file paths, separated by commas.
