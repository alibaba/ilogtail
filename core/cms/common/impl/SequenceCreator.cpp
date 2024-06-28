#include "common/SequenceCreator.h"
#include "common/ScopeGuard.h"

namespace common {
    SequenceCreator::SequenceCreator() {
        m_seq = 0;
    }

    int SequenceCreator::getSequence() {
        m_locker.lock();
        defer(m_locker.unlock());

        int seq = ++m_seq;
        if (seq < 0) {
            // 溢出则复位
            m_seq = seq = 1;
        }

        return seq;
    }
}
