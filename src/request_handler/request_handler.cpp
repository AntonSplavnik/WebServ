#include "request_handler.hpp"
#include "connection.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <vector>

// Comparison function for sorting: directories first, then files alphabetically

/*
Logic:
Lines 10-11: Check if each entry is a directory
S_ISDIR(a.second.st_mode) → true if a is a directory
S_ISDIR(b.second.st_mode) → true if b is a directory
Lines 12-14: If one is a directory and the other isn't:
aIsDir > bIsDir returns true when a is a directory and b is a file
This puts directories before files
Line 15: If both are the same type (both dirs or both files):
a.first < b.first sorts alphabetically by filename
Result: Directories first, then files, both sorted alphabetically.
*/
static bool compareEntries(const std::pair<std::string, struct stat>& a, const std::pair<std::string, struct stat>& b) {
	bool aIsDir = S_ISDIR(a.second.st_mode);
	bool bIsDir = S_ISDIR(b.second.st_mode);
	if (aIsDir != bIsDir) {
		return aIsDir > bIsDir; // Directories first
	}
	return a.first < b.first; // Alphabetical
}

RequestHandler::RequestHandler() {}
RequestHandler::~RequestHandler() {}

void RequestHandler::handleGET(Connection& connection, const std::string& indexFile, bool autoindex, std::string& path) {
	// Path existence and directory handling is done before calling this function
	// This function only handles regular files (not directories)
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		// Differentiate 404 vs 403 if needed
		connection.setStatusCode(errno == ENOENT ? 404 : 403);
		connection.setState(PREPARING_RESPONSE);
		return;
	}

	// Read content
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();
	file.close();

	// Check read errors
	if (content.empty() && errno) {
		connection.setStatusCode(500);
		connection.setState(PREPARING_RESPONSE);
		return;
	}

	// Success
	connection.setStatusCode(200);
	connection.setResponseData(content);
	connection.setState(PREPARING_RESPONSE);
}
void RequestHandler::handleDELETE(Connection& connection, std::string& path) {

	std::ifstream file(path.c_str());
	if (file.is_open()) {
		file.close();
		if(std::remove(path.c_str()) == 0) {
			connection.setStatusCode(204);
			std::cout << "[DEBUG] Succes: 204 file deleted" << std::endl;
		} else {
			connection.setStatusCode(403);
			std::cout << "[DEBUG] Error: 403 permission denied" << std::endl;
		}
	} else {
		connection.setStatusCode(404);
		std::cout << "[DEBUG] Error: 404 path is not found" << std::endl;
	}
	connection.setState(PREPARING_RESPONSE);
}

void RequestHandler::handlePOST(Connection& connection, std::string& path) {

	std::cout << "[DEBUG] UploadPath: " << path << std::endl;
	PostHandler post(path);

	const HttpRequest& request = connection.getRequest();

	std::string contentType = request.getContentType();
	std::cout << "[DEBUG] POST Content-Type: '" << contentType << "'" << std::endl;
	std::cout << "[DEBUG] Request valid: " << (request.getStatus() ? "true" : "false") << std::endl;


	if (contentType.find("multipart/form-data") != std::string::npos) {
		if(post.handleMultipart(connection)) {
			connection.setState(WRITING_DISK);
		} else {
			connection.setState(PREPARING_RESPONSE);
		}
	} else if (post.isSupportedContentType(contentType)) {
		post.handleFile(connection, contentType);
		connection.setState(WRITING_DISK);
	} else {
		std::cout << "[DEBUG] Unsupported Content-Type: " << contentType << std::endl;
		connection.setStatusCode(415);
		connection.setState(SENDING_RESPONSE);
	}
	// HttpResponse response(request);
	// response.generateResponse(statusCode);
	// client.responseData = response.getResponse();
	// client.bytesSent = 0;
	// client.state = SENDING_RESPONSE;
}

void RequestHandler::handleRedirect(Connection& connection, const LocationConfig* location) {
	const HttpRequest& req = connection.getRequest();
	HttpResponse response(req);
	
	response.setStatusCode(location->redirect_code);
	response.generateRedirectResponse(location->redirect);
	
	connection.setResponseData(response.getResponse());
	connection.setStatusCode(location->redirect_code);
	connection.setState(SENDING_RESPONSE);
	
	std::cout << "[DEBUG] Redirect " << location->redirect_code << " to: " << location->redirect << std::endl;
}

