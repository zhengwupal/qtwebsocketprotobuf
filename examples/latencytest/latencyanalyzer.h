#ifndef LATENCY_ANALYZER_H
#define LATENCY_ANALYZER_H

#include <QDateTime>
#include <QList>
#include <QString>
#include <algorithm>
#include <chrono>
#include <cmath>

static inline qint64 getHighResolutionTimestamp()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

class LatencyAnalyzer
{
  public:
    struct Statistics
    {
        qint64 minRtt;
        qint64 maxRtt;
        double avgRtt;
        double jitter;        // 网络Jitter (RFC 3550)
        double percentile50;  // 中位数
        double percentile95;  // 95%分位数
        double percentile99;  // 99%分位数
        int sampleCount;
    };

    LatencyAnalyzer(int maxSamples = 1000);

    void addSample(qint64 rtt);
    Statistics getStatistics() const;
    QString getStatisticsString() const;
    void reset();

  private:
    QList<qint64> m_rttHistory;
    qint64 m_sumRtt;
    qint64 m_minRtt;
    qint64 m_maxRtt;
    int m_totalSamples;
    int m_maxSamples;

    // Jitter计算相关
    qint64 m_previousRtt;
    double m_jitter;
    bool m_hasPreviousRtt;

    // 缓存排序结果，避免重复排序
    mutable QList<qint64> m_cachedSortedData;
    mutable bool m_sortedDataValid;

    double calculatePercentile(const QList<qint64>& sortedData, double percentile) const;
    void invalidateSortedCache() const;
};

#endif  // LATENCY_ANALYZER_H
