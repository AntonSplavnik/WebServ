/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   post_handler.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 00:00:00 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/28 21:35:35 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include "post_handler.hpp"
#include "request.hpp"
#include "connection.hpp"
#include "logger.hpp"


PostHandler::PostHandler(const std::string uploadPath)
    :_uploadPath(uploadPath){
    logDebug("PostHandler created with uploadPath: '" + _uploadPath + "'");
}

bool PostHandler::handleFile(Connection& connection, const std::string& contentType) {
    std::string extension = getExtensionFromContentType(contentType);

    std::string fileName = generateFilename(extension);
    std::string filePath = _uploadPath + fileName;
    connection.setFilePath(_uploadPath, fileName);
    logDebug("Saving file to: '" + filePath + "'");

    return true;
}
std::string PostHandler::generateFilename(const std::string& extension) {
    static int counter = 0;
    counter++;

    std::ostringstream fileName;
    fileName << "file_" << time(0) << "_" << counter;

    if (!extension.empty()) {
        fileName << "." << extension;
    }

    return fileName.str();
}
std::string PostHandler::getExtensionFromContentType(const std::string& contentType) {
    if (contentType.find("text/html") != std::string::npos) {
        return "html";
    }
    else if (contentType.find("text/css") != std::string::npos) {
        return "css";
    }
    else if (contentType.find("text/plain") != std::string::npos) {
        return "txt";
    }
    else if (contentType.find("image/jpeg") != std::string::npos) {
        return "jpg";
    }
    else if (contentType.find("image/png") != std::string::npos) {
        return "png";
    }
    else if (contentType.find("image/gif") != std::string::npos) {
        return "gif";
    }
    else if (contentType.find("image/webp") != std::string::npos) {
        return "webp";
    }
    else if (contentType.find("image/svg+xml") != std::string::npos) {
        return "svg";
    }
    else if (contentType.find("application/javascript") != std::string::npos) {
        return "js";
    }
    else if (contentType.find("application/pdf") != std::string::npos) {
        return "pdf";
    }
    else if (contentType.find("application/json") != std::string::npos) {
        return "json";
    }
    else if (contentType.find("application/octet-stream") != std::string::npos) {
        return "bin";
    }
    else {
        return "unknown";
    }
}

/* bool PostHandler::saveRawContent(const std::string& filePath, const std::string& content) {
    std::ofstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        std::cout << "[ERROR] Failed to open file for writing: " << filePath << std::endl;
        return false;
    }

    file.write(content.c_str(), content.size());
    file.close();

    if (file.good()) {
        std::cout << "[SUCCESS] File saved: " << filePath
                  << " (" << content.size() << " bytes)" << std::endl;
        return true;
    } else {
        std::cout << "[ERROR] Failed to write to file: " << filePath << std::endl;
        return false;
    }
} */

