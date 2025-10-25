#include "latencyanalyzer.h"
#include <QDebug>

LatencyAnalyzer::LatencyAnalyzer(int maxSamples)
    : m_sumRtt(0), m_minRtt(std::numeric_limits<qint64>::max()), m_maxRtt(std::numeric_limits<qint64>::min()),
      m_totalSamples(0), m_maxSamples(maxSamples), m_previousRtt(0), m_jitter(0.0), m_hasPreviousRtt(false),
      m_sortedDataValid(false)
{
}

void LatencyAnalyzer::addSample(qint64 rtt)
{
    if (rtt < m_minRtt)
        m_minRtt = rtt;
    if (rtt > m_maxRtt)
        m_maxRtt = rtt;

    m_sumRtt += rtt;
    m_totalSamples++;

    if (m_hasPreviousRtt) {
        // RFC 3550标准Jitter计算: J = J + (|D(i-1,i)| - J) / 16
        qint64 diff = qAbs(rtt - m_previousRtt);
        m_jitter = m_jitter + (diff - m_jitter) / 16.0;
    } else {
        m_hasPreviousRtt = true;
    }
    m_previousRtt = rtt;

    m_rttHistory.append(rtt);

    if (m_rttHistory.size() > m_maxSamples) {
        qint64 removedRtt = m_rttHistory.takeFirst();

        m_sumRtt -= removedRtt;
        m_totalSamples--;

        // 优化Min/Max更新逻辑
        if (removedRtt == m_minRtt || removedRtt == m_maxRtt) {
            if (m_rttHistory.isEmpty()) {
                m_minRtt = std::numeric_limits<qint64>::max();
                m_maxRtt = std::numeric_limits<qint64>::min();
            } else {
                // 只有当移除的是极值时才重新计算
                if (removedRtt == m_minRtt) {
                    m_minRtt = *std::min_element(m_rttHistory.begin(), m_rttHistory.end());
                }
                if (removedRtt == m_maxRtt) {
                    m_maxRtt = *std::max_element(m_rttHistory.begin(), m_rttHistory.end());
                }
            }
        }
    }

    invalidateSortedCache();
}

LatencyAnalyzer::Statistics LatencyAnalyzer::getStatistics() const
{
    Statistics stats;

    if (m_totalSamples == 0) {
        stats.minRtt = 0;
        stats.maxRtt = 0;
        stats.avgRtt = 0.0;
        stats.jitter = 0.0;
        stats.percentile50 = 0.0;
        stats.percentile95 = 0.0;
        stats.percentile99 = 0.0;
        stats.sampleCount = 0;
        return stats;
    }

    stats.minRtt = m_minRtt;
    stats.maxRtt = m_maxRtt;
    stats.avgRtt = static_cast<double>(m_sumRtt) / m_totalSamples;
    stats.sampleCount = m_totalSamples;

    stats.jitter = m_jitter;

    if (!m_sortedDataValid) {
        m_cachedSortedData = m_rttHistory;
        std::sort(m_cachedSortedData.begin(), m_cachedSortedData.end());
        m_sortedDataValid = true;
    }

    stats.percentile50 = calculatePercentile(m_cachedSortedData, 50.0);
    stats.percentile95 = calculatePercentile(m_cachedSortedData, 95.0);
    stats.percentile99 = calculatePercentile(m_cachedSortedData, 99.0);

    return stats;
}

QString LatencyAnalyzer::getStatisticsString() const
{
    Statistics stats = getStatistics();

    return QString("RTT: min=%1μs, max=%2μs, avg=%3μs | "
                   "Jitter: %4μs | "
                   "Percentiles: P50=%5μs, P95=%6μs, P99=%7μs | "
                   "Samples: %8")
        .arg(stats.minRtt)
        .arg(stats.maxRtt)
        .arg(stats.avgRtt, 0, 'f', 2)
        .arg(stats.jitter, 0, 'f', 2)
        .arg(stats.percentile50, 0, 'f', 2)
        .arg(stats.percentile95, 0, 'f', 2)
        .arg(stats.percentile99, 0, 'f', 2)
        .arg(stats.sampleCount);
}

void LatencyAnalyzer::reset()
{
    m_rttHistory.clear();
    m_sumRtt = 0;
    m_minRtt = std::numeric_limits<qint64>::max();
    m_maxRtt = std::numeric_limits<qint64>::min();
    m_totalSamples = 0;
    m_previousRtt = 0;
    m_jitter = 0.0;
    m_hasPreviousRtt = false;
    invalidateSortedCache();
}

double LatencyAnalyzer::calculatePercentile(const QList<qint64>& sortedData, double percentile) const
{
    if (sortedData.isEmpty())
        return 0.0;

    double index = (percentile / 100.0) * sortedData.size();

    if (index <= 0) {
        return sortedData.first();
    }
    if (index >= sortedData.size()) {
        return sortedData.last();
    }

    int lowerIndex = static_cast<int>(index) - 1;
    int upperIndex = static_cast<int>(index);

    if (lowerIndex < 0) {
        return sortedData.first();
    }
    if (upperIndex >= sortedData.size()) {
        return sortedData.last();
    }

    double weight = index - lowerIndex - 1;
    return sortedData[lowerIndex] * (1.0 - weight) + sortedData[upperIndex] * weight;
}

void LatencyAnalyzer::invalidateSortedCache() const
{
    m_sortedDataValid = false;
}
