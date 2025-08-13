#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Client connection states
enum ClientState {
    READING_REQUEST,   // Waiting to read HTTP request
    SENDING_RESPONSE   // Ready to send HTTP response
};

// Structure to track client connection info
struct ClientInfo {
    int fd;
    ClientState state;
    std::string response_data;
    size_t bytes_sent;

    ClientInfo() : fd(-1), state(READING_REQUEST), bytes_sent(0) {}
};

// Function to make a socket non-blocking
// Non-blocking sockets don't wait for operations to complete
int make_socket_non_blocking(int fd) {
    std::cout << "Making socket FD " << fd << " non-blocking" << std::endl;
    int flags = fcntl(fd, F_GETFL, 0);  // Get current flags
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);  // Add non-blocking flag
}

int main() {
    // STEP 1: Create the listening socket
    // AF_INET = IPv4, SOCK_STREAM = TCP protocol
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        std::cerr << "Socket creation failed.\n";
        return 1;
    }
    std::cout << "Created listening socket with FD: " << listen_fd << std::endl;

    // STEP 2: Set socket options
    // SO_REUSEADDR allows reusing the address immediately after server shutdown
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    std::cout << "Set SO_REUSEADDR option on FD " << listen_fd << std::endl;

    // STEP 3: Configure server address structure
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));  // Clear the structure
    server_addr.sin_family = AF_INET;          // IPv4
    server_addr.sin_port = htons(PORT);        // Convert port to network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all available interfaces
    std::cout << "Configured server address for port " << PORT << std::endl;

    // STEP 4: Bind the socket to the address
    // This associates the socket with the specific IP address and port
    if (bind(listen_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Bind failed.\n";
        close(listen_fd);
        return 1;
    }
    std::cout << "Bound socket FD " << listen_fd << " to port " << PORT << std::endl;

    // STEP 5: Start listening for incoming connections
    // The second parameter (10) is the backlog - max pending connections
    if (listen(listen_fd, 10) < 0) {
        std::cerr << "Listen failed.\n";
        close(listen_fd);
        return 1;
    }
    std::cout << "Socket FD " << listen_fd << " is now listening (backlog: 10)" << std::endl;

    // STEP 6: Make the listening socket non-blocking
    make_socket_non_blocking(listen_fd);

    // STEP 7: Set up poll file descriptors array and client tracking
    // poll() monitors multiple file descriptors for I/O events
    struct pollfd fds[10];  // Array to hold up to 10 file descriptors
    ClientInfo clients[10]; // Track client connection states
    std::memset(fds, 0, sizeof(fds));  // Initialize array to zero

    // Add the listening socket to the poll array
    fds[0].fd = listen_fd;        // File descriptor to monitor
    fds[0].events = POLLIN;       // Monitor for incoming data/connections
    std::cout << "Added listening socket FD " << listen_fd << " to poll array at index 0" << std::endl;

    int nfds = 1;  // Number of file descriptors being monitored

    std::cout << "Server is listening on port " << PORT << "...\n";
    std::cout << "=== Current FD Status ===" << std::endl;
    std::cout << "fds[0].fd = " << fds[0].fd << " (listening socket)" << std::endl;

    // STEP 8: Main server loop
    while (true) {
        std::cout << "\n--- Waiting for events with poll() ---" << std::endl;

        // poll() blocks until at least one FD has an event
        // Parameters: fds array, number of fds to check, timeout (-1 = infinite)
        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            std::cerr << "Poll failed.\n";
            break;
        }

        std::cout << "poll() returned " << ret << " (number of FDs with events)" << std::endl;

        // STEP 9: Check if listening socket has incoming connection
        if (fds[0].revents & POLLIN) {
            std::cout << "Event detected on listening socket FD " << fds[0].fd << std::endl;

            // Accept the new connection
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &client_len);

            if (client_fd >= 0 && nfds < 10) {
                std::cout << "New connection accepted! Client FD: " << client_fd << std::endl;

                // Make client socket non-blocking
                make_socket_non_blocking(client_fd);

                // Add client socket to poll array
                fds[nfds].fd = client_fd;
                fds[nfds].events = POLLIN;  // Start by reading the request

                // Initialize client info
                clients[nfds].fd = client_fd;
                clients[nfds].state = READING_REQUEST;

                std::cout << "Added client socket FD " << client_fd << " to poll array at index " << nfds << std::endl;
                nfds++;
                std::cout << "New client connected. Total FDs: " << nfds << std::endl;
            }
        }

        // STEP 10: Check client sockets for events
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN && clients[i].state == READING_REQUEST) {
                std::cout << "POLLIN event on client FD " << fds[i].fd << " (reading request)" << std::endl;

                // Read data from client
                char buffer[BUFFER_SIZE];
                std::memset(buffer, 0, BUFFER_SIZE);
                int bytes = recv(fds[i].fd, buffer, BUFFER_SIZE - 1, 0);

                std::cout << "recv() returned " << bytes << " bytes from FD " << fds[i].fd << std::endl;

                if (bytes <= 0) {
                    // Client disconnected or error
                    std::cout << "Client FD " << fds[i].fd << " disconnected" << std::endl;
                    close(fds[i].fd);

                    // Remove from arrays by shifting remaining elements
                    for (int j = i; j < nfds - 1; j++) {
                        fds[j] = fds[j + 1];
                        clients[j] = clients[j + 1];
                    }
                    nfds--;
                    i--;  // Adjust index since we shifted elements
                }
                else {
                    // Request received, prepare response
                    std::cout << "Request received from FD " << fds[i].fd << ":\n" << buffer << std::endl;

                    // Prepare HTTP response
                    clients[i].response_data =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 12\r\n"
                        "\r\n";

                    clients[i].bytes_sent = 0;
                    clients[i].state = SENDING_RESPONSE;

                    // Switch to monitoring for write readiness
                    fds[i].events = POLLOUT;
                    std::cout << "Switched FD " << fds[i].fd << " to POLLOUT mode (ready to send response)" << std::endl;
                }
            }

            if (fds[i].revents & POLLOUT && clients[i].state == SENDING_RESPONSE) {
                std::cout << "POLLOUT event on client FD " << fds[i].fd << " (sending response)" << std::endl;

                // Send remaining response data
                const char* data = clients[i].response_data.c_str() + clients[i].bytes_sent;
                size_t remaining = clients[i].response_data.length() - clients[i].bytes_sent;

                int bytes_sent = send(fds[i].fd, data, remaining, 0);
                std::cout << "send() returned " << bytes_sent << " bytes to FD " << fds[i].fd << std::endl;

                if (bytes_sent > 0) {
                    clients[i].bytes_sent += bytes_sent;

                    // Check if entire response was sent
                    if (clients[i].bytes_sent == clients[i].response_data.length()) {
                        std::cout << "Complete response sent to FD " << fds[i].fd << ". Closing connection." << std::endl;
                        close(fds[i].fd);

                        // Remove from arrays
                        for (int j = i; j < nfds - 1; j++) {
                            fds[j] = fds[j + 1];
                            clients[j] = clients[j + 1];
                        }
                        nfds--;
                        i--;
                    } else {
                        std::cout << "Partial send: " << clients[i].bytes_sent << "/" << clients[i].response_data.length() << " bytes sent" << std::endl;
                    }
                } else {
                    // Send failed, close connection
                    std::cout << "Send failed for FD " << fds[i].fd << ". Closing connection." << std::endl;
                    close(fds[i].fd);

                    // Remove from arrays
                    for (int j = i; j < nfds - 1; j++) {
                        fds[j] = fds[j + 1];
                        clients[j] = clients[j + 1];
                    }
                    nfds--;
                    i--;
                }
            }

            // Handle client disconnection
            if (fds[i].revents & POLLHUP) {
                std::cout << "Client FD " << fds[i].fd << " hung up" << std::endl;
                close(fds[i].fd);

                // Remove from arrays
                for (int j = i; j < nfds - 1; j++) {
                    fds[j] = fds[j + 1];
                    clients[j] = clients[j + 1];
                }
                nfds--;
                i--;
            }
        }
    }

    // STEP 12: Cleanup - close listening socket
    std::cout << "\nShutting down server. Closing listening socket FD " << listen_fd << std::endl;
    close(listen_fd);
    return 0;
}
