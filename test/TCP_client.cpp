/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   TCP_client.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:08 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/09/16 17:18:10 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string>

int main() {
    // Create socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    // Connect to server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed: " << strerror(errno) << std::endl;
        close(client_fd);
        return -1;
    }

    std::cout << "Connected to server!" << std::endl;
    std::cout << "Choose HTTP method (GET, POST) or 'quit' to exit:" << std::endl;

    std::string input;
    while (true) {
        std::cout << "\nEnter HTTP method (GET/POST/quit): ";
        std::getline(std::cin, input);

        if (input == "quit") {
            break;
        }

        std::string http_request;

        if (input == "GET" || input == "get") {
            // Create HTTP GET request
            http_request =
                "GET / HTTP/1.1\r\n"
                "Host: localhost:8080\r\n"
                "User-Agent: TestClient/1.0\r\n"
                "Accept: text/plain\r\n"
                "Connection: close\r\n"
                "\r\n";
            std::cout << "Preparing HTTP GET request..." << std::endl;

        } else if (input == "POST" || input == "post") {
            // Get POST data from user
            std::cout << "Enter POST data: ";
            std::string post_data;
            std::getline(std::cin, post_data);

            // Create HTTP POST request
            http_request =
                "POST / HTTP/1.1\r\n"
                "Host: localhost:8080\r\n"
                "User-Agent: TestClient/1.0\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: " + std::to_string(post_data.length()) + "\r\n"
                "Connection: close\r\n"
                "\r\n" +
                post_data;
            std::cout << "Preparing HTTP POST request with " << post_data.length() << " bytes of data..." << std::endl;

        } else {
            std::cout << "Invalid method. Use GET, POST, or quit." << std::endl;
            continue;
        }

        // Display the HTTP request we're sending
        std::cout << "\n=== HTTP Request ===" << std::endl;
        std::cout << http_request << std::endl;
        std::cout << "==================" << std::endl;

        // SEND: Send HTTP request to server
        std::cout << "SEND: Sending HTTP request (" << http_request.length() << " bytes)" << std::endl;
        ssize_t bytes_sent = send(client_fd, http_request.c_str(), http_request.length(), 0);

        if (bytes_sent > 0) {
            std::cout << "SEND: Successfully sent " << bytes_sent << " bytes" << std::endl;
        } else {
            std::cout << "SEND: Failed - " << strerror(errno) << std::endl;
            break;
        }

        // RECV: Get HTTP response from server
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        std::cout << "RECV: Waiting for HTTP response..." << std::endl;
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "RECV: Got " << bytes_received << " bytes" << std::endl;
            std::cout << "\n=== HTTP Response ===" << std::endl;
            std::cout << buffer << std::endl;
            std::cout << "====================" << std::endl;
        } else if (bytes_received == 0) {
            std::cout << "RECV: Server closed connection" << std::endl;
        } else {
            std::cout << "RECV: Error - " << strerror(errno) << std::endl;
            break;
        }

        // Server closes connection after each request, so we need to reconnect
        std::cout << "\nConnection closed by server. Reconnecting for next request..." << std::endl;
        close(client_fd);

        // Reconnect for next request
        client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fd == -1) {
            std::cerr << "Socket creation failed" << std::endl;
            return -1;
        }

        if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Reconnection failed: " << strerror(errno) << std::endl;
            return -1;
        }

        std::cout << "Reconnected to server!" << std::endl;
    }

    close(client_fd);
    std::cout << "\nDisconnected from server" << std::endl;
    return 0;
}
