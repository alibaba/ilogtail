# iLogtail Windows版本编译流程
## 编译环境配置
参考：https://github.com/alibaba/ilogtail/issues/327
若要编译64位版本，相关软件下载64位的即可
## 准备编译依赖
- 将boost安装到与ilogtail同级目录
- 下载ilogtail-deps编译依赖，放到与ilogtail同级目录
  - 32位依赖：https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/prebuiltdependencies/ilogtail-deps.windows-386.zip
  - 64位依赖：
## 修改编译脚本
将ilogtail/scripts/windows32_build.bat(windows64_build.bat)脚本中的的CMAKE_BIN、DEVENV_BIN两个环境变量值替换成编译机器上实际的路径
```shell
set CMAKE_BIN=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake
set DEVENV_BIN=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.com
```
## 执行编译脚本
cd 到ilogtail/scripts目录下，执行windows32_build.bat或者windows64_build.bat脚本，等待约6分钟，完成编译。
编译产物列表：
- ilogtail.exe
- PluginAdapter.dll
- PluginBase.dll
- PluginBase.h
