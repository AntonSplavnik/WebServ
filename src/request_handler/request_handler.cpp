#include "request_handler.hpp"
#include "connection.hpp"

void RequestHandler::handleGET(Connection& connection) {
	std::string path = connection.getRoutingResult().mappedPath;

	if (isDirectory(path)) {
		handleDirectory(connection, path);
	} else {
		serveFile(connection, path);
	}
}

void RequestHandler::handleDirectory(Connection& connection, const std::string& dirPath) {
	std::string path = dirPath;
	const RoutingResult& result = connection.getRoutingResult();

	// Ensure trailing slash for directory paths
	if (path[path.length() - 1] != '/') {
		path += "/";
	}

	// Try to find index file
	std::string indexFile;
	if (result.location && !result.location->index.empty()) {
		indexFile = result.location->index;
	} else if (result.serverConfig && !result.serverConfig->index.empty()) {
		indexFile = result.serverConfig->index;
	}

	// Check if index file exists
	if (!indexFile.empty()) {
		std::string indexPath = path + indexFile;
		std::ifstream indexTest(indexPath.c_str());
		if (indexTest.is_open()) {
			indexTest.close();
			connection.setIndexPath(indexPath);
			serveFile(connection, indexPath);
			return;
		} else {
			// Index file configured but not found - check autoindex
			bool autoindex = false;
			if (result.location) {
				autoindex = result.location->autoindex;
			} else if (result.serverConfig) {
				autoindex = result.serverConfig->autoindex;
			}
			handleAutoindex(connection, path, autoindex);
			return;
		}
	} else {
		// No index file configured - check autoindex directly
		bool autoindex = false;
		if (result.location) {
			autoindex = result.location->autoindex;
		} else if (result.serverConfig) {
			autoindex = result.serverConfig->autoindex;
		}
		handleAutoindex(connection, path, autoindex);
	}
}
void RequestHandler::serveFile(Connection& connection, const std::string& filePath) {
	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file.is_open()) {
		// Differentiate 404 vs 403 if needed
		connection.setStatusCode(errno == ENOENT ? 404 : 403);
		connection.prepareResponse();
		return;
	}

	// Read content
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();

	// Check read errors (not EOF, which is normal)
	if (file.fail() && !file.eof()) {
		file.close();
		connection.setStatusCode(500);
		connection.prepareResponse();
		return;
	}

	file.close();

	// Success (even if content is empty - valid for 0-byte files)
	connection.setStatusCode(200);
	connection.setBodyContent(content);
	connection.prepareResponse();
}
void RequestHandler::handleDELETE(Connection& connection) {

	const std::string& path = connection.getRoutingResult().mappedPath;
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
	connection.prepareResponse();
}
void RequestHandler::handlePOST(Connection& connection) {

	const std::string& path = connection.getRoutingResult().mappedPath;
	std::cout << "[DEBUG] UploadPath: " << path << std::endl;
	PostHandler post(path);

	const HttpRequest& request = connection.getRequest();

	std::string contentType = request.getContentType();
	std::cout << "[DEBUG] POST Content-Type: '" << contentType << "'" << std::endl;
	std::cout << "[DEBUG] Request valid: " << (request.getStatus() ? "true" : "false") << std::endl;

	// Empty POST - no content to process
	if (request.getBody().empty()) {
		connection.setStatusCode(204);
		connection.prepareResponse();
		return;
	}

	// Has body - validate Content-Type
	if (contentType.find("multipart/form-data") != std::string::npos) {
		if(post.handleMultipart(connection)) {
			connection.setState(WRITING_DISK);
		} else {
			connection.setStatusCode(400);
			connection.prepareResponse();
		}
	} else if (post.isSupportedContentType(contentType)) {
		post.handleFile(connection, contentType);
		connection.setState(WRITING_DISK);
	} else {
		std::cout << "[DEBUG] Unsupported Content-Type: " << contentType << std::endl;
		connection.setStatusCode(415);
		connection.prepareResponse();;
	}

}
void RequestHandler::handleRedirect(Connection& connection) {

	const LocationConfig* location = connection.getRoutingResult().location;
	connection.setStatusCode(location->redirect_code);
	connection.setRedirectUrl(location->redirect);
	connection.prepareResponse();
}


// Helper struct for sorting directory entries
struct DirEntry {
	std::string name;
	bool isDir;
	off_t size;
	time_t mtime;

	DirEntry(const std::string& n, bool d, off_t s, time_t t)
		: name(n), isDir(d), size(s), mtime(t) {}
};

