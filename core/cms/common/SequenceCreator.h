#ifndef _SEQUENCE_CREATOR_H_
#define _SEQUENCE_CREATOR_H_

#include "Singleton.h"

namespace common {

#include "test_support"
class SequenceCreator {
public:
    SequenceCreator();

    int getSequence();

private:
    int m_seq;
    InstanceLock m_locker;
};
#include "test_support"

    typedef Singleton<SequenceCreator> SingletonSequenceCreator;
}

#endif
