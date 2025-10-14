/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   post_handler.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 00:00:00 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/14 15:31:51 by antonsplavn      ###   ########.fr       */
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
        PostHandler(const std::string uploadPath);

        // Main POST handling methods
        void handlePOST(const HttpRequest& request, ClientInfo& client);
        void handleMultipart(const HttpRequest& request, ClientInfo& client);

        // Multipart parsing methods
        std::vector<MultipartPart> parseMultipartData(const std::string& body, const std::string& boundary);
        MultipartPart parseMultipartPart(const std::string& partData);
        void parseContentDisposition(const std::string& headerLine, std::string& name, std::string& filename);
        std::string extractContentTypeFromHeader(const std::string& headerLine);

        // Processing and saving methods
        void processMultipartParts(const std::vector<MultipartPart>& parts, const HttpRequest& request, ClientInfo& client);
        bool saveFileFromMultipart(const std::string& filePath, const std::string& content);
        void saveFormFieldToLog(const std::string& fieldName, const std::string& fieldValue);

        // Utility methods
        std::string extractBoundary(const std::string& contentType);
        bool isSupportedContentType(const std::string& contentType);
        void handleFile(const HttpRequest& request, ClientInfo& client, const std::string& contentType);
        std::string getExtensionFromContentType(const std::string& contentType);
        std::string generateFilename(const std::string& extension);
        bool saveRawContent(const std::string& filePath, const std::string& content);

    private:
        std::string _uploadPath;
};

#endif