// Autoindex helper functions
bool RequestHandler::isDirectory(const std::string& path) {
	struct stat statbuf;
	if (stat(path.c_str(), &statbuf) != 0) {
		return false;
	}
	return S_ISDIR(statbuf.st_mode);
}
std::string RequestHandler::formatFileSize(off_t bytes) {
	const char* units[] = {"B", "KB", "MB", "GB"};
	int unitIndex = 0;
	double size = static_cast<double>(bytes);

	while (size >= 1024.0 && unitIndex < 3) {
		size /= 1024.0;
		unitIndex++;
	}

	std::stringstream ss;
	if (unitIndex == 0) {
		ss << static_cast<long>(size) << " " << units[unitIndex];
	} else {
		ss.precision(1);
		ss << std::fixed << size << " " << units[unitIndex];
	}
	return ss.str();
}
std::string RequestHandler::formatDateTime(time_t timestamp) {
	char buffer[64];
	struct tm* timeinfo = localtime(&timestamp);

	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
	return std::string(buffer);
}
// Comparison function for sorting: directories first, then alphabetical
static bool compareDirEntries(const DirEntry& a, const DirEntry& b) {
	if (a.isDir != b.isDir)
		return a.isDir;
	return a.name < b.name;
}
std::string RequestHandler::generateDirectoryListing(const std::string& dirPath, const std::string& requestPath) {
	DIR* dir = opendir(dirPath.c_str());
	if (!dir) {
		return "";
	}

	std::vector<DirEntry> entries;
	struct dirent* entry;

	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;

		// Skip hidden files and current directory
		if (name[0] == '.' && name != "..") {
			continue;
		}

		std::string fullPath = dirPath + "/" + name;
		struct stat statbuf;
		if (stat(fullPath.c_str(), &statbuf) != 0) {
			continue;
		}

		bool isDir = S_ISDIR(statbuf.st_mode);
		entries.push_back(DirEntry(name, isDir, statbuf.st_size, statbuf.st_mtime));
	}
	closedir(dir);

	// Sort entries
	std::sort(entries.begin(), entries.end(), compareDirEntries);

	// Generate HTML
	std::stringstream html;
	html << "<!DOCTYPE html>\n"
		 << "<html>\n<head>\n"
		 << "<title>Index of " << requestPath << "</title>\n"
		 << "<style>\n"
		 << "body { font-family: monospace; margin: 20px; }\n"
		 << "h1 { border-bottom: 1px solid #ccc; }\n"
		 << "table { border-collapse: collapse; width: 100%; }\n"
		 << "th { text-align: left; padding: 8px; background: #f0f0f0; }\n"
		 << "td { padding: 8px; border-bottom: 1px solid #eee; }\n"
		 << "a { text-decoration: none; color: #0366d6; }\n"
		 << "a:hover { text-decoration: underline; }\n"
		 << "</style>\n"
		 << "</head>\n<body>\n"
		 << "<h1>Index of " << requestPath << "</h1>\n"
		 << "<table>\n"
		 << "<tr><th>Name</th><th>Size</th><th>Modified</th></tr>\n";

	for (size_t i = 0; i < entries.size(); ++i) {
		const DirEntry& e = entries[i];
		std::string href = requestPath;
		if (href[href.length() - 1] != '/') {
			href += "/";
		}
		href += e.name;
		if (e.isDir && e.name != "..") {
			href += "/";
		}

		html << "<tr>"
			 << "<td><a href=\"" << href << "\">" << e.name;
		if (e.isDir) {
			html << "/";
		}
		html << "</a></td>"
			 << "<td>" << (e.isDir ? "-" : formatFileSize(e.size)) << "</td>"
			 << "<td>" << formatDateTime(e.mtime) << "</td>"
			 << "</tr>\n";
	}

	html << "</table>\n</body>\n</html>";
	return html.str();
}
void RequestHandler::handleAutoindex(Connection& connection, const std::string& dirPath, bool autoindex) {
	if (autoindex) {
		std::string requestPath = connection.getRequest().getPath();
		std::string html = generateDirectoryListing(dirPath, requestPath);
		if (html.empty()) {
			connection.setStatusCode(500);
			connection.prepareResponse();
			return;
		}
		connection.setStatusCode(200);
		connection.setBodyContent(html);
		connection.setIndexPath("autoindex.html");
		connection.prepareResponse();
	} else {
		connection.setStatusCode(403);
		connection.prepareResponse();
	}
}
