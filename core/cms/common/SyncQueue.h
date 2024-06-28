//
// Created by 韩呈杰 on 2023/4/14.
//
#ifndef ARGUSAGENT_COMMON_SYNC_QUEUE_H
#define ARGUSAGENT_COMMON_SYNC_QUEUE_H

#include <list>
#include <mutex>
#include <memory>
#include <limits>
#include <chrono>
#include <boost/thread/condition_variable.hpp>
#ifdef max
#   undef max
#endif

enum {
    Block = true,
    Nonblock = false,
};

template<typename TMutex, typename TQueue, typename T>
bool PopQueue(TMutex &mutex, TQueue &queue, T &data) {
    std::lock_guard<TMutex> guard(mutex);
    bool has = !queue.empty();
    if (has) {
        data = queue.front();
        queue.pop_front();
    }
    return has;
}

namespace sync_queue {
    namespace details {
        template<typename T>
        class queue_base_t {
        protected:
            mutable std::mutex mutex_;
            boost::condition_variable_any cv_;
            std::list<T> data_inner;
            std::list<T> &data_;
            bool closed_ = false;   // 队列是否已关闭
        public:
            const size_t capacity_;  // data_容量
            explicit queue_base_t(size_t capacity, std::list<T> *data = nullptr) :
                    data_{data == nullptr ? data_inner : *data},
                    capacity_(capacity == 0 ? 1 : capacity) {
            }

            bool is_closed() const {
                std::unique_lock<std::mutex> lock(this->mutex_);
                return this->closed_;
            }

            size_t count() const {
                std::unique_lock<std::mutex> lock(this->mutex_);
                return this->data_.size();
            }

            template<typename TR, typename Rep, typename Period>
            bool pop(TR &data, const std::chrono::duration<Rep, Period> &timeout, bool waitZero) {
                std::unique_lock<std::mutex> l(this->mutex_);
                if (!wait(l, timeout, waitZero, [&]() { return !is_empty() || closed_; })) {
                    // 超时(或在无需等待时，无数据)
                    return false;
                }

                if (this->is_empty()) {
                    return false;  // 已关闭
                }

                data = this->data_.front();
                this->data_.pop_front();

                if (this->free_count() == 1) {
                    // 说明以前是full或溢出状态
                    this->cv_.notify_all();
                }

                return true;
            }

        protected:
            bool is_empty() const {
                return data_.empty();
            }

            size_t free_count() const {
                return capacity_ - data_.size();
            }

            template<typename Rep, typename Period>
            bool wait(std::unique_lock<std::mutex> &lock, const std::chrono::duration<Rep, Period> &timeout,
                      bool waitZero, const std::function<bool()> &stopWaiting) {
                bool ok = true;
                auto micros = std::chrono::duration_cast<std::chrono::microseconds>(timeout).count();
                if (micros <= 0) {
                    if (waitZero) {
                        this->cv_.wait(lock, stopWaiting);
                    } else {
                        ok = stopWaiting();
                    }
                } else {
                    ok = this->cv_.wait_for(lock, boost::chrono::microseconds(micros), stopWaiting);
                }
                return ok;
            }
        };

        template<typename T, bool blockType = Block>
        class queue_t : public queue_base_t<T> {
        protected:
            const bool zero_capacity;

            bool is_overflow() const {
                return zero_capacity && this->data_.size() >= this->capacity_;
            }

        public:
            // capacity_为0时，允许放入一个，但_queue会处于overflow状态
            explicit queue_t(size_t capacity, std::list<T> *data = nullptr)
                    : queue_base_t<T>(capacity, data), zero_capacity(capacity == 0) {
            }

            // close以后的入chan操作会返回false, 而出chan则在队列为空后才返回false
            void close() {
                std::unique_lock<std::mutex> lock(this->mutex_);
                if (!this->closed_) {
                    this->closed_ = true;
                    if (this->is_overflow()) {
                        // 消除溢出
                        this->data_.pop_back();
                    }
                    this->cv_.notify_all();
                }
            }

            // 当zero_capacity时， timeout只是排队时间，一但入队列，则会无限期的等待，直到队列关闭或被消费掉
            template<typename TR, typename Rep, typename Period>
            bool push(TR &&data, const std::chrono::duration<Rep, Period> &timeout, bool waitZero) {
                std::unique_lock<std::mutex> l(this->mutex_);
                if (!this->wait(l, timeout, waitZero, [this]() { return this->free_count() > 0 || this->closed_; })) {
                    // 超时(或无需等待时，队列满)
                    return false;
                }
                if (this->closed_) {
                    return false;
                }

                this->data_.push_back(std::forward<TR>(data));
                if (this->data_.size() == 1) {
                    this->cv_.notify_all();
                }

                // 当queue溢出,需等待queue回复正常
                if (this->is_overflow()) {
                    // 在zero_capacity为true的情况下，pop时会通知到两类对象:
                    // 一: 其它push对象
                    // 二: 当前的等待对象
                    this->cv_.wait(l);
                }

                return !this->closed_;
            }
        };