int PostHandler::handleMultipart(Connection& connection) {
    /*
        POST /upload HTTP/1.1
        Host: localhost:8080
        Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW
        Content-Length: 1234

        ------WebKitFormBoundary7MA4YWxkTrZu0gW
        Content-Disposition: form-data; name="username"

        john_doe
        ------WebKitFormBoundary7MA4YWxkTrZu0gW
        Content-Disposition: form-data; name="email"

        john@example.com
        ------WebKitFormBoundary7MA4YWxkTrZu0gW
        Content-Disposition: form-data; name="avatar"; filename="photo.jpg"
        Content-Type: image/jpeg

        [BINARY DATA OF IMAGE]
        ------WebKitFormBoundary7MA4YWxkTrZu0gW
        Content-Disposition: form-data; name="document"; filename="resume.pdf"
        Content-Type: application/pdf

        [BINARY DATA OF PDF]
        ------WebKitFormBoundary7MA4YWxkTrZu0gW--

    */
    std::string contentType = connection.getRequest().getContentType();
    std::string requestBody = connection.getRequest().getBody();
    std::string boundary = extractBoundary(contentType);

    std::vector<MultipartPart> parts = parseMultipartData(requestBody, boundary);

    // Sanitize filenames
    for (size_t i = 0; i < parts.size(); i++) {
        if (!parts[i].fileName.empty()) {
            std::string safe = sanitizeFilename(parts[i].fileName);
            if (safe.empty()) {
                return false;
            }
            parts[i].fileName = safe;
        }
    }
    connection.setMultipart(_uploadPath, parts);
    return true;
}
std::vector<MultipartPart> PostHandler::parseMultipartData(const std::string& body, const std::string& boundary) {
	std::vector<MultipartPart> parts;

	logDebug("parseMultipartData: body size=" + toString(body.size()) + ", boundary='" + boundary + "'");
	// Le boundary final a "--" à la fin
	std::string endBoundary = boundary + "--";

	size_t pos = 0;
	size_t found = 0;
	while ((found = body.find(boundary, pos)) != std::string::npos) {
	// Ignorer le premier boundary (début du multipart)
		if (found == pos) {
			pos = found + boundary.length();
			continue;
		}
		// Extraire le contenu entre deux boundaries
		std::string partData = body.substr(pos, found - pos);
        // Parser cette partie individuelle AVANT de vérifier la fin
        MultipartPart part = parseMultipartPart(partData);
        if (!part.name.empty()) {  // Validation basique
            parts.push_back(part);
        }
		// Vérifier si on atteint la fin (boundary--) APRÈS avoir parsé
		if (body.substr(found, endBoundary.length()) == endBoundary) {
            break;
        }
		// Avancer la position
		pos = found + boundary.length();
    }
    return parts;
}
MultipartPart PostHandler::parseMultipartPart(const std::string& partData) {
	MultipartPart part;

	// Séparer headers du contenu avec "\r\n\r\n"
	size_t headerEnd = partData.find("\r\n\r\n");
	if (headerEnd == std::string::npos) {
		return part; // Partie invalide
	}

	std::string headers = partData.substr(0, headerEnd);
	std::string content = partData.substr(headerEnd + 4);

	// Nettoyer le contenu (enlever les \r\n finaux)
	while (!content.empty() && (content[content.size() - 1] == '\r' || content[content.size() - 1] == '\n')) {
		content.erase(content.size() - 1);
	}

	// Parser les headers ligne par ligne
	std::istringstream headerStream(headers);
	std::string headerLine;

	while (std::getline(headerStream, headerLine)) {
	// Nettoyer \r en fin de ligne
		if (!headerLine.empty() && headerLine[headerLine.size() - 1] == '\r') {
			headerLine.erase(headerLine.size() - 1);
        }

		if (headerLine.find("Content-Disposition:") == 0) {
		// Extraire name et fileName
			parseContentDisposition(headerLine, part.name, part.fileName);
		}
		else if (headerLine.find("Content-Type:") == 0) {
            // Extraire content type
            part.contentType = extractContentTypeFromHeader(headerLine);
        }
    }
	part.content = content;
    return part;
}
void PostHandler::parseContentDisposition(const std::string& headerLine, std::string& name, std::string& fileName) {
    // Exemple: Content-Disposition: form-data; name="avatar"; filename="photo.jpg"

    size_t namePos = headerLine.find("name=\"");
    if (namePos != std::string::npos) {
        namePos += 6; // Longueur de "name=\""
        size_t nameEnd = headerLine.find("\"", namePos);
        if (nameEnd != std::string::npos) {
            name = headerLine.substr(namePos, nameEnd - namePos);
        }
    }

    // Chercher fileName="..." (optionnel)
    size_t filenamePos = headerLine.find("filename=\"");
    if (filenamePos != std::string::npos) {
        filenamePos += 10; // Longueur de "filename=\""
        size_t filenameEnd = headerLine.find("\"", filenamePos);
        if (filenameEnd != std::string::npos) {
            fileName = headerLine.substr(filenamePos, filenameEnd - filenamePos);
        }
    }
}
std::string PostHandler::extractContentTypeFromHeader(const std::string& headerLine) {
    // Exemple: Content-Type: image/jpeg
    size_t colonPos = headerLine.find(":");
    if (colonPos != std::string::npos) {
        std::string contentType = headerLine.substr(colonPos + 1);

        // Supprimer espaces de début
        size_t start = contentType.find_first_not_of(" \t");
        if (start != std::string::npos) {
            return contentType.substr(start);
        }
    }
    return "";
}

std::string PostHandler::extractBoundary(const std::string& contentType) {
	std::string boundary;
	size_t boundaryPos = contentType.find("boundary=");
	if (boundaryPos != std::string::npos) {
		boundary = "--" + contentType.substr(boundaryPos + 9);
	}
	return boundary;
}
std::string PostHandler::sanitizeFilename(const std::string& filename) {
    if (filename.empty()) {
        return "";  // Signal invalid
    }

    // Extract basename
    size_t lastSlash = filename.find_last_of("/\\");
    std::string basename = (lastSlash != std::string::npos)
        ? filename.substr(lastSlash + 1)
        : filename;

    // SECURITY: Detect path traversal attempt
    if (basename != filename) {
        logWarning("Path traversal attempt detected: " + filename);
        return "";  // Signal attack attempt
    }

    // SECURITY: Detect directory traversal patterns
    if (basename.find("..") != std::string::npos) {
        logWarning("Directory traversal pattern detected: " + filename);
        return "";  // Signal attack attempt
    }

    std::string safe;
    // Remove null bytes and control characters
    for (size_t i = 0; i < basename.size(); i++) {
        unsigned char c = static_cast<unsigned char>(basename[i]);
        if (c == 0 || c < 32) {
            logWarning("Control character detected in filename");
            return "";  // Signal attack attempt
        }
        safe += basename[i];
    }

    return safe.empty() ? "" : safe;
}

bool PostHandler::isSupportedContentType(const std::string& contentType) {
    return (contentType.find("text/plain") != std::string::npos ||
            contentType.find("text/css") != std::string::npos ||
            contentType.find("text/html") != std::string::npos ||
            contentType.find("image/jpeg") != std::string::npos ||
            contentType.find("image/png") != std::string::npos ||
            contentType.find("image/gif") != std::string::npos ||
            contentType.find("image/webp") != std::string::npos ||
            contentType.find("image/svg+xml") != std::string::npos ||
            contentType.find("application/javascript") != std::string::npos ||
            contentType.find("application/json") != std::string::npos ||
            contentType.find("application/pdf") != std::string::npos ||
            contentType.find("application/octet-stream") != std::string::npos);
}
