//
// Created by 韩呈杰 on 2023/6/7.
//
#include <gtest/gtest.h>
#include "common/JsonValueEx.h"
#include <boost/format.hpp>

#include "common/Logger.h"
#include "common/Config.h"

using namespace common;

class CommJsonValueExTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
    }

    void TearDown() override {
    }
};

TEST_F(CommJsonValueExTest, ParseObject) {
    EXPECT_TRUE(json::parse("null").isNull());

    // allow_trailing_commas
    const char *szJson = R"({"age":"23","male":true,})";
    EXPECT_FALSE(json::parseObject(szJson).isNull());

    auto jv = json::parse(szJson);
    EXPECT_FALSE(jv.isNull());

    EXPECT_FALSE(jv.asObject().empty());
    EXPECT_TRUE(jv.asObject().contains("age"));
    EXPECT_FALSE(jv.asObject()["age"].isNull());
    EXPECT_FALSE(jv.asObject().contains("<age>"));
    EXPECT_TRUE(jv.asObject()["<age>"].isNull());
    int height = 10;
    jv.asObject().get("height", height);
    EXPECT_EQ(height, 10);

    EXPECT_EQ(jv.getObjectString("age"), "23");
    EXPECT_EQ(jv.getObjectNumber<double>("age"), 23.0);
    EXPECT_FALSE(jv.getObjectBool("age", false));
    EXPECT_TRUE(jv.getObjectBool("male"));

    std::cout << jv.toString() << std::endl;
    const char *expectJson = R"({"age":"23","male":true})";
    EXPECT_EQ(jv.toString(), expectJson);

    {
        std::string error;
        json::Object obj = json::parseObject("[]", error);
        EXPECT_TRUE(obj.isNull());
        EXPECT_EQ("not a json object", error);
    }
}

TEST_F(CommJsonValueExTest, ObjectGetString) {
    boost::json::object _obj{
            {"age", uint64_t(23)},
            {"height", double(1.69)},
    };
    json::Object obj{&_obj};
    EXPECT_EQ(obj.getString("age"), "23");
    EXPECT_EQ(obj.getString("height"), "1.6899999999999999");
}

TEST_F(CommJsonValueExTest, ObjectGetString2) {
    boost::json::object _obj{
            {"person", {
                    {"name", "hcj"},
            }},
            {"class", "A"},
            {"level", {
                    {"value", "A+"},
            }}
    };
    json::Object root{&_obj};

    auto r0 = root.getString({});
    EXPECT_TRUE(r0.result.empty());
    EXPECT_FALSE(r0.error.empty());

    auto r1 = root.getString({"person", "name"});
    EXPECT_EQ(r1.result, "hcj");
    EXPECT_TRUE(r1.error.empty());

    auto r2 = root.getString({"class", "level"});
    EXPECT_TRUE(r2.result.empty());
    EXPECT_EQ(r2.error, "node not object: class");

    auto r3 = root.getString({"level", "class"});
    EXPECT_TRUE(r3.result.empty());
    EXPECT_EQ(r3.error, "path not exist: level.class");

    auto r4 = root.getString({"animal", "height"});
    EXPECT_TRUE(r4.result.empty());
    EXPECT_EQ(r4.error, "path not exist: animal");
}

TEST_F(CommJsonValueExTest, StringExchangeWithNumber) {
    const char *expectJsons[] = {
            R"({"age":"23"})",
            R"({"age":23})",
    };
    for (auto *expectJson: expectJsons) {
        auto obj = json::parseObject(expectJson);
        EXPECT_FALSE(obj.isNull());

        EXPECT_EQ(obj.getString("age"), "23");
        EXPECT_EQ(obj.getNumber<int>("age"), 23);
        LogInfo("{}", obj.toStyledString());
    }
}

