#pragma once

#include "SlsCoding.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "SlsLogPbParser.h"
#include "MemoryBuffer.h"
#include "sls_allocator.h"
#include "sls_protobuf_tag.h"
#include <vector>

namespace apsara::sls::spl {
/**
* Pack an unsigned 32-bit integer in base-128 varint encoding and return the
* number of bytes written, which must be 5 or less.
*
* \param value
*      Value to encode.
* \param[out] out
*      Packed value.
* \return
*      Number of bytes written to `out`.
*/
static size_t uint32_pack(uint32_t value, uint8_t* out)
{
    unsigned rv = 0;

    if (value >= 0x80) {
        out[rv++] = value | 0x80;
        value >>= 7;
        if (value >= 0x80) {
            out[rv++] = value | 0x80;
            value >>= 7;
            if (value >= 0x80) {
                out[rv++] = value | 0x80;
                value >>= 7;
                if (value >= 0x80) {
                    out[rv++] = value | 0x80;
                    value >>= 7;
                }
            }
        }
    }
    /* assert: value<128 */
    out[rv++] = value;
    return rv;
}

struct SlsLogWriter
{
    SlsLogWriter(MemoryArena* memoryArena) :
        mMemoryArena(memoryArena),
        mMemoryBuffer(memoryArena)
    {}
    ~SlsLogWriter()
    {}

    void SetTime(uint32_t logTime)
    {
        mMemoryBuffer.WriteBuffer(&protobuf_tag::sls_logs_log_time, 1);
        WriteVarInt32(logTime);
    }

    void SetTimeNs(uint32_t logTimeNs)
    {
        mMemoryBuffer.WriteBuffer(&protobuf_tag::sls_logs_log_time_ns, 1);
        WriteFixedInt32(logTimeNs);
    }

    void AddContent(const char* key, size_t key_len, const char* value, size_t value_len)
    {
        uint32_t kv_size = (key_len + value_len) + VarintLength(key_len) + VarintLength(value_len) + 2;
        mMemoryBuffer.WriteBuffer(&protobuf_tag::sls_logs_log_contents, 1);
        WriteVarInt32(kv_size);

        mMemoryBuffer.WriteBuffer(&protobuf_tag::sls_logs_log_contents_key, 1);
        WriteVarInt32(key_len);
        mMemoryBuffer.WriteBuffer((const uint8_t*)key, key_len);

        mMemoryBuffer.WriteBuffer(&protobuf_tag::sls_logs_log_contents_value, 1);
        WriteVarInt32(value_len);
        mMemoryBuffer.WriteBuffer((const uint8_t*)value, value_len);
    }

    // currently not used
    void AddSchemaValue(const char* value, size_t value_len)
    {
        mMemoryBuffer.WriteBuffer(&protobuf_tag::sls_logs_log_values, 1);
        WriteVarInt32(value_len);
        mMemoryBuffer.WriteBuffer((const uint8_t*)value, value_len);
    }

    MemoryArena* mMemoryArena;
    MemoryBuffer mMemoryBuffer;

    void WriteVarInt32(uint32_t value)
    {
        uint8_t buf[8];
        mMemoryBuffer.WriteBuffer(buf, uint32_pack(value, buf));
    }
    void WriteFixedInt32(uint32_t value)
    {
        mMemoryBuffer.WriteBuffer((uint8_t *)&value, 4);
    }
    int32_t BufferSize() const
    {
        return mMemoryBuffer.Size();
    }
    uint8_t* PackToBuffer(uint8_t* buf) const
    {
        return mMemoryBuffer.PackToBuffer(buf);
    }
};

struct SlsLogGroupWriter
{
    SlsLogGroupWriter(MemoryArena* memoryArena) :
        mMemoryArena(memoryArena),
        mMemoryBuffer(memoryArena)
    {}
    ~SlsLogGroupWriter()
    {
        for (auto& log : mLogWriters)
        {
            if (log)
            {
                delete log;
            }
        }
        mLogWriters.clear();
    }

