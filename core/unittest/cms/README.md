**WITHOUT_SPL设为OFF**

# TODO LIST
1. CoreDump模块未激活, 用例(Cms_CloudClientTest.SendCoreDump)会失败<br/>`/workspace/ilogtail/build/unittest/cms/cms_unittest --gtest_filter=Cms_CloudClientTest.SendCoreDump`
> @笃敏 ilogtail有coredump能力，argus无需激活该项。
2. 

# 编译主程序
点击最左侧的CMake图标。配置中去掉如下选项:
1) BUILD_LOGTAIL_UT
2) BUILD_TESTING

同时Project Status中Build改为ilogtail
