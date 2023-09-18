#pragma once

#include "SlsCoding.h"
#include <vector>
#include <cassert>

namespace apsara::sls::spl {

#define IF_NULL_RETURN_FALSE(ptr)  { if (ptr == NULL) { return false;} }
#define IF_FAIL_RETURN_FALSE(exp)  { if ((exp) == false) { return false;}}
#define IF_CONDITION_RETURN_FALSE(condition) { if (condition) { return false;}}

class SlsPbValidator {
  static bool IsValidPair(const char *ptr, const char *ptr_end) {
    // only two field exist , utf-8 is not checked
    //  string key = 1;
    //  string value = 2;
    for (uint32_t i = 1; i <= 2; i++) {
      uint32_t head = 0;
      ptr = GetVarint32Ptr(ptr, ptr_end, &head);
      IF_NULL_RETURN_FALSE(ptr);
      uint32_t mode = head & 0x7;
      uint32_t index = head >> 3;
      IF_CONDITION_RETURN_FALSE(index != i || mode != 2);
      uint32_t len = 0;
      ptr = GetVarint32Ptr(ptr, ptr_end, &len);
      IF_CONDITION_RETURN_FALSE(ptr == NULL || ptr + len > ptr_end);
      ptr += len;
    }
    return ptr == ptr_end;
  }

  static bool IsValidTag(const char *ptr, const char *ptr_end) {
    return IsValidPair(ptr, ptr_end);
  }

  static bool IsValidLog(const char *ptr, const char *ptr_end) {
    bool has_time = false;
    while (ptr < ptr_end) {
      uint32_t head = 0;
      ptr = GetVarint32Ptr(ptr, ptr_end, &head);
      IF_NULL_RETURN_FALSE(ptr);
      uint32_t mode = head & 0x7;
      uint32_t index = head >> 3;
      if (index == 1) {
        IF_CONDITION_RETURN_FALSE(mode != 0 || has_time);
        has_time = true;
        uint32_t data_time;
        ptr = GetVarint32Ptr(ptr, ptr_end, &data_time);
        IF_NULL_RETURN_FALSE(ptr);
      } else if (index == 2) {
        IF_CONDITION_RETURN_FALSE(mode != 2);
        uint32_t len = 0;
        ptr = GetVarint32Ptr(ptr, ptr_end, &len);
        IF_CONDITION_RETURN_FALSE(ptr == NULL || ptr + len > ptr_end);
        IF_FAIL_RETURN_FALSE(IsValidPair(ptr, ptr + len));
        ptr += len;
      } else {
        return false;
      }
    }
    return has_time && ptr == ptr_end;
  }

public :
  static bool IsValidLogGroup(const char *ptr, int32_t len, std::string &topic_value) {
    const char *ptr_end = ptr + len;
    while (ptr < ptr_end) {
      uint32_t head = 0;
      ptr = GetVarint32Ptr(ptr, ptr_end, &head);
      IF_NULL_RETURN_FALSE(ptr);
      uint32_t mode = head & 0x7;
      uint32_t index = head >> 3;
      if (index >= 1 && index <= 6) {
        // all the mode for index [1, 6] should be 2
        IF_CONDITION_RETURN_FALSE(mode != 2);
        uint32_t len = 0;
        ptr = GetVarint32Ptr(ptr, ptr_end, &len);
        IF_CONDITION_RETURN_FALSE (ptr == NULL || ptr + len > ptr_end)
        if (index == 1) {
          // check log
          IF_FAIL_RETURN_FALSE(IsValidLog(ptr, ptr + len));
        } else if (index == 3) {
          topic_value = std::string(ptr, len);
        } else if (index == 6) {
          // check tag;
          IF_FAIL_RETURN_FALSE(IsValidTag(ptr, ptr + len));
        }
        ptr += len;
      } else {
        return false;
      }
    }
    return true;
  }
};

/**
 * 偏移及长度
 */
struct SlsPbOffsetLen {
  int32_t mOffset;
  int32_t mLen;

  SlsPbOffsetLen() :
      mOffset(0),
      mLen(0) {
  }

  SlsPbOffsetLen(int32_t offset, int32_t len) :
      mOffset(offset),
      mLen(len) {
  }
};

/**
 * 字符串片段
 */
struct SlsStringPiece {
  const char *mPtr;
  int32_t mLen;

  SlsStringPiece(const char *ptr, int32_t len) : mPtr(ptr), mLen(len) {
  }

