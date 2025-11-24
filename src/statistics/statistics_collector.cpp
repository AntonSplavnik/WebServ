/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   statistics_collector.cpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/24 00:00:00 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/24 00:00:00 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "statistics_collector.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

StatisticsCollector::StatisticsCollector()
	: _startTime(time(NULL)),
	  _activeConnections(0),
	  _activeCgiProcesses(0),
	  _totalRequests(0),
	  _totalConnections(0),
	  _getRequests(0),
	  _postRequests(0),
	  _deleteRequests(0),
	  _cgiRequests(0),
	  _responses2xx(0),
	  _responses3xx(0),
	  _responses4xx(0),
	  _responses5xx(0),
	  _totalBytesReceived(0),
	  _totalBytesSent(0),
	  _connectionTimeouts(0),
	  _cgiTimeouts(0),
	  _lastWindowCleanup(time(NULL)),
	  _totalConnectionDuration(0.0),
	  _completedConnections(0),
	  _peakConcurrentConnections(0) {
}

StatisticsCollector::~StatisticsCollector() {
}

void StatisticsCollector::recordConnectionStart(int fd) {
	_connectionStartTimes[fd] = time(NULL);
	_totalConnections++;
}

void StatisticsCollector::recordConnectionEnd(int fd) {
	std::map<int, time_t>::iterator it = _connectionStartTimes.find(fd);
	if (it != _connectionStartTimes.end()) {
		time_t duration = time(NULL) - it->second;
		_totalConnectionDuration += static_cast<double>(duration);
		_completedConnections++;
		_connectionStartTimes.erase(it);
	}
}

void StatisticsCollector::recordRequest(const std::string& method, int statusCode,
                                         size_t bytesReceived, size_t bytesSent,
                                         double durationMs) {
	_totalRequests++;
	_totalBytesReceived += bytesReceived;
	_totalBytesSent += bytesSent;

	// Count by method
	if (method == "GET") {
		_getRequests++;
	} else if (method == "POST") {
		_postRequests++;
	} else if (method == "DELETE") {
		_deleteRequests++;
	}

	if (method.find("CGI") != std::string::npos) {
		_cgiRequests++;
	}

	// Count by status code
	if (statusCode >= 200 && statusCode < 300) {
		_responses2xx++;
	} else if (statusCode >= 300 && statusCode < 400) {
		_responses3xx++;
	} else if (statusCode >= 400 && statusCode < 500) {
		_responses4xx++;
	} else if (statusCode >= 500 && statusCode < 600) {
		_responses5xx++;
	}

	// Add to recent requests window
	RequestRecord record(time(NULL), method, statusCode,
	                     bytesReceived, bytesSent, durationMs);
	_recentRequests.push_back(record);
}

void StatisticsCollector::setActiveConnections(size_t count) {
	_activeConnections = count;
	if (count > _peakConcurrentConnections) {
		_peakConcurrentConnections = count;
	}
}

void StatisticsCollector::setActiveCgiProcesses(size_t count) {
	_activeCgiProcesses = count;
}

void StatisticsCollector::addListeningPort(int port) {
	// Check if port already exists
	for (size_t i = 0; i < _listeningPorts.size(); ++i) {
		if (_listeningPorts[i] == port) {
			return;
		}
	}
	_listeningPorts.push_back(port);
}

void StatisticsCollector::updatePeriodicStats() {
	time_t now = time(NULL);

	// Cleanup old requests from window every 10 seconds
	if (now - _lastWindowCleanup >= 10) {
		cleanupOldRequests();
		_lastWindowCleanup = now;
	}
}

void StatisticsCollector::incrementTimeouts() {
	_connectionTimeouts++;
}

void StatisticsCollector::incrementCgiTimeouts() {
	_cgiTimeouts++;
}

void StatisticsCollector::cleanupOldRequests() {
	time_t cutoff = time(NULL) - 60; // Keep last 60 seconds

	while (!_recentRequests.empty() && _recentRequests.front().timestamp < cutoff) {
		_recentRequests.pop_front();
	}
}

double StatisticsCollector::calculateRequestsPerMinute() const {
	return static_cast<double>(countRecentRequests());
}

double StatisticsCollector::calculateAvgConnectionDuration() const {
	if (_completedConnections == 0) {
		return 0.0;
	}
	return _totalConnectionDuration / static_cast<double>(_completedConnections);
}

