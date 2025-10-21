/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   post_handler.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 00:00:00 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/17 20:49:32 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "post_handler.hpp"
#include "server.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

PostHandler::PostHandler(const std::string uploadPath, const ConfigData& configData)
    :_configData(configData), _uploadPath(uploadPath) {
    std::cout << "[DEBUG] PostHandler created with uploadPath: '" << _uploadPath << "'" << std::endl;
}

bool PostHandler::saveRawContent(const std::string& filePath, const std::string& content) {
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
}

void PostHandler::handleFile(const HttpRequest& request, ClientInfo& client, const std::string& contentType) {
    std::string extension = getExtensionFromContentType(contentType);

    std::string filename = generateFilename(extension);
    std::string filePath = _uploadPath + filename;
    std::cout << "[DEBUG] Saving file to: '" << filePath << "'" << std::endl;

    if (saveRawContent(filePath, request.getBody())) {
        HttpResponse response(request, _configData);
        response.generateResponse(200);
        client.responseData = response.getResponse();
    } else {
        HttpResponse response(request, _configData);
        response.generateResponse(500);
        client.responseData = response.getResponse();
    }
}

std::string PostHandler::generateFilename(const std::string& extension) {
    static int counter = 0;
    counter++;

    std::ostringstream filename;
    filename << "file_" << time(0) << "_" << counter;

    if (!extension.empty()) {
        filename << "." << extension;
    }

    return filename.str();
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

bool PostHandler::isSupportedContentType(const std::string& contentType)
{
		return (contentType.find("text/plain") != std::string::npos ||
                contentType.find("text/css") != std::string::npos ||
				contentType.find("image/jpeg") != std::string::npos ||
				contentType.find("image/png") != std::string::npos ||
				contentType.find("image/gif") != std::string::npos ||
				contentType.find("application/javascript") != std::string::npos ||
				contentType.find("application/json") != std::string::npos ||
				contentType.find("application/octet-stream") != std::string::npos);
}

void PostHandler::handleMultipart(const HttpRequest& request, ClientInfo& client) {

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

	std::string contentType = request.getContenType();
	std::string requestBody = request.getBody();

	std::string boundary = extractBoundary(contentType);
	std::vector<MultipartPart> parts = parseMultipartData(requestBody, boundary);
	processMultipartParts(parts, request, client);

	HttpResponse response(request, _configData);
	response.generateResponse(200);
	client.responseData = response.getResponse();
}

std::vector<MultipartPart> PostHandler::parseMultipartData(const std::string& body, const std::string& boundary) {
	std::vector<MultipartPart> parts;

	std::cout << "[DEBUG] parseMultipartData: body size=" << body.size()
	          << ", boundary='" << boundary << "'" << std::endl;
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
		// Extraire name et filename
			parseContentDisposition(headerLine, part.name, part.filename);
		}
		else if (headerLine.find("Content-Type:") == 0) {
            // Extraire content type
            part.contentType = extractContentTypeFromHeader(headerLine);
        }
    }
	part.content = content;
    return part;
}

void PostHandler::parseContentDisposition(const std::string& headerLine, std::string& name, std::string& filename) {
	// Exemple: Content-Disposition: form-data; name="avatar"; filename="photo.jpg"

	size_t namePos = headerLine.find("name=\"");
	if (namePos != std::string::npos) {
		namePos += 6; // Longueur de "name=\""
		size_t nameEnd = headerLine.find("\"", namePos);
		if (nameEnd != std::string::npos) {
			name = headerLine.substr(namePos, nameEnd - namePos);
		}
	}

	// Chercher filename="..." (optionnel)
	size_t filenamePos = headerLine.find("filename=\"");
	if (filenamePos != std::string::npos) {
		filenamePos += 10; // Longueur de "filename=\""
		size_t filenameEnd = headerLine.find("\"", filenamePos);
		if (filenameEnd != std::string::npos) {
			filename = headerLine.substr(filenamePos, filenameEnd - filenamePos);
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

void PostHandler::processMultipartParts(const std::vector<MultipartPart>& parts, const HttpRequest& request, ClientInfo& client) {

    std::string uploadDir = _uploadPath;
    std::cout << "[DEBUG] Upload directory: '" << uploadDir << "'" << std::endl;

    // Create folder if doesnt existe
    system(("mkdir -p " + uploadDir).c_str());

    // Save every part
    for (size_t i = 0; i < parts.size(); i++) {
        const MultipartPart& part = parts[i];

        std::cout << "[DEBUG] Part " << i << ": name='" << part.name
                  << "', filename='" << part.filename
                  << "', contentType='" << part.contentType
                  << "', content size=" << part.content.size() << " bytes" << std::endl;

        if (!part.filename.empty()) {
            std::string filePath = uploadDir + part.filename;

            if (saveFileFromMultipart(filePath, part.content)) {
                std::cout << "File saved: " << part.filename
                         << " (" << part.content.size() << " bytes)" << std::endl;
            } else {
                std::cout << "ERROR: Failed to save file: " << part.filename << std::endl;
            }
        } else {
            std::cout << "Form field: " << part.name << " = " << part.content << std::endl;
            saveFormFieldToLog(part.name, part.content);
        }
    }
    HttpResponse response(request, _configData);
    response.generateResponse(200);
    client.responseData = response.getResponse();
}

bool PostHandler::saveFileFromMultipart(const std::string& filePath, const std::string& content) {
    std::ofstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    file.write(content.c_str(), content.size());
    file.close();

    return file.good();
}

void PostHandler::saveFormFieldToLog(const std::string& fieldName, const std::string& fieldValue) {
    std::string logFile = _uploadPath + "form_data.log";

    std::ofstream file(logFile.c_str(), std::ios::app);
    if (file.is_open()) {
        file << "Field: " << fieldName << " = " << fieldValue << std::endl;
        file.close();
    }
}

std::string PostHandler::extractBoundary(const std::string& contentType) {
	std::string boundary;
	size_t boundaryPos = contentType.find("boundary=");
	if (boundaryPos != std::string::npos) {
		boundary = "--" + contentType.substr(boundaryPos + 9);
	}
	return boundary;
}
