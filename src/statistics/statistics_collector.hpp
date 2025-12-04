/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   statistics_collector.hpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: drongier <drongier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/24 00:00:00 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/12/04 20:04:23 by drongier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef STATISTICS_COLLECTOR_HPP
#define STATISTICS_COLLECTOR_HPP

#include <string>
#include <map>
#include <deque>
#include <ctime>
#include <sys/types.h>
#include <vector>

struct RequestRecord {
	time_t timestamp;
	std::string method;
	int statusCode;
	size_t bytesReceived;
	size_t bytesSent;
	double durationMs;

	RequestRecord()
		: timestamp(0), method(""), statusCode(0),
		  bytesReceived(0), bytesSent(0), durationMs(0.0) {}

	RequestRecord(time_t ts, const std::string& m, int status,
	              size_t rx, size_t tx, double dur)
		: timestamp(ts), method(m), statusCode(status),
		  bytesReceived(rx), bytesSent(tx), durationMs(dur) {}
};

class StatisticsCollector {
public:
	StatisticsCollector();
	~StatisticsCollector();

	// Connection lifecycle tracking
	void recordConnectionStart(int fd);
	void recordConnectionEnd(int fd);

	// Request tracking
	void recordRequest(const std::string& method, int statusCode,
	                   size_t bytesReceived, size_t bytesSent, double durationMs);

	// State updates (called from event loop)
	void setActiveConnections(size_t count);
	void setActiveCgiProcesses(size_t count);
	void addListeningPort(int port);

	// Periodic maintenance (call every event loop iteration)
	void updatePeriodicStats();

	// Dashboard generation
	std::string generateHtmlDashboard() const;
	std::string generateJsonStats() const;

	// Direct access for special cases
	void incrementTimeouts();
	void incrementCgiTimeouts();

private:
	// Server start time
	time_t _startTime;

	// Current state
	size_t _activeConnections;
	size_t _activeCgiProcesses;
	std::vector<int> _listeningPorts;
	std::map<int, time_t> _connectionStartTimes; // fd -> start timestamp

	// Lifetime counters
	size_t _totalRequests;
	size_t _totalConnections;
	size_t _getRequests;
	size_t _postRequests;
	size_t _deleteRequests;
	size_t _cgiRequests;
	size_t _responses2xx;
	size_t _responses3xx;
	size_t _responses4xx;
	size_t _responses5xx;
	size_t _totalBytesReceived;
	size_t _totalBytesSent;
	size_t _connectionTimeouts;
	size_t _cgiTimeouts;

	// Time-windowed metrics (last 60 seconds)
	std::deque<RequestRecord> _recentRequests;
	time_t _lastWindowCleanup;

	// Aggregated statistics
	double _totalConnectionDuration;
	size_t _completedConnections;
	size_t _peakConcurrentConnections;

	// Helper methods
	void cleanupOldRequests();
	double calculateRequestsPerMinute() const;
	double calculateAvgConnectionDuration() const;
	double calculateAvgRequestDuration() const;
	size_t countRecentRequests() const;
	size_t countRecentByStatus(int statusClass) const; // 2xx, 4xx, etc.
	std::string formatUptime() const;
	std::string formatBytes(size_t bytes) const;
	std::string escapeHtml(const std::string& str) const;
};

#endif
