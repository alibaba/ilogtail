| Class | Function/Variable | Type | Parameter | Description |
| - | - | - | - | - | 
| baseSetting | - | - | - | 保存controller的启动设置信息的类 |
|  | RunMode | int | - | 运行模式，0为集群模式, 1为单机模式 |
|  | StoreMode | int | - | 持久化信息的存储模式，0为本地文件存储，1为LevelDB存储 |
| Config | - | - | - | 单个的ilogtail配置文件的信息 |
|  | Name | String | - | ilogtail配置文件的名称（主Key） |
|  | Path | String | - | ilogtail配置文件在本地的路径 |
|  | Description | String | - | ilogtail配置文件的备注 |
| ConfigGroup | - | - | - | Config组，目前默认只有一个default组 |
|  | Name | String | - | Config组的名称（主Key） |
|  | Description | String | - | Config组的描述 |
|  | Configs | []String | - | 该Config组里所有的Config的Name，升序排序 |
|  | func GetConfigNumber | int | - | 获取Config组中的Config数量 |
|  | func AddConfig | error | *Config | 向Config组中添加Config信息，加入失败返回错误信息，保证插入后数据有序 |
|  | func DelConfig | error | String | 删除Config组中指定的Config信息，删除失败会返回错误信息，保证删除后数据有序 |
|  | func GetConfigList | - | []Config | 以数组的方式返回ConfigGroup中所有Config的信息 |
|  | func ChangeConfigName| error | String, String | 修改ConfigGroup中指定config的名称，修改失败则返回错误信息。改动影响ConfigList（有待修改） |
|  | func ChangeConfigPath | error | String, String | 修改ConfigGroup中指定config的路径，修改失败则返回错误信息。改动影响ConfigList（有待修改） |
|  | func ChangeConfigDescription | error | String, String | 修改ConfigGroup中指定config的描述，修改失败则返回错误信息。改动影响ConfigList（有待修改） |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |

| Function | Type | Parameter | Description |
| - | - | - | - | 
| GetMyBaseSetting | *baseSetting | - | 获取运行配置 |
| UpdateMyBasesetting | - | - | 更新运行配置 |
| InitManager | - | - | 初始化（运行配置，持久化数据） |
| NewConfig | *Config | String, String, String | 新建一个ilogtail配置文件的信息 |
| AddConfig | error | *Config | 将一个Config文件加入ConfigList中，加入失败会返回错误信息 |
| DelConfig | error | String | 删除ConfigList中指定名称的Config，删除失败会返回错误信息 |
| GetConfigs | - | []Config | 以数组的方式返回ConfigList中所有Config的信息 |
| NewConfigGroup | *ConfigGroup | - | 新建一个ConfigGroup |
| AddConfigGroup | error | *ConfigGroup | 将一个ConfigGroup加入ConfigGroupList中，加入失败会返回错误信息 |
| DelConfigGroup | error | String | 删除ConfigGroupList中指定名称的ConfigGroup，删除失败会返回错误信息 |
| GetConfigGroups | - | []ConfigGroup | 以数组的方式返回ConfigGroupList中所有ConfigGroup的信息 |
|  |  |  |  |
|  |  |  |  |
|  |  |  |  |

| Variable | Type | Description |
| - | - | - |
| BaseSettingFile | String | controller的启动设置文件地址，为`./base_setting.json` |
| myBaseSetting | baseSetting | 读取保存运行配置信息的单例 |
| MaxConfigNum | int | 最大可以保存的Config个数，为1000 |
| ConfigList | map[string]*Config | 保存运行状态中所有Config的单例，Key为Config的Name |
| MaxConfigGroupNum | int | 最大可以保存的ConfigGroup个数，为10 |
| ConfigGroupList | map[string]*ConfigGroup | 保存运行状态中所有ConfigGroup的单例，Key为ConfigGroup的Name |
|  |  |  |
|  |  |  |