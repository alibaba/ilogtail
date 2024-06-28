#include "common/RegexUtil.h"
#include "common/Logger.h"

#include <regex>
#include <algorithm> // std::min

#ifdef min
#   undef max
#   undef min
#endif

using namespace common;

std::string show_regex_error(const std::regex_error &e) {
    std::string err_message = e.what();

#   define CASE(type, msg) \
    case std::regex_constants::type: \
        err_message += std::string(" (") + #type + "): " + (msg); \
        break

    switch (e.code()) {
        CASE(error_collate, "The expression contains an invalid collating element name");
        CASE(error_ctype, "The expression contains an invalid character class name");
        CASE(error_escape, "The expression contains an invalid escaped character or a trailing escape");
        CASE(error_backref, "The expression contains an invalid back reference");
        CASE(error_brack, "The expression contains mismatched square brackets ('[' and ']')");
        CASE(error_paren, "The expression contains mismatched parentheses ('(' and ')')");
        CASE(error_brace, "The expression contains mismatched curly braces ('{' and '}')");
        CASE(error_badbrace, "The expression contains an invalid range in a {} expression");
        CASE(error_range, "The expression contains an invalid character range (e.g. [b-a])");
        CASE(error_space, "There was not enough memory to convert the expression into a finite state machine");
        CASE(error_badrepeat, "one of *?+{ was not preceded by a valid regular expression");
        CASE(error_complexity, "The complexity of an attempted match exceeded a predefined level");
        CASE(error_stack, "There was not enough memory to perform a match");
#if defined(__APPLE__) || defined(__FreeBSD__)
        CASE(__re_err_grammar, "__re_err_grammar");
        CASE(__re_err_empty, "__re_err_empty");
        CASE(__re_err_unknown, "__re_err_unknown");
        CASE(__re_err_parse, "__re_err_parse");
#endif
    }

#   undef CASE

    return err_message;
}

struct MyRegex {
    std::regex regex;
    bool success;

    explicit MyRegex(const std::string &pattern) {
        success = true;
        try {
            regex = pattern;
        } catch (const std::regex_error &ex) {
            LogError("pattern={}, error={}", pattern, show_regex_error(ex));
            success = false;
        }
    }

    bool search(const std::string &input, std::smatch &matched) const {
        return std::regex_search(input, matched, this->regex);
    }

    bool search(const std::string &input) const {
        return std::regex_search(input, this->regex);
    }
};

int RegexUtil::match_first(const std::string &input, const std::string &pattern, std::string &out) {
    // LogInfo("input={}, pattern={}", input, pattern);
    MyRegex regex{pattern};
    if (!regex.success) {
        return -1;
    }

    int ret = -3; // nomatch
    std::smatch matched;
    if (regex.search(input, matched)) {
        LogDebug("input={}, pattern={}, size={}, position={}, length={}", input, pattern, matched.size(),
                 matched.position(0), matched.length(0));
        out = matched[0];
        ret = static_cast<int>(matched.position(0) + matched.length(0));
    }

    return ret;
}

bool RegexUtil::match(const std::string &input, const std::string &pattern) {
    MyRegex regex{pattern};
    return regex.success && regex.search(input);
}

int RegexUtil::match(const std::string &input, const std::string &pattern, size_t count, std::vector<std::string> &d) {
    MyRegex regex{pattern};
    if (!regex.success) {
        return 0;
    }

    int ret = 0;
    std::smatch matched;
    if (regex.search(input, matched)) {
        ret = 1;
        LogDebug("input={}, pattern={}, size={}, position={}, length={}", input, pattern, matched.size(),
                 matched.position(0), matched.length(0));

        count = std::min({count, static_cast<size_t>(MaxMatch)}); // 不超过10个
        d.reserve(std::min({matched.size(), count})); // 预分配内存
        for (size_t i = 1; i < matched.size() && d.size() < count; i++) {
            d.push_back(matched[i]);
        }
    }
    return ret;
}