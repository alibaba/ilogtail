#pragma once
#include <set>
#include <string>
#include <vector>

namespace logtail {

// Label is a key/value pair of strings.
struct Label {
    std::string name;
    std::string value;
};

// 需要Label的sort方法

// Labels is a sorted set of labels. Order has to be guaranteed upon instantiation

class Labels {
private:
    std::vector<Label> labels;

public:
    Labels();
    int64_t size() { return labels.size(); }
    void bytes();
    void matchLabels();
    void hash();
    void hashForLabels();
    void hashWithoutLabels();
    void bytesWithlabels();
    void bytesWithoutLabels();
    void copy();
    std::string get(std::string);
    void has();
};

class LabelsBuilder {
private:
    Labels base;
    std::vector<std::string> del;
    std::vector<Label> add;

public:
    LabelsBuilder();
    void reset();
    void labels();
    std::string get(std::string);
};


} // namespace logtail
