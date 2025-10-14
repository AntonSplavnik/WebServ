/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   post_handler.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 00:00:00 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/09 00:00:00 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef POST_HANDLER_HPP
#define POST_HANDLER_HPP

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

// Forward declarations
class HttpRequest;
class HttpResponse;
struct ClientInfo;

struct MultipartPart {
    std::string name;
    std::string filename;
    std::string contentType;
    std::string content;
};

class PostHandler {
public:
    // Main POST handling methods
    static void handlePOST(const HttpRequest& request, ClientInfo& client);
    static void handleMultipart(const HttpRequest& request, ClientInfo& client);

    // Multipart parsing methods
    static std::vector<MultipartPart> parseMultipartData(const std::string& body, const std::string& boundary);
    static MultipartPart parseMultipartPart(const std::string& partData);
    static void parseContentDisposition(const std::string& headerLine, std::string& name, std::string& filename);
    static std::string extractContentTypeFromHeader(const std::string& headerLine);

    // Processing and saving methods
    static void processMultipartParts(const std::vector<MultipartPart>& parts, const HttpRequest& request, ClientInfo& client);
    static bool saveFileFromMultipart(const std::string& filePath, const std::string& content);
    static void saveFormFieldToLog(const std::string& fieldName, const std::string& fieldValue);

    // Utility methods
    static std::string extractBoundary(const std::string& contentType);
    static bool isSupportedContentType(const std::string& contentType);
    static void handleFile(const HttpRequest& request, ClientInfo& client, const std::string& contentType);
    static std::string getExtensionFromContentType(const std::string& contentType);
    static std::string generateFilename(const std::string& extension);
    static bool saveRawContent(const std::string& filePath, const std::string& content);
};

#endif

