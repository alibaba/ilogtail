#ifndef CMS_SLIDING_WINDOW_H
#define CMS_SLIDING_WINDOW_H

namespace cloudMonitor {
    template<class T>
    class SlidingWindow {
    public:
        typedef T Type;
        explicit SlidingWindow(size_t num);

        ~SlidingWindow();

        void Update(T value);

        void GetValues(std::vector<T> &values) const;
        T Max() const;
        T Min() const;
        T Last() const;
        double Avg() const;

        int Size() const {
            return static_cast<int>(mCount > mCapacity ? mCapacity : mCount);
        }

        size_t AccumulateCount() const {
            return mCount;
        }

    private:
        const size_t mCapacity;
        size_t mCount = 0;
        T *mValues = nullptr;
    };

    template<class T>
    SlidingWindow<T>::SlidingWindow(size_t num)
            :mCapacity(num) {
        mCount = 0;
        mValues = new T[mCapacity];
    }

    template<class T>
    SlidingWindow<T>::~SlidingWindow() {
        if (mValues != nullptr) {
            delete[] mValues;
            mValues = nullptr;
            mCount = 0;
        }
    }

    template<class T>
    void SlidingWindow<T>::Update(T value) {
        size_t index = mCount % mCapacity;
        mCount++;
        if (mCount >= 2 * mCapacity) {
            mCount -= mCapacity;
        }
        mValues[index] = value;
    }

    template<class T>
    void SlidingWindow<T>::GetValues(std::vector<T> &values) const {
        values.clear();
        int num = this->Size();
        for (int i = 0; i < num; i++) {
            values.push_back(mValues[i]);
        }
    }

    template<class T>
    T SlidingWindow<T>::Last() const {
        return mValues[mCount > 0 ? ((mCount - 1) % mCapacity) : 0];
    }

    template<class T>
    T SlidingWindow<T>::Max() const {
        int num = this->Size();
        if (num == 0) {
            return 0;
        }
        T max = mValues[0];
        for (int i = 1; i < num; i++) {
            if (max < mValues[i]) {
                max = mValues[i];
            }
        }
        return max;
    }

    template<class T>
    T SlidingWindow<T>::Min() const {
        int num = this->Size();
        if (num == 0) {
            return 0;
        }
        T min = mValues[0];
        for (int i = 1; i < num; i++) {
            if (min > mValues[i]) {
                min = mValues[i];
            }
        }
        return min;
    }

    template<class T>
    double SlidingWindow<T>::Avg() const {
        int num = this->Size();
        if (num == 0) {
            return 0.0;
        }

        double total = 0;
        for (int i = 0; i < num; i++) {
            total += mValues[i];
        }
        return total / num;
    }
}
#endif