double StatisticsCollector::calculateAvgRequestDuration() const {
	if (_recentRequests.empty()) {
		return 0.0;
	}

	double total = 0.0;
	for (std::deque<RequestRecord>::const_iterator it = _recentRequests.begin();
	     it != _recentRequests.end(); ++it) {
		total += it->durationMs;
	}
	return total / static_cast<double>(_recentRequests.size());
}

size_t StatisticsCollector::countRecentRequests() const {
	return _recentRequests.size();
}

size_t StatisticsCollector::countRecentByStatus(int statusClass) const {
	size_t count = 0;
	int lower = statusClass;
	int upper = statusClass + 100;

	for (std::deque<RequestRecord>::const_iterator it = _recentRequests.begin();
	     it != _recentRequests.end(); ++it) {
		if (it->statusCode >= lower && it->statusCode < upper) {
			count++;
		}
	}
	return count;
}

std::string StatisticsCollector::formatUptime() const {
	time_t uptime = time(NULL) - _startTime;

	size_t days = uptime / 86400;
	size_t hours = (uptime % 86400) / 3600;
	size_t minutes = (uptime % 3600) / 60;
	size_t seconds = uptime % 60;

	std::ostringstream oss;
	if (days > 0) {
		oss << days << "d ";
	}
	oss << std::setfill('0') << std::setw(2) << hours << ":"
	    << std::setw(2) << minutes << ":"
	    << std::setw(2) << seconds;

	return oss.str();
}

std::string StatisticsCollector::formatBytes(size_t bytes) const {
	const char* units[] = {"B", "KB", "MB", "GB", "TB"};
	int unitIndex = 0;
	double value = static_cast<double>(bytes);

	while (value >= 1024.0 && unitIndex < 4) {
		value /= 1024.0;
		unitIndex++;
	}

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2) << value << " " << units[unitIndex];
	return oss.str();
}

std::string StatisticsCollector::escapeHtml(const std::string& str) const {
	std::string result;
	for (size_t i = 0; i < str.length(); ++i) {
		switch (str[i]) {
			case '<': result += "&lt;"; break;
			case '>': result += "&gt;"; break;
			case '&': result += "&amp;"; break;
			case '"': result += "&quot;"; break;
			case '\'': result += "&#39;"; break;
			default: result += str[i];
		}
	}
	return result;
}

