#ifndef WIN32
#include <gtest/gtest.h>
#include "common/CommonGlob.h"
#include "common/StringUtils.h"
#include "common/UnitTestEnv.h"

using namespace common;
using namespace std;

TEST(CommonGlobTest,adjustPattern)
{
    string inPattern;
    vector<string> outPatterns;
    inPattern="**";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(1));
    if(outPatterns.size()==size_t(1)){
        EXPECT_EQ(outPatterns[0],"/**/*");
    }
    inPattern="**a";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(1));
    if(outPatterns.size()==size_t(1)){
        EXPECT_EQ(outPatterns[0],"/**/*a");
    }
    inPattern="a**";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(2));
    if(outPatterns.size()==size_t(2)){
        EXPECT_EQ(outPatterns[0],"a*");
        EXPECT_EQ(outPatterns[1],"a*/**/*");
    }
    inPattern="**/a";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(1));
    if(outPatterns.size()==size_t(1)){
        EXPECT_EQ(outPatterns[0],"/**/a");
    }
    inPattern="a/**";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(1));
    if(outPatterns.size()==size_t(1)){
        EXPECT_EQ(outPatterns[0],"a/**/*");
    }
    inPattern="/**a";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(2));
    if(outPatterns.size()==size_t(2)){
        EXPECT_EQ(outPatterns[0],"/*a");
        EXPECT_EQ(outPatterns[1],"/**/*a");
    }
    inPattern="a**/";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(2));
    if(outPatterns.size()==size_t(2)){
        EXPECT_EQ(outPatterns[0],"a*/");
        EXPECT_EQ(outPatterns[1],"a*/**/");
    }
    inPattern="/**/";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(1));
    if(outPatterns.size()==size_t(1)){
        EXPECT_EQ(outPatterns[0],"/**/");
    }    
    inPattern="a**a";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(2));
    if(outPatterns.size()==size_t(2)){
        EXPECT_EQ(outPatterns[0],"a*a");
        EXPECT_EQ(outPatterns[1],"a*/**/*a");
    }
    inPattern="a*a";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(1));
    if(outPatterns.size()==size_t(2)){
        EXPECT_EQ(outPatterns[0],"a*a");
    }
    inPattern="/******/";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(1));
    if(outPatterns.size()==size_t(1)){
        EXPECT_EQ(outPatterns[0],"/**/");
    }
    inPattern="a******";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(2));
    if(outPatterns.size()==size_t(2)){
        EXPECT_EQ(outPatterns[0],"a*");
        EXPECT_EQ(outPatterns[1],"a*/**/*");
    }
    inPattern="*******";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(1));
    if(outPatterns.size()==size_t(1)){
        EXPECT_EQ(outPatterns[0],"/**/*");
    }
    inPattern="*******a";
    outPatterns.clear();
    globUtil::adjustPattern(inPattern,outPatterns);
    EXPECT_EQ(outPatterns.size(), size_t(1));
    if(outPatterns.size()==size_t(1)){
        EXPECT_EQ(outPatterns[0],"/**/*a");
    }
}

