/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   post_handler.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 00:00:00 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/19 15:19:19 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef POST_HANDLER_HPP
#define POST_HANDLER_HPP

#include <string>
#include <vector>


// Forward declarations
class HttpRequest;
class Connection;

struct MultipartPart {
    std::string name;
    std::string fileName;
    std::string contentType;
    std::string content;
};

class PostHandler {

    public:
        PostHandler(const std::string uploadPath);

        int handleMultipart(Connection& connection);
        bool handleFile(Connection& connection, const std::string& contentType);

        bool isSupportedContentType(const std::string& contentType);

        private:
        // Multipart parsing methods
        std::vector<MultipartPart> parseMultipartData(const std::string& body, const std::string& boundary);
        MultipartPart parseMultipartPart(const std::string& partData);
        void parseContentDisposition(const std::string& headerLine, std::string& name, std::string& fileName);
        std::string extractContentTypeFromHeader(const std::string& headerLine);

        // Processing and saving methods
        int processMultipartParts(const std::vector<MultipartPart>& parts);
        bool saveFileFromMultipart(const std::string& filePath, const std::string& content);
        void saveFormFieldToLog(const std::string& fieldName, const std::string& fieldValue);

        // Utility methods
        std::string extractBoundary(const std::string& contentType);
        std::string getExtensionFromContentType(const std::string& contentType);
        std::string generateFilename(const std::string& extension);
        // bool saveRawContent(const std::string& filePath, const std::string& content);

        std::string sanitizeFilename(const std::string& filename);

        std::string _uploadPath;
};

#endif

