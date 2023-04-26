@echo off

REM Procedures:
REM 1. Set environments.
REM 2. Build iLogtail.
REM 3. Build iLogtail plugin.
REM 4. Make package.


set ILOGTAIL_VERSION=%1
if "%ILOGTAIL_VERSION%" == "" (
    echo Must specify iLogtail version.
    goto quit
)
set CurrentPath=%~dp0
set P1Path=
set P2Path=
:begin
for /f "tokens=1,* delims=\" %%i in ("%CurrentPath%") do (set content=%%i&&set CurrentPath=%%j)
if "%P1Path%%content%\" == "%~dp0" goto end
set P2Path=%P1Path%
set P1Path=%P1Path%%content%\
goto begin
:end

set ILOGTAIL_PLUGIN_SRC_PATH=%P2Path%\ilogtail
REM boost_1_68_0依赖包与ilogtail放在同一级目录
set BOOST_ROOT=%P2Path%\boost_1_68_0
REM 设置ilogtail-deps.windows-386编译依赖包路径
set ILOGTAIL_DEPS_PATH=E:/pipeline/win32/ilogtail-deps.windows-386
set CMAKE_BIN=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake
set DEVENV_BIN=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.com
set OUTPUT_DIR=%ILOGTAIL_PLUGIN_SRC_PATH%\output
set ILOGTAIL_CORE_BUILD_PATH=%ILOGTAIL_PLUGIN_SRC_PATH%\core\build

go env -w GOPROXY="https://goproxy.cn,direct"
set GOARCH=386
set GOFLAGS=-buildvcs=false
set CGO_ENABLED=1

set PATH=%PATH%;%DEVENV_BIN%

REM Clean up
IF exist %OUTPUT_DIR% ( rd /s /q %OUTPUT_DIR% )
mkdir %OUTPUT_DIR%

REM Build C++ core(ilogtail.exe、PluginAdapter.dll)
echo begin to compile core
cd %ILOGTAIL_PLUGIN_SRC_PATH%\core
IF exist build ( rd /s /q build )
mkdir build
cd build
"%CMAKE_BIN%" -DCMAKE_BUILD_TYPE=Release -DLOGTAIL_VERSION=Bytedance -DDEPS_ROOT=%ILOGTAIL_DEPS_PATH% ..
if not %ERRORLEVEL% == 0 (
    echo Run cmake failed.
    goto quit
)
"%DEVENV_BIN%" logtail.sln /Build "Release|Win32" 1>build.stdout 2>build.stderr
if not %ERRORLEVEL% == 0 (
    echo Build iLogtail source failed.
    goto quit
)
echo Build core success

REM import plugins
cd %ILOGTAIL_PLUGIN_SRC_PATH%
echo ===============GENERATING PLUGINS IMPORT==================
del /f/s/q %ILOGTAIL_PLUGIN_SRC_PATH%\plugins\all\all.go
del /f/s/q %ILOGTAIL_PLUGIN_SRC_PATH%\plugins\all\all_debug.go
del /f/s/q %ILOGTAIL_PLUGIN_SRC_PATH%\plugins\all\all_windows.go
del /f/s/q %ILOGTAIL_PLUGIN_SRC_PATH%\plugins\all\all_linux.go
go run -mod=mod "%ILOGTAIL_PLUGIN_SRC_PATH%\tools\builder" -root-dir="%ILOGTAIL_PLUGIN_SRC_PATH%" -config="plugins.yml,external_plugins.yml" -modfile="go.mod"
echo generating plugins finished successfully

REM Build plugins(PluginBase.dll、PluginBase.h)
echo Begin to build plugins...
IF exist %OUTPUT_DIR% ( rd /s /q %OUTPUT_DIR% )
mkdir %OUTPUT_DIR%
xcopy /Y %ILOGTAIL_CORE_BUILD_PATH%\plugin\Release\PluginAdapter.dll %ILOGTAIL_PLUGIN_SRC_PATH%\pkg\logtail
set LDFLAGS="-X "github.com/alibaba/ilogtail/pluginmanager.BaseVersion=%LOGTAIL_VERSION%""
go build -mod=mod -buildmode=c-shared -ldflags=%LDFLAGS% -o output\PluginBase.dll %LOGTAIL_PLUGIN_SRC_PATH%/plugin_main
if not %ERRORLEVEL% == 0 (
    echo Build iLogtail plugin source failed.
    goto quit
)
echo Build plugins success

REM Copy artifacts
xcopy /Y %ILOGTAIL_CORE_BUILD_PATH%\Release\ilogtail.exe %OUTPUT_DIR%
xcopy /Y %ILOGTAIL_CORE_BUILD_PATH%\plugin\Release\PluginAdapter.dll %OUTPUT_DIR%
xcopy /Y E:\pipeline\win32\oneAgent.exe %OUTPUT_DIR%
cd %OUTPUT_DIR%
dir

:quit