std::string StatisticsCollector::generateHtmlDashboard() const {
	std::ostringstream html;

	html << "<!DOCTYPE html>\n"
	     << "<html>\n"
	     << "<head>\n"
	     << "  <meta charset=\"UTF-8\">\n"
	     << "  <title>WebServ Statistics</title>\n"
	     << "  <meta http-equiv=\"refresh\" content=\"5\">\n"
	     << "  <style>\n"
	     << "    * { margin: 0; padding: 0; box-sizing: border-box; }\n"
	     << "    body {\n"
	     << "      font-family: 'Courier New', monospace;\n"
	     << "      background: #1a1a1a;\n"
	     << "      color: #00ff00;\n"
	     << "      padding: 20px;\n"
	     << "    }\n"
	     << "    .container { max-width: 1200px; margin: 0 auto; }\n"
	     << "    h1 {\n"
	     << "      text-align: center;\n"
	     << "      margin-bottom: 10px;\n"
	     << "      color: #00ff00;\n"
	     << "      font-size: 2em;\n"
	     << "    }\n"
	     << "    .update-info {\n"
	     << "      text-align: center;\n"
	     << "      color: #888;\n"
	     << "      margin-bottom: 30px;\n"
	     << "      font-size: 0.9em;\n"
	     << "    }\n"
	     << "    .grid {\n"
	     << "      display: grid;\n"
	     << "      grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));\n"
	     << "      gap: 20px;\n"
	     << "      margin-bottom: 20px;\n"
	     << "    }\n"
	     << "    .card {\n"
	     << "      background: #2a2a2a;\n"
	     << "      border: 2px solid #00ff00;\n"
	     << "      border-radius: 8px;\n"
	     << "      padding: 20px;\n"
	     << "    }\n"
	     << "    .card h2 {\n"
	     << "      color: #00ff00;\n"
	     << "      margin-bottom: 15px;\n"
	     << "      font-size: 1.3em;\n"
	     << "      border-bottom: 1px solid #00ff00;\n"
	     << "      padding-bottom: 5px;\n"
	     << "    }\n"
	     << "    table {\n"
	     << "      width: 100%;\n"
	     << "      border-collapse: collapse;\n"
	     << "    }\n"
	     << "    td {\n"
	     << "      padding: 8px 4px;\n"
	     << "      border-bottom: 1px solid #333;\n"
	     << "    }\n"
	     << "    td:first-child {\n"
	     << "      color: #888;\n"
	     << "      width: 60%;\n"
	     << "    }\n"
	     << "    td:last-child {\n"
	     << "      text-align: right;\n"
	     << "      color: #00ff00;\n"
	     << "      font-weight: bold;\n"
	     << "    }\n"
	     << "    .metric-big {\n"
	     << "      font-size: 2.5em;\n"
	     << "      text-align: center;\n"
	     << "      color: #00ff00;\n"
	     << "      margin: 10px 0;\n"
	     << "    }\n"
	     << "    .ports {\n"
	     << "      display: flex;\n"
	     << "      flex-wrap: wrap;\n"
	     << "      gap: 10px;\n"
	     << "      margin-top: 10px;\n"
	     << "    }\n"
	     << "    .port-badge {\n"
	     << "      background: #00ff00;\n"
	     << "      color: #1a1a1a;\n"
	     << "      padding: 5px 15px;\n"
	     << "      border-radius: 4px;\n"
	     << "      font-weight: bold;\n"
	     << "    }\n"
	     << "  </style>\n"
	     << "</head>\n"
	     << "<body>\n"
	     << "  <div class=\"container\">\n"
	     << "    <h1>⚡ WebServ Statistics Dashboard ⚡</h1>\n"
	     << "    <div class=\"update-info\">Auto-refresh: 5s | Uptime: "
	     << formatUptime() << "</div>\n\n";

	// Real-time status
	html << "    <div class=\"grid\">\n"
	     << "      <div class=\"card\">\n"
	     << "        <h2>Real-Time Status</h2>\n"
	     << "        <div class=\"metric-big\">" << _activeConnections << "</div>\n"
	     << "        <div style=\"text-align: center; color: #888; margin-bottom: 15px;\">Active Connections</div>\n"
	     << "        <table>\n"
	     << "          <tr><td>Active CGI Processes</td><td>" << _activeCgiProcesses << "</td></tr>\n"
	     << "          <tr><td>Peak Concurrent Connections</td><td>" << _peakConcurrentConnections << "</td></tr>\n"
	     << "        </table>\n"
	     << "      </div>\n\n";

	// Listening ports
	html << "      <div class=\"card\">\n"
	     << "        <h2>Listening Ports</h2>\n"
	     << "        <div class=\"ports\">\n";
	for (size_t i = 0; i < _listeningPorts.size(); ++i) {
		html << "          <div class=\"port-badge\">:" << _listeningPorts[i] << "</div>\n";
	}
	if (_listeningPorts.empty()) {
		html << "          <div style=\"color: #888;\">No ports configured</div>\n";
	}
	html << "        </div>\n"
	     << "      </div>\n\n";

	// Last minute metrics
	html << "      <div class=\"card\">\n"
	     << "        <h2>Last Minute</h2>\n"
	     << "        <div class=\"metric-big\">" << std::fixed << std::setprecision(1)
	     << calculateRequestsPerMinute() << "</div>\n"
	     << "        <div style=\"text-align: center; color: #888; margin-bottom: 15px;\">Requests/min</div>\n"
	     << "        <table>\n"
	     << "          <tr><td>2xx Responses</td><td>" << countRecentByStatus(200) << "</td></tr>\n"
	     << "          <tr><td>4xx Errors</td><td>" << countRecentByStatus(400) << "</td></tr>\n"
	     << "          <tr><td>5xx Errors</td><td>" << countRecentByStatus(500) << "</td></tr>\n"
	     << "          <tr><td>Avg Request Duration</td><td>" << std::fixed << std::setprecision(2)
	     << calculateAvgRequestDuration() << " ms</td></tr>\n"
	     << "        </table>\n"
	     << "      </div>\n"
	     << "    </div>\n\n";

	// Lifetime statistics
	html << "    <div class=\"grid\">\n"
	     << "      <div class=\"card\">\n"
	     << "        <h2>Lifetime Requests</h2>\n"
	     << "        <table>\n"
	     << "          <tr><td>Total Requests</td><td>" << _totalRequests << "</td></tr>\n"
	     << "          <tr><td>GET Requests</td><td>" << _getRequests << "</td></tr>\n"
	     << "          <tr><td>POST Requests</td><td>" << _postRequests << "</td></tr>\n"
	     << "          <tr><td>DELETE Requests</td><td>" << _deleteRequests << "</td></tr>\n"
	     << "          <tr><td>CGI Requests</td><td>" << _cgiRequests << "</td></tr>\n"
	     << "        </table>\n"
	     << "      </div>\n\n";

	html << "      <div class=\"card\">\n"
	     << "        <h2>Response Status Codes</h2>\n"
	     << "        <table>\n"
	     << "          <tr><td>2xx Success</td><td>" << _responses2xx << "</td></tr>\n"
	     << "          <tr><td>3xx Redirects</td><td>" << _responses3xx << "</td></tr>\n"
	     << "          <tr><td>4xx Client Errors</td><td>" << _responses4xx << "</td></tr>\n"
	     << "          <tr><td>5xx Server Errors</td><td>" << _responses5xx << "</td></tr>\n"
	     << "        </table>\n"
	     << "      </div>\n\n";

	html << "      <div class=\"card\">\n"
	     << "        <h2>Traffic & Performance</h2>\n"
	     << "        <table>\n"
	     << "          <tr><td>Total Connections</td><td>" << _totalConnections << "</td></tr>\n"
	     << "          <tr><td>Bytes Received</td><td>" << formatBytes(_totalBytesReceived) << "</td></tr>\n"
	     << "          <tr><td>Bytes Sent</td><td>" << formatBytes(_totalBytesSent) << "</td></tr>\n"
	     << "          <tr><td>Avg Connection Duration</td><td>" << std::fixed << std::setprecision(2)
	     << calculateAvgConnectionDuration() << " s</td></tr>\n"
	     << "          <tr><td>Connection Timeouts</td><td>" << _connectionTimeouts << "</td></tr>\n"
	     << "          <tr><td>CGI Timeouts</td><td>" << _cgiTimeouts << "</td></tr>\n"
	     << "        </table>\n"
	     << "      </div>\n"
	     << "    </div>\n\n";

	html << "  </div>\n"
	     << "</body>\n"
	     << "</html>\n";

	return html.str();
}