    void AddTag(const char* key, size_t key_len, const char* value, size_t value_len)
    {
        uint32_t kv_size = (key_len + value_len) + VarintLength(key_len) + VarintLength(value_len) + 2;

        mMemoryBuffer.WriteBuffer(&protobuf_tag::sls_logs_loggroup_tags, 1);
        WriteVarInt32(kv_size);

        mMemoryBuffer.WriteBuffer(&protobuf_tag::sls_logs_tag_key, 1);
        WriteVarInt32(key_len);
        mMemoryBuffer.WriteBuffer((const uint8_t*)key, key_len);

        mMemoryBuffer.WriteBuffer(&protobuf_tag::sls_logs_tag_value, 1);
        WriteVarInt32(value_len);
        mMemoryBuffer.WriteBuffer((const uint8_t*)value, value_len);
    }

    void AddTopic(const char* topic, size_t topic_len)
    {
        mMemoryBuffer.WriteBuffer(&protobuf_tag::sls_logs_loggroup_topic, 1);
        WriteVarInt32(topic_len);
        mMemoryBuffer.WriteBuffer((const uint8_t*)topic, topic_len);
    }

    void AddSource(const char* source, size_t source_len)
    {
        mMemoryBuffer.WriteBuffer(&protobuf_tag::sls_logs_loggroup_source, 1);
        WriteVarInt32(source_len);
        mMemoryBuffer.WriteBuffer((const uint8_t*)source, source_len);
    }

    SlsLogWriter* AddLog()
    {
        auto logWriter = new SlsLogWriter(mMemoryArena);
        mLogWriters.push_back(logWriter);
        return logWriter;
    }

    MemoryArena* mMemoryArena;
    MemoryBuffer mMemoryBuffer;
    std::vector<SlsLogWriter*> mLogWriters;

    void WriteVarInt32(uint32_t value)
    {
        uint8_t buf[8];
        mMemoryBuffer.WriteBuffer(buf, uint32_pack(value, buf));
    }
    void WriteFixedInt32(uint32_t value)
    {
        mMemoryBuffer.WriteBuffer((uint8_t *)&value, 4);
    }
    int32_t BufferSize() const
    {
        int32_t bufferSize = mMemoryBuffer.Size();
        for (const auto& log : mLogWriters)
        {
            auto tmpSize = log->BufferSize();
            bufferSize += 1 + VarintLength(tmpSize) + tmpSize;
        }
        return bufferSize;
    }
    uint8_t* PackToBuffer(uint8_t* buf) const
    {
        buf = mMemoryBuffer.PackToBuffer(buf);
        for (const auto& log : mLogWriters)
        {
            *buf++ = protobuf_tag::sls_logs_loggroup_logs;
            buf += uint32_pack(log->BufferSize(), buf);
            buf = log->PackToBuffer(buf);
        }
        return buf;
    }
};

struct SlsLogGroupListWriter
{
    ~SlsLogGroupListWriter()
    {
        for (auto& group : mLogGroupWriters)
        {
            if (group)
            {
                delete group;
            }
        }
        mLogGroupWriters.clear();
    }

    void AddLogGroup(SlsLogGroupWriter* writer)
    {
        mLogGroupWriters.push_back(writer);
    }

    uint8_t* PackToBuffer(uint8_t* buf) const
    {
        for (const auto& group : mLogGroupWriters)
        {
            *buf++ = protobuf_tag::sls_logs_log_group_list;
            buf += uint32_pack(group->BufferSize(), buf);
            buf = group->PackToBuffer(buf);
        }
        return buf;
    }
    int32_t BufferSize() const
    {
        int32_t bufferSize = 0;
        for (const auto& group : mLogGroupWriters)
        {
            auto tmpSize = group->BufferSize();
            bufferSize += 1 + VarintLength(tmpSize) + tmpSize;
        }
        return bufferSize;
    }
    std::vector<SlsLogGroupWriter*> mLogGroupWriters;
};

}