TEST_F(CommJsonValueExTest, ParseArray) {
    {
        std::string error;
        json::Array arr = json::parseArray("{}", error);
        EXPECT_TRUE(arr.isNull());
        EXPECT_EQ("not a json array", error);
    }
    const char *expectJson = R"([{"age":23},[1,2],"abc",456,true,null])";
    auto arr = json::parseArray(expectJson);
    EXPECT_FALSE(arr.isNull());
    EXPECT_EQ(arr.size(), (size_t) 6);

    EXPECT_FALSE(arr.at<json::Object>(0).isNull());
    json::Object arr0 = arr[0].asObject();
    EXPECT_FALSE(arr0.isNull());
    EXPECT_EQ(arr0.getNumber<int>("age"), 23);
    EXPECT_TRUE(HasPrefix(arr0.toString(), "{"));
    EXPECT_TRUE(HasPrefix(arr0.toStyledString(), "{"));

    json::Array arr1 = arr.at<json::Array>(1);
    EXPECT_FALSE(arr1.isNull());
    EXPECT_EQ(arr1.size(), size_t(2));
    EXPECT_EQ(arr1[0].asNumber<int>(), 1);
    EXPECT_EQ(arr1[1].asNumber<int>(), 2);
    EXPECT_TRUE(HasPrefix(arr1.toString(), "["));
    EXPECT_FALSE(HasPrefix(arr1.toString(), "[["));
    EXPECT_TRUE(HasPrefix(arr1.toStyledString(), "["));
    EXPECT_FALSE(HasPrefix(arr1.toString(), "[["));

    EXPECT_EQ(arr.at<std::string>(2), "abc");
    EXPECT_EQ(arr.at<int>(3), 456);
    EXPECT_TRUE(arr.at<bool>(4));
    EXPECT_TRUE(arr.at<json::Value>(5).isNull());
}

TEST_F(CommJsonValueExTest, Precision) {
    json::Pretty pretty;

    auto runTest = [&](double v, const std::string &expect) {
        std::string buf = fmt::sprintf("%.*g\n", pretty.actualPrecision(), v);
        EXPECT_EQ(expect, buf);
        std::stringstream os;
        pretty.print(os, boost::json::value_from(v));
        EXPECT_EQ(os.str(), buf);
    };

    pretty.precision = 5;
    runTest(100.0 / 3, "33.333\n");
    runTest(0.25000000, "0.25\n");
    runTest(0.2563456, "0.25635\n");

    pretty.precision = 1;
    runTest(0.2563456, "0.3\n");

    pretty.precision = 17;
    runTest(1234857476305.256345694873740545068, "1234857476305.2563\n");

    pretty.precision = 24;
    runTest(0.256345694873740545068, "0.25634569487374054\n");

    pretty.precisionType = json::EnumPrecisionType::DecimalPlaces;

    pretty.precision = 5;
    runTest(0.256345694873740545068, "0.25635\n");

    pretty.precision = 1;
    runTest(0.256345694873740545068, "0.3\n");

    pretty.precision = 10;
    runTest(0.23300000, "0.233\n");
}

namespace json {
    boost::json::object sortKey(const boost::json::object &obj);
}
TEST_F(CommJsonValueExTest, SortKey) {
    boost::json::object obj{
            {"z", 35},
            {"a", "hello"}
    };
    const char *expectedKeys[] = {"z", "a"};
    ASSERT_EQ(obj.size(), sizeof(expectedKeys)/sizeof(expectedKeys[0]));

    {
        int index = 0;
        for (const auto &it: obj) {
            EXPECT_EQ(std::string{expectedKeys[index++]}, std::string{it.key().data()});
        }
    }

    boost::json::object _obj = json::sortKey(obj);
    const char *expectedKeys2[] = {"a", "z"};

    {
        int index = 0;
        for (const auto &it: _obj) {
            EXPECT_EQ(std::string{expectedKeys2[index++]}, std::string{it.key().data()});
            EXPECT_EQ(it.value(), obj.at(it.key()));
        }
    }
}

TEST_F(CommJsonValueExTest, fixNumericLocale) {
    std::string s = "25,22";
    json::fixNumericLocale(s.begin(), s.end());
    EXPECT_EQ(s, "25.22");
}