  SlsStringPiece() : mPtr(NULL), mLen(0) {
  }

  std::string ToString() const {
    if (mPtr == NULL) {
      return std::string();
    }
    return std::string(mPtr, mLen);
  }
};


static bool ParseKeyValuePb(const char *ptr, const char *ptr_end, SlsStringPiece &key, SlsStringPiece &value) {
  // only two field exist , utf-8 is not checked
  //  string key = 1;
  //  string value = 2;
  for (uint32_t i = 1; i <= 2; i++) {
    uint32_t head = 0;
    ptr = GetVarint32Ptr(ptr, ptr_end, &head);
    IF_NULL_RETURN_FALSE(ptr);
    uint32_t mode = head & 0x7;
    uint32_t index = head >> 3;
    IF_CONDITION_RETURN_FALSE(index != i || mode != 2);
    uint32_t len = 0;
    ptr = GetVarint32Ptr(ptr, ptr_end, &len);
    IF_CONDITION_RETURN_FALSE(ptr == NULL || ptr + len > ptr_end);
    if (i == 1) {
      key.mPtr = ptr;
      key.mLen = len;
    } else {
      value.mPtr = ptr;
      value.mLen = len;
    }
    ptr += len;
  }
  return ptr == ptr_end;
}

/**
 * 标签
 */
struct SlsTagFlatPb {
  const char *mPtr;
  int32_t mLen;

  SlsTagFlatPb(const char *ptr, int32_t len) : mPtr(ptr), mLen(len) {
  }

  bool GetTagKeyValue(SlsStringPiece &key, SlsStringPiece &value) const {
    return ParseKeyValuePb(mPtr, mPtr + mLen, key, value);
  }
};

/**
 * 日志
 */
struct SlsLogFlatPb {
  /**
   * 指针
   */
  const char *mPtr;
  /**
   * 长度
   */
  int32_t mLen;
  /**
   * 时间
   */
  uint32_t mTime;

  /**
   * 构造函数
   *
   * @param ptr
   * @param len
   */
  SlsLogFlatPb(const char *ptr, int32_t len) : mPtr(ptr), mLen(len), mTime(0) {
  }

  /**
   * 获取时间
   *
   * @return
   */
  uint32_t GetTime() const {
    return mTime;
  }

  /**
   * 解码
   *
   * @return
   */
  bool Decode() {
    // 开始指针
    const char *ptr = mPtr;
    // 结束指针
    const char *ptr_end = mPtr + mLen;
    // 遍历：指针
    while (ptr < ptr_end) {
      uint32_t head = 0;
      ptr = GetVarint32Ptr(ptr, ptr_end, &head);
      IF_NULL_RETURN_FALSE(ptr);
      // 索引
      uint32_t index = head >> 3;
      if (index == 1) {
        // if：索引 == 1，则为时间

        ptr = GetVarint32Ptr(ptr, ptr_end, &mTime);
        IF_NULL_RETURN_FALSE(ptr);
        return mTime > 0;
      } else if (index == 2) {
        // if：索引 == 2，则为kv

        uint32_t len = 0;
        ptr = GetVarint32Ptr(ptr, ptr_end, &len);
        if (ptr == NULL || ptr + len > ptr_end) {
          return false;
        }
        ptr += len;
      } else {
        break;
      }
    }
    return false;
  }


  /**
   * 日志读取器
   */
  struct SlsLogFlatPbReader {
  private :
    const char *mPtr;
    const char *mEndPtr;
  public :
    /**
     * 构造函数
     *
     * @param ptr
     * @param ptr_end
     */
    SlsLogFlatPbReader(const char *ptr, const char *ptr_end) : mPtr(ptr), mEndPtr(ptr_end) {
    }