        // 非阻塞push, 汰旧留新
        template<typename T>
        class queue_t<T, Nonblock> : public queue_base_t<T> {
            size_t discardCount_ = 0;
        public:
            using queue_base_t<T>::queue_base_t;

            // close以后的入chan操作会返回false, 而出chan则在队列为空后才返回false
            void close() {
                std::unique_lock<std::mutex> lock(this->mutex_);
                if (!this->closed_) {
                    this->closed_ = true;
                    this->cv_.notify_all();
                }
            }

            template<typename TR>
            bool push(TR &&data) {
                std::unique_lock<std::mutex> lock(this->mutex_);
                if (this->closed_) {
                    return false;
                }

                this->data_.push_back(std::forward<TR>(data));
                const size_t size = this->data_.size();
                if (size == 1) {
                    this->cv_.notify_all();
                } else if (size > this->capacity_) {
                    ++discardCount_;
                    this->data_.pop_front(); // 队列溢出，挤掉老的
                }

                return !this->closed_;
            }

            size_t discardCount() const {
                return this->discardCount_;
            }
        };
    }
}

template<typename T, bool blockType>
class SyncQueueBase {
protected:
    // 允许拷贝、move
    std::shared_ptr<sync_queue::details::queue_t<T, blockType>> queue_;

protected:
    explicit SyncQueueBase(size_t capacity = 0, std::list<T> *data = nullptr) {
        queue_ = std::make_shared<sync_queue::details::queue_t<T, blockType>>(capacity, data);
    }

public:
    // 出chan(支持兼容类型的出chan)
    template<typename TR>
    SyncQueueBase &operator>>(TR &data) {
        queue_->pop(data, std::chrono::milliseconds::zero(), true);
        return *this;
    }

    template<typename TR>
    bool Take(TR &x) {
        return queue_->pop(x, std::chrono::seconds::zero(), true);
    }

    template<typename TR, typename Rep, typename Period>
    bool Take(TR &x, std::chrono::duration<Rep, Period> timeout) {
        return queue_->pop(x, timeout, false);
    }

    template<typename TR>
    bool TryTake(TR &x) {
        return queue_->pop(x, std::chrono::seconds::zero(), false);
    }

    // 不关心队列中实际数据，只关注是否有。true - 成功， false - 失败
    template<typename Rep, typename Period>
    bool Wait(const std::chrono::duration<Rep, Period> &timeout) {
        T x;
        return queue_->pop(x, timeout, true);
    }

    bool Wait() {
        return Wait(std::chrono::seconds::zero());
    }

    // close以后的入chan操作返回false, 而出chan则在队列为空后才返回false
    void Close() {
        queue_->close();
    }

    bool IsClosed() const {
        return queue_->is_closed();
    }

    size_t Count() const {
        return queue_->count();
    }

    size_t Size() const {
        return Count();
    }

    size_t Capacity() const {
        return queue_->capacity_;
    }
};

template<typename T, bool blockType = Block>
class SyncQueue : public SyncQueueBase<T, blockType> {
public:
    explicit SyncQueue(size_t capacity = 0, std::list<T> *data = nullptr)
            : SyncQueueBase<T, blockType>(capacity, data) {
    }

    explicit SyncQueue(std::list<T> &data, size_t capacity = std::numeric_limits<size_t>::max())
            : SyncQueue(capacity, &data) {
    }

    // 入chan，支持move语义
    template<typename TR>
    SyncQueue &operator<<(TR &&data) {
        this->queue_->push(std::forward<TR>(data), std::chrono::milliseconds::zero(), true);
        return *this;
    }

    template<typename TR, typename Rep, typename Period>
    bool Push(TR &&data, const std::chrono::duration<Rep, Period> &timeout) {
        return this->queue_->push(std::forward<TR>(data), timeout, false);
    }

    template<typename TR>
    bool Push(TR &&data) {
        return this->queue_->push(std::forward<TR>(data), std::chrono::seconds::zero(), true);
    }
};

template<typename T>
class SyncQueue<T, Nonblock> : public SyncQueueBase<T, Nonblock> {
public:
    explicit SyncQueue(size_t capacity = 1, std::list<T> *data = nullptr)
            : SyncQueueBase<T, Nonblock>(capacity, data) {
    }

    // 使用自定义list
    explicit SyncQueue(std::list<T> &data, size_t capacity = 1)
            : SyncQueue(capacity, &data) {
    }

    // 入chan，支持move语义
    template<typename TR>
    SyncQueue &operator<<(TR &&data) {
        this->queue_->push(std::forward<TR>(data));
        return *this;
    }

    template<typename TR>
    bool Push(TR &&data) {
        return this->queue_->push(std::forward<TR>(data));
    }

    size_t DiscardCount() const {
        return this->queue_->discardCount();
    }
};

#endif //!ARGUSAGENT_COMMON_SYNC_QUEUE_H