std::string RequestHandler::generateAutoindexHTML(const HttpRequest& request, const std::string& dirPath) {
	std::ostringstream html;
	
	// Get the request path for building links
	std::string requestPath = request.getPath();
	if (requestPath.empty() || requestPath[requestPath.length() - 1] != '/') {
		requestPath += '/';
	}
	
	// HTML header
	html << "<!DOCTYPE html>\n";
	html << "<html>\n";
	html << "<head>\n";
	html << "<title>Index of " << requestPath << "</title>\n";
	html << "<style>\n";
	html << "body { font-family: Arial, sans-serif; margin: 40px; }\n";
	html << "h1 { color: #333; }\n";
	html << "table { border-collapse: collapse; width: 100%; margin-top: 20px; }\n";
	html << "th, td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }\n";
	html << "th { background-color: #f2f2f2; font-weight: bold; }\n";
	html << "tr:hover { background-color: #f5f5f5; }\n";
	html << "a { text-decoration: none; color: #0066cc; }\n";
	html << "a:hover { text-decoration: underline; }\n";
	html << "</style>\n";
	html << "</head>\n";
	html << "<body>\n";
	html << "<h1>Index of " << requestPath << "</h1>\n";
	html << "<table>\n";
	html << "<thead>\n";
	html << "<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n";
	html << "</thead>\n";
	html << "<tbody>\n";
	
	// Add parent directory link
	if (requestPath != "/") {
		std::string parentPath = requestPath;
		if (parentPath.length() > 1) {
			parentPath = parentPath.substr(0, parentPath.length() - 1); // Example: "/images/photos/" → "/images/photos"
			//If a slash is found: take everything up to and including it
			//If no slash: set to "/"
			size_t lastSlash = parentPath.find_last_of('/');
			if (lastSlash != std::string::npos) {
				parentPath = parentPath.substr(0, lastSlash + 1);
			} else {
				parentPath = "/";
			}
		}
		html << "<tr><td><a href=\"" << parentPath << "\">../</a></td><td>-</td><td>-</td></tr>\n";
	}
	
	// Open directory and list contents
	DIR* dir = opendir(dirPath.c_str());
	if (dir != NULL) {
		struct dirent* entry;
		std::vector<std::pair<std::string, struct stat> > entries;
		
		// Read all entries
		while ((entry = readdir(dir)) != NULL) {
			std::string name = entry->d_name;
			// Skip . and ..
			if (name == "." || name == "..") {
				continue;
			}
			
			std::string fullPath = dirPath + name;
			struct stat entryStat;
			if (stat(fullPath.c_str(), &entryStat) == 0) {
				entries.push_back(std::make_pair(name, entryStat));
			}
		}
		closedir(dir);
		
		// Sort entries: directories first, then files, both alphabetically
		std::sort(entries.begin(), entries.end(), compareEntries);
		
		// Generate HTML for each entry
		for (size_t i = 0; i < entries.size(); ++i) {
			const std::string& name = entries[i].first;
			const struct stat& entryStat = entries[i].second;
			bool isDir = S_ISDIR(entryStat.st_mode);
			
			std::string linkName = name;
			if (isDir) {
				linkName += "/";
			}
			
			// Format last modified time 
			// Using st_mtime directly (seconds since epoch)
			std::ostringstream timeStream;
			timeStream << entryStat.st_mtime;
			std::string timeStr = timeStream.str();
			
			// Format size (simplified for C++98 compatibility)
			std::string sizeStr;
			if (isDir) {
				sizeStr = "-";
			} else {
				std::ostringstream sizeStream;
				if (entryStat.st_size < 1024) {
					sizeStream << entryStat.st_size << " B";
				} else if (entryStat.st_size < 1024 * 1024) {
					sizeStream << (entryStat.st_size / 1024) << " KB";
				} else {
					sizeStream << (entryStat.st_size / (1024 * 1024)) << " MB";
				}
				sizeStr = sizeStream.str();
			}
			
			html << "<tr><td><a href=\"" << requestPath << name;
			if (isDir) {
				html << "/";
			}
			html << "\">" << linkName << "</a></td>";
			html << "<td>" << timeStr << "</td>";
			html << "<td>" << sizeStr << "</td></tr>\n";
		}
	}
	
	html << "</tbody>\n";
	html << "</table>\n";
	html << "</body>\n";
	html << "</html>\n";
	
	// Return just the HTML body - Connection::prepareResponse() will build the full HTTP response
	return html.str();
}