TEST(CommonGlobTest, myglob1) {
    vector<string> fileNames;
    string pattern = (TEST_CONF_PATH / "conf/glob/test.log").string();
    globUtil::myglob1(pattern, fileNames);
    EXPECT_EQ(fileNames.size(), size_t(1));
    if (fileNames.size() == size_t(1)) {
        EXPECT_EQ(fileNames[0], (TEST_CONF_PATH / "conf/glob/test.log").string());
    }

    fileNames.clear();
    pattern = (TEST_CONF_PATH / "conf/glob/aabb").string();
    globUtil::myglob1(pattern, fileNames);
    EXPECT_EQ(fileNames.size(), size_t(1));
    if (fileNames.size() == size_t(1)) {
        EXPECT_EQ(fileNames[0], (TEST_CONF_PATH / "conf/glob/aabb/").string());
    }

    fileNames.clear();
    pattern = "/conf/glob/aabb";
    globUtil::myglob1(pattern, fileNames);
    EXPECT_TRUE(fileNames.empty());

    fileNames.clear();
    pattern = (TEST_CONF_PATH / "conf/glob/*").string();
    globUtil::myglob1(pattern, fileNames);
    EXPECT_EQ(fileNames.size(), size_t(2));
    if (fileNames.size() == size_t(2)) {
        EXPECT_EQ(StringUtils::Contain(fileNames, (TEST_CONF_PATH / "conf/glob/test.log").string(), true), true);
        EXPECT_EQ(StringUtils::Contain(fileNames, (TEST_CONF_PATH / "conf/glob/aabb/").string(), true), true);
    }

    fileNames.clear();
    pattern = (TEST_CONF_PATH / "conf/glob/**").string();
    globUtil::myglob1(pattern, fileNames);
    EXPECT_EQ(fileNames.size(), size_t(2));
    if (fileNames.size() == size_t(2)) {
        EXPECT_EQ(StringUtils::Contain(fileNames, (TEST_CONF_PATH / "conf/glob/test.log").string(), true), true);
        EXPECT_EQ(StringUtils::Contain(fileNames, (TEST_CONF_PATH / "conf/glob/aabb/").string(), true), true);
    }
    fileNames.clear();
    pattern = (TEST_CONF_PATH / "conf/glob/**/*").string();
    globUtil::myglob1(pattern, fileNames);
    EXPECT_EQ(fileNames.size(), size_t(2));
    if (fileNames.size() == size_t(2)) {
        EXPECT_EQ(StringUtils::Contain(fileNames, (TEST_CONF_PATH / "conf/glob/aabb/test.log").string(), true), true);
        EXPECT_EQ(StringUtils::Contain(fileNames, (TEST_CONF_PATH / "conf/glob/aabb/aabb/").string(), true), true);
    }
    fileNames.clear();
    pattern = "~/argusagent/test/unit_test/conf/glob/aabb";
    globUtil::myglob1(pattern, fileNames);
    EXPECT_EQ(fileNames.size(), size_t(0));
}

TEST(CommonGlobTest, myglob2) {
    string pattern;
    vector<string> fileNames;

    fileNames.clear();
    pattern = (TEST_CONF_PATH / "conf/glob/**").string();
    globUtil::myglob2(pattern, fileNames);
    EXPECT_EQ(fileNames.size(), size_t(3));
    if (fileNames.size() == size_t(3)) {
        EXPECT_EQ(fileNames[0], (TEST_CONF_PATH / "conf/glob/test.log").string());
        EXPECT_EQ(fileNames[1], (TEST_CONF_PATH / "conf/glob/aabb/test.log").string());
        EXPECT_EQ(fileNames[2], (TEST_CONF_PATH / "conf/glob/aabb/aabb/test.log").string());
    }

    fileNames.clear();
    pattern = (TEST_CONF_PATH / "conf/glob/a**").string();
    globUtil::myglob2(pattern, fileNames);
    EXPECT_EQ(fileNames.size(), size_t(2));
    if (fileNames.size() == size_t(2)) {
        EXPECT_EQ(fileNames[0], (TEST_CONF_PATH / "conf/glob/aabb/test.log").string());
        EXPECT_EQ(fileNames[1], (TEST_CONF_PATH / "conf/glob/aabb/aabb/test.log").string());
    }

    fileNames.clear();
    pattern = (TEST_CONF_PATH / "conf/glob/*/**").string();
    globUtil::myglob2(pattern, fileNames);
    EXPECT_EQ(fileNames.size(), size_t(2));
    if (fileNames.size() == size_t(2)) {
        EXPECT_EQ(fileNames[0], (TEST_CONF_PATH / "conf/glob/aabb/test.log").string());
        EXPECT_EQ(fileNames[1], (TEST_CONF_PATH / "conf/glob/aabb/aabb/test.log").string());
    }

    fileNames.clear();
    pattern = (TEST_CONF_PATH / "conf/glob/*/**/").string();
    globUtil::myglob2(pattern, fileNames);
    EXPECT_EQ(fileNames.size(), size_t(2));
    if (fileNames.size() == size_t(2)) {
        EXPECT_EQ(fileNames[0], (TEST_CONF_PATH / "conf/glob/aabb/").string());
        EXPECT_EQ(fileNames[1], (TEST_CONF_PATH / "conf/glob/aabb/aabb/").string());
    }

    fileNames.clear();
    pattern = (TEST_CONF_PATH / "conf/glob/**/**").string();
    globUtil::myglob2(pattern, fileNames);
    EXPECT_EQ(fileNames.size(), size_t(3));
    if (fileNames.size() == size_t(3)) {
        EXPECT_EQ(fileNames[0], (TEST_CONF_PATH / "conf/glob/test.log").string());
        EXPECT_EQ(fileNames[1], (TEST_CONF_PATH / "conf/glob/aabb/test.log").string());
        EXPECT_EQ(fileNames[2], (TEST_CONF_PATH / "conf/glob/aabb/aabb/test.log").string());
    }
}
#endif // WIN32