    /**
     * 获取下一个kv
     *
     * @param key
     * @param value
     * @return
     */
    bool getNextKeyValue(SlsStringPiece &key, SlsStringPiece &value) {
      // 遍历：指针
      while (mPtr < mEndPtr) {
        // 头
        uint32_t head = 0;
        mPtr = GetVarint32Ptr(mPtr, mEndPtr, &head);
        IF_NULL_RETURN_FALSE(mPtr);
        // 模式
        uint32_t mode = head & 0x7;
        // 索引
        uint32_t index = head >> 3;
        if (index == 1) {
          // if：索引 == 1

          uint32_t data_time = 0;
          mPtr = GetVarint32Ptr(mPtr, mEndPtr, &data_time);
          IF_NULL_RETURN_FALSE(mPtr);
        } else if (index == 2) {
          // if：索引 == 2

          key.mLen = 0;
          value.mLen = 0;
          IF_CONDITION_RETURN_FALSE(mode != 2);
          uint32_t len = 0;
          mPtr = GetVarint32Ptr(mPtr, mEndPtr, &len);
          IF_CONDITION_RETURN_FALSE(mPtr == NULL || mPtr + len > mEndPtr);
          IF_FAIL_RETURN_FALSE(ParseKeyValuePb(mPtr, mPtr + len, key, value));
          mPtr += len;
          return true;
        } else {
          return false;
        }
      }
      return false;
    }
  };

  /**
   * 获取日志读取器
   *
   * @return
   */
  SlsLogFlatPbReader GetReader() const {
    return SlsLogFlatPbReader(mPtr, mPtr + mLen);
  }
};


/**
 * 日志组
 */
struct SlsLogGroupFlatPb {
  const char *mDataPtr;
  int32_t mDataLen;
  std::vector<SlsLogFlatPb> mFlatLogs;
  std::vector<SlsPbOffsetLen> mTagsOffset;
  bool mInitFromLogGroupList;
  SlsPbOffsetLen mTopic;
  SlsPbOffsetLen mSource;

  /**
   * 构造函数
   *
   * @param data_ptr
   * @param data_len
   * @param initFromLogGroupList
   */
  SlsLogGroupFlatPb(const char *data_ptr, int32_t data_len, bool initFromLogGroupList = false) :
      mDataPtr(data_ptr), mDataLen(data_len), mInitFromLogGroupList(initFromLogGroupList) {
  }

  /**
   * 析构函数
   */
  ~SlsLogGroupFlatPb() {
    if (!mInitFromLogGroupList) {
      delete mDataPtr;
      mDataPtr = NULL;
    }
  }

  /**
   * 解码
   *
   * @return
   */
  bool Decode() {
    // 开始指针
    const char *ptr = mDataPtr;
    // 结束指针
    const char *ptr_end = ptr + mDataLen;
    // 遍历：指针
    while (ptr < ptr_end) {
      // 头
      uint32_t head = 0;
      ptr = GetVarint32Ptr(ptr, ptr_end, &head);
      IF_NULL_RETURN_FALSE(ptr);
      // 模式
      uint32_t mode = head & 0x7;
      // 索引
      uint32_t index = head >> 3;
      if (index >= 1 && index <= 6) {
        // all the mode for index [1, 6] should be 2
        IF_CONDITION_RETURN_FALSE(mode != 2);
        // 长度
        uint32_t len = 0;
        ptr = GetVarint32Ptr(ptr, ptr_end, &len);
        IF_CONDITION_RETURN_FALSE (ptr == NULL || ptr + len > ptr_end)
        if (index == 1) {
          // if：索引 == 1

          SlsLogFlatPb flat_pb(ptr, len);
          if (flat_pb.Decode() == false) {
            return false;
          }
          mFlatLogs.push_back(flat_pb);
        } else if (index == 3) {
          // if：索引 == 3

          mTopic.mOffset = ptr - mDataPtr;
          mTopic.mLen = len;
        } else if (index == 4) {
          // if：索引 == 4

          mSource.mOffset = ptr - mDataPtr;
          mSource.mLen = len;
        } else if (index == 6) {
          // if：索引 == 6

          SlsPbOffsetLen offset_len(ptr - mDataPtr, len);
          mTagsOffset.push_back(offset_len);
        }
        ptr += len;
      } else {
        break;  // it should never happen
      }
    }
    return true;
  }

  /**
   * 获取日志数量
   *
   * @return
   */
  int32_t GetLogsSize() const {
    return mFlatLogs.size();
  }

  /**
   * 获取日志
   *
   * @param i
   * @return
   */
  const SlsLogFlatPb &GetLogFlatPb(int32_t i) const {
    if (i >= 0 && i < (int32_t) mFlatLogs.size()) {
      return mFlatLogs[i];
    }
    const static SlsLogFlatPb s_empty_pb(NULL, 0);
    return s_empty_pb;
  }

  /**
   * 获取标签数量
   *
   * @return
   */
  int32_t GetTagsSize() const {
    return mTagsOffset.size();
  }