std::string StatisticsCollector::generateJsonStats() const {
	std::ostringstream json;

	json << "{\n"
	     << "  \"server\": {\n"
	     << "    \"uptime\": " << (time(NULL) - _startTime) << ",\n"
	     << "    \"uptimeFormatted\": \"" << escapeHtml(formatUptime()) << "\"\n"
	     << "  },\n"
	     << "  \"realtime\": {\n"
	     << "    \"activeConnections\": " << _activeConnections << ",\n"
	     << "    \"activeCgiProcesses\": " << _activeCgiProcesses << ",\n"
	     << "    \"peakConcurrentConnections\": " << _peakConcurrentConnections << "\n"
	     << "  },\n"
	     << "  \"lastMinute\": {\n"
	     << "    \"requests\": " << countRecentRequests() << ",\n"
	     << "    \"requestsPerMinute\": " << std::fixed << std::setprecision(2)
	     << calculateRequestsPerMinute() << ",\n"
	     << "    \"responses2xx\": " << countRecentByStatus(200) << ",\n"
	     << "    \"responses4xx\": " << countRecentByStatus(400) << ",\n"
	     << "    \"responses5xx\": " << countRecentByStatus(500) << ",\n"
	     << "    \"avgRequestDurationMs\": " << std::fixed << std::setprecision(2)
	     << calculateAvgRequestDuration() << "\n"
	     << "  },\n"
	     << "  \"lifetime\": {\n"
	     << "    \"totalRequests\": " << _totalRequests << ",\n"
	     << "    \"totalConnections\": " << _totalConnections << ",\n"
	     << "    \"getRequests\": " << _getRequests << ",\n"
	     << "    \"postRequests\": " << _postRequests << ",\n"
	     << "    \"deleteRequests\": " << _deleteRequests << ",\n"
	     << "    \"cgiRequests\": " << _cgiRequests << ",\n"
	     << "    \"responses2xx\": " << _responses2xx << ",\n"
	     << "    \"responses3xx\": " << _responses3xx << ",\n"
	     << "    \"responses4xx\": " << _responses4xx << ",\n"
	     << "    \"responses5xx\": " << _responses5xx << ",\n"
	     << "    \"totalBytesReceived\": " << _totalBytesReceived << ",\n"
	     << "    \"totalBytesSent\": " << _totalBytesSent << ",\n"
	     << "    \"connectionTimeouts\": " << _connectionTimeouts << ",\n"
	     << "    \"cgiTimeouts\": " << _cgiTimeouts << ",\n"
	     << "    \"avgConnectionDuration\": " << std::fixed << std::setprecision(2)
	     << calculateAvgConnectionDuration() << "\n"
	     << "  }\n"
	     << "}\n";

	return json.str();
}
