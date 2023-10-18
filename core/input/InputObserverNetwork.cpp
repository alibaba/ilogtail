#include "input/InputObserverNetwork.h"
#include "InputObserverNetwork.h"

using namespace std;

namespace logtail {
const std::string InputObserverNetwork::sName = "input_observer_network";

bool logtail::InputObserverNetwork::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    mDetail = config.toStyledString();
    optionalGoPipeline["mix_process_mode"] = "observer";
    return true;
}

bool logtail::InputObserverNetwork::Start() {
    return true;
}

bool logtail::InputObserverNetwork::Stop(bool isPipelineRemoving) {
    return true;
}
} // namespace logtail