  /**
   * 获取标签
   *
   * @param i
   * @return
   */
  SlsTagFlatPb GetTagFlatPb(int32_t i) const {
    if (i >= 0 && i < (int32_t) mTagsOffset.size()) {
      const SlsPbOffsetLen &offset_len = mTagsOffset[i];
      return SlsTagFlatPb(mDataPtr + offset_len.mOffset, offset_len.mLen);
    }
    return SlsTagFlatPb(NULL, 0);
  }

  /**
   * 获取主题
   *
   * @param topic
   * @return
   */
  bool GetTopic(SlsStringPiece &topic) const {
    if (mTopic.mLen > 0) {
      topic.mPtr = mDataPtr + mTopic.mOffset;
      topic.mLen = mTopic.mLen;
      return true;
    }
    return false;
  }

  /**
   * 获取主题
   *
   * @param ptr
   * @param len
   * @return
   */
  bool GetTopic(const char *&ptr, int32_t &len) {
    if (mTopic.mLen > 0) {
      ptr = mDataPtr + mTopic.mOffset;
      len = mTopic.mLen;
      return true;
    }
    return false;
  }

  /**
   * 获取主题
   *
   * @return
   */
  std::string GetTopic(void) const {
    if (mTopic.mLen > 0) {
      return std::string(mDataPtr + mTopic.mOffset, mTopic.mLen);
    }
    return "";
  }

  /**
   * 获取源
   *
   * @param source
   * @return
   */
  bool GetSource(SlsStringPiece &source) const {
    if (mSource.mLen > 0) {
      source.mPtr = mDataPtr + mSource.mOffset;
      source.mLen = mSource.mLen;
      return true;
    }
    return false;
  }

  /**
   * 获取源
   *
   * @param ptr
   * @param len
   * @return
   */
  bool GetSource(const char *&ptr, int32_t &len) {
    if (mSource.mLen > 0) {
      ptr = mDataPtr + mSource.mOffset;
      len = mSource.mLen;
      return true;
    }
    return false;
  }

  /**
   * 获取源
   *
   * @return
   */
  std::string GetSource(void) const {
    if (mSource.mLen > 0) {
      return std::string(mDataPtr + mSource.mOffset, mSource.mLen);
    }
    return "";
  }
};

/**
 * 日志组列表
 */
struct SlsLogGroupListFlatPb {
  const char *mDataPtr;
  int32_t mDataLen;
  std::vector<SlsLogGroupFlatPb> mFlatLogGroups;

  /**
   * 构造函数
   *
   * @param data_ptr
   * @param data_len
   */
  SlsLogGroupListFlatPb(const char *data_ptr, int32_t data_len) :
      mDataPtr(data_ptr), mDataLen(data_len) {
  }

  /**
   * 解码
   *
   * @return
   */
  bool Decode() {
    const char *ptr = mDataPtr;
    const char *ptr_end = ptr + mDataLen;
    while (ptr < ptr_end) {
      uint32_t head = 0;
      ptr = GetVarint32Ptr(ptr, ptr_end, &head);
      IF_NULL_RETURN_FALSE(ptr);
      uint32_t mode = head & 0x7;
      uint32_t index = head >> 3;
      if (index == 1) {
        IF_CONDITION_RETURN_FALSE(mode != 2);
        uint32_t len = 0;
        ptr = GetVarint32Ptr(ptr, ptr_end, &len);
        IF_CONDITION_RETURN_FALSE (ptr == NULL || ptr + len > ptr_end);
        SlsLogGroupFlatPb flat_pb(ptr, len, true);
        if (flat_pb.Decode() == false) {
          return false;
        }
        mFlatLogGroups.push_back(flat_pb);
        ptr += len;
      } else {
        break;  // it should never happen
      }
    }
    return true;
  }

  /**
   * 获取日志组数量
   *
   * @return
   */
  uint32_t GetLogGroupSize() const {
    return mFlatLogGroups.size();
  }

  /**
   * 获取日志组
   *
   * @param i
   * @return
   */
  const SlsLogGroupFlatPb &GetLogGroupFlatPb(int32_t i) const {
    if (i >= 0 && i < (int32_t) mFlatLogGroups.size()) {
      return mFlatLogGroups[i];
    }
    const static SlsLogGroupFlatPb s_empty_pb(NULL, 0);
    return s_empty_pb;
  }
};


#undef IF_NULL_RETURN_FALSE
#undef IF_FAIL_RETURN_FALSE
#undef IF_CONDITION_RETURN_FALSE

}

