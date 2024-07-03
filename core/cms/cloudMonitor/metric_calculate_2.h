//
// Created by 韩呈杰 on 2023/2/5.
//

#ifndef ARGUSAGENT_METRIC_CALCULATE_2_H
#define ARGUSAGENT_METRIC_CALCULATE_2_H
#ifdef max
#	undef max
#endif
#ifdef min
#	undef min
#endif
#include <algorithm>
#include <memory>
#include "common/FieldEntry.h"

namespace cloudMonitor {
    // TFieldMeta必须 实现value()，且[begin, end)可遍历TMetric的字段
    template<typename TMetric, typename TField = double>
    class MetricCalculate2 {
    public:
        typedef FieldName<TMetric, TField> FieldMeta;
        void Reset() {
            mCount = 0;
        }

        void AddValue(const TMetric &v) {
            mCount++;
            if (1 == mCount) {
                enumerate([&](const FieldMeta &field) {
                    const TField &metricValue = field.value(v);

                    field.value(mMax) = metricValue;
                    field.value(mMin) = metricValue;
                    field.value(mTotal) = metricValue;
                    field.value(mLast) = metricValue;
                });
            } else {
                enumerate([&](const FieldName<TMetric, TField> &field) {
                    const TField &metricValue = field.value(v);

                    field.value(mMax) = std::max(field.value(mMax), metricValue);
                    field.value(mMin) = std::min(field.value(mMin), metricValue);
                    field.value(mTotal) += metricValue;
                    field.value(mLast) = metricValue;
                });
            }
            // for (auto it = metaBegin; it < metaEnd; it++) {
            //     TFieldType metricValue = it->value(v);
            //
            //     if (mCount == 1) {
            //         it->value(mMax) = metricValue;
            //         it->value(mMin) = metricValue;
            //     } else {
            //         it->value(mMax) = std::max(it->value(mMax), metricValue);
            //         it->value(mMin) = std::min(it->value(mMin), metricValue);
            //     }
            //     it->value(mTotal) += metricValue;
            //     it->value(mLast) = metricValue;
            // }
        }

        bool GetMaxValue(TMetric &dst) const {
            return GetValue(mMax, dst);
        }

        bool GetMinValue(TMetric &dst) const {
            return GetValue(mMin, dst);
        }

        bool GetAvgValue(TMetric &dst) const {
            bool exist = GetValue(mTotal, dst);
            if (exist && mCount > 1) {
                enumerate([&](const FieldName<TMetric, TField> &field) {
                    field.value(dst) /= mCount;
                });
            }
            return exist;
        }

        // 统计，计算最大、最小、均值
        bool Stat(TMetric &max, TMetric &min, TMetric &avg, TMetric *last = nullptr) {
            return GetMaxValue(max) && GetMinValue(min) && GetAvgValue(avg) && (last == nullptr || GetLastValue(*last));
        }

        bool GetLastValue(TMetric &dst) const {
            return GetValue(mLast, dst);
        }

        std::shared_ptr<TMetric> GetLastValue() const {
            auto ret = std::make_shared<TMetric>();
            if (!GetValue(mLast, *ret)) {
                ret.reset();
            }
            return ret;
        }

        size_t Count() const {
            return mCount;
        }

    private:
        bool GetValue(const TMetric &src, TMetric &dst) const {
            bool exist = (mCount > 0);
            if (exist) {
                dst = src;
            }
            return exist;

        }

    private:
        TMetric mMax;
        TMetric mMin;
        TMetric mTotal;
        TMetric mLast;
        size_t mCount = 0;
    };
}
#endif //ARGUSAGENT_METRIC_CALCULATE_2_H