TEST_F(CommJsonValueExTest, fixZerosInTheEnd) {
    {
        std::string s = "25.22";
        s.erase(json::fixZerosInTheEnd(s.begin(), s.end()), s.end());
        EXPECT_EQ(s, "25.22");
    }
    {
        std::string s = "25.220";
        s.erase(json::fixZerosInTheEnd(s.begin(), s.end()), s.end());
        EXPECT_EQ(s, "25.22");
    }
    {
        std::string s = "25.000";
        s.erase(json::fixZerosInTheEnd(s.begin(), s.end()), s.end());
        EXPECT_EQ(s, "25.0");
    }
    {
        std::string s = "0.000";
        s.erase(json::fixZerosInTheEnd(s.begin(), s.end()), s.end());
        EXPECT_EQ(s, "0.0");
    }
}

TEST_F(CommJsonValueExTest, Pretty_DoubleToString) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.3f", 1.25);
    EXPECT_STREQ("1.250", buf);

    json::Pretty pretty;
    pretty.precisionType = json::EnumPrecisionType::DecimalPlaces;
    pretty.precision = 3;
    EXPECT_EQ(pretty.doubleToString(1.25), "1.25");
    EXPECT_EQ(pretty.doubleToString(1), "1.0");
}

TEST_F(CommJsonValueExTest, Pretty_SpecialDoubleToString) {
    json::Pretty pretty;
    pretty.useSpecialFloat = true;
    EXPECT_EQ(pretty.doubleToString(std::numeric_limits<double>::quiet_NaN()), "nan");
    EXPECT_EQ(pretty.doubleToString(std::numeric_limits<double>::infinity()), "inf");
    EXPECT_EQ(pretty.doubleToString(-std::numeric_limits<double>::infinity()), "-inf");

    pretty.useSpecialFloat = false;
    EXPECT_EQ(pretty.doubleToString(std::numeric_limits<double>::quiet_NaN()), "null");
    EXPECT_EQ(pretty.doubleToString(std::numeric_limits<double>::infinity()), "1e+9999");
    EXPECT_EQ(pretty.doubleToString(-std::numeric_limits<double>::infinity()), "-1e+9999");
}

TEST_F(CommJsonValueExTest, PretyPrintUint64) {
    json::Pretty pretty;

    std::stringstream os;
    pretty.print(os, boost::json::value_from(uint64_t(1)));
    EXPECT_EQ(os.str(), "1\n");
}

TEST_F(CommJsonValueExTest, mergeArray) {
    EXPECT_EQ(json::mergeArray("", {}), "");
    EXPECT_EQ(json::mergeArray("{", {}), "{");
    EXPECT_EQ(json::mergeArray("{}", {}), "{}");
    EXPECT_EQ(json::mergeArray("[]", {}), "[]");

    std::string s = json::mergeArray("[]", {R"({"name":"abc"})"});
    LogInfo("{}", s);
    EXPECT_EQ(s, "[\n\t{\n\t\t\"name\" : \"abc\"\n\t}\n]");

    s = json::mergeArray(R"(["abc"])", {R"({"name":"cde"})", R"({})"});
    LogInfo("{}", s);
    // "[\n\t\"abc\",\n\t{\n\t\t\"name\" : \"cde\"\n\t},\n\t{\n\n\t}\n]\n"
    EXPECT_EQ(s, "[\n\t\"abc\",\n\t{\n\t\t\"name\" : \"cde\"\n\t},\n\t{}\n]");
}

TEST_F(CommJsonValueExTest, ValueFrom) {
    {
        auto value = boost::json::value_from(std::string{"hello"});
        EXPECT_EQ(value.kind(), boost::json::kind::string);
    }
    {
        std::map<std::string, std::string> data {
                {"a", "b"},
        };
        auto value = boost::json::value_from(data);
        EXPECT_EQ(value.kind(), boost::json::kind::object);
        EXPECT_EQ(boost::json::serialize(value), R"({"a":"b"})");
    }
}
