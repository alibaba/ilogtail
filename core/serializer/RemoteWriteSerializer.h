#pragma once

#include "serializer/Serializer.h"

namespace logtail {

class RemoteWriteEventGroupSerializer : public EventGroupSerializer {
public:
    RemoteWriteEventGroupSerializer(Flusher* f) : Serializer<BatchedEvents>(f) {}
    bool Serialize(BatchedEvents&& p, std::string& res, std::string& errorMsg) override;
};


} // namespace logtail