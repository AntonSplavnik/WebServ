#include <iostream>
#include <csignal>
#include "server/server.hpp"
#include "config/config.hpp"

// Global server pointer for signal handling
Server* g_server = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down server..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
    exit(0);
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [port]" << std::endl;
    std::cout << "  port: Port number to listen on (default: 8080)" << std::endl;
    std::cout << "Example: " << program_name << " 3000" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== WebServ Starting ===" << std::endl;

    // Parse command line arguments
    int port = 8080; // default port
    if (argc > 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    if (argc == 2) {
        try {
            port = std::stoi(argv[1]);
            if (port < 1 || port > 65535) {
                std::cerr << "Error: Port must be between 1 and 65535" << std::endl;
                return 1;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid port number '" << argv[1] << "'" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    try {
        // Create and configure server
        std::cout << "Creating server on port " << port << "..." << std::endl;
        Server server;
        g_server = &server;
        
        // Set up signal handlers for graceful shutdown
        signal(SIGINT, signalHandler);   // Ctrl+C
        signal(SIGTERM, signalHandler);  // Termination signal
        
        // Set port
        server.setPort(port);
        
        std::cout << "Initializing server..." << std::endl;
        server.initialize();
        
        std::cout << "Starting server..." << std::endl;
        server.start();
        
        std::cout << "Server listening on port " << port << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        std::cout << "You can test with: curl http://localhost:" << port << std::endl;
        std::cout << "Or use the test client in test/TCP_client.cpp" << std::endl;
        std::cout << "========================" << std::endl;
        
        // Run server (this will block until stopped)
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown server error occurred" << std::endl;
        return 1;
    }

    std::cout << "Server shutdown complete." << std::endl;
    return 0;
}
