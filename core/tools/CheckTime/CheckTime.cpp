// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdlib>
#include "common/TimeUtil.h"

#include <string>
#include <vector>

using namespace std;
using namespace logtail;

bool CheckTimeFormat(const std::string& timeValue, const std::string& timeFormat) {
    LogtailTime logTime = {0, 0};

    const char* compareResult = strstr(timeFormat.c_str(), "%f");
    bool haveNanosecond = compareResult != nullptr;
    bool endWithNanosecond = compareResult == (timeFormat.c_str() + timeFormat.size() - 2);
    int nanosecondLength = -1;
    const char* strptimeResult = NULL;
    strptimeResult = Strptime(timeValue.c_str(), timeFormat.c_str(), &logTime, nanosecondLength, -1);
    if (NULL == strptimeResult) {
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc >= 3) {
        cout << argv[1] << endl;
        cout << argv[2] << endl;

        if (CheckTimeFormat(argv[1], argv[2])) {
            cout << "TRUE" << endl;
        } else {
            cout << "FALSE" << endl;
        }
    } else {
        // ./CheckTime timestamp format
        cout << "FALSE" << endl;
    }

    return 0;
}
