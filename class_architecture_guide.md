# WebServ Class Architecture Guide

i need you to teach me class arcitecture based on the current project. i'm a student, so make sure that you explain things and not just implementing. follow my instructions dont derrive from it. This project is webserver like nginx implementation with http 1.1 protocol only.

## Project Overview
This WebServ project is an HTTP/1.1 web server implementation similar to nginx. The codebase demonstrates fundamental class architecture principles through a real-world networking application.

## Class Architecture Analysis

### Core Classes Structure

#### 1. Server Class (`src/server/server.hpp`)
The main orchestrator class that manages the entire web server lifecycle.

**Responsibilities:**
- Client connection management using poll()
- HTTP method handling (GET, POST, DELETE)
- Timeout and connection state management
- Event loop coordination

**Key Members:**
- `Socket _serverSocket` - Main listening socket
- `std::vector<struct pollfd> _pollFds` - Event polling management
- `std::map<int, ClientInfo> _clients` - Active client tracking

#### 2. Socket Class (`src/socket/socket.hpp`)
Low-level socket operations wrapper.

**Responsibilities:**
- Socket creation and configuration
- Port binding and connection listening
- Client connection acceptance
- Non-blocking socket setup

#### 3. HttpRequest Class (`src/http_request/http_request.hpp`)
HTTP request parsing and representation.

**Responsibilities:**
- Raw HTTP data parsing
- Request line extraction (method, path, version)
- Header and body processing
- Request validation

#### 4. HttpResponse Class (`src/http_response/http_response.hpp`)
HTTP response generation and formatting.

**Responsibilities:**
- Status code and response generation
- Content type determination
- Response body creation
- HTTP response formatting

#### 5. HttpMessage Class (`src/http_message/ahttp_messag.hpp`)
Base functionality for HTTP messages.

**Responsibilities:**
- Common HTTP header management
- Shared parsing utilities
- Base validation logic

#### 6. Config Class (`src/config/config.hpp`)
Configuration management system.

**Responsibilities:**
- Server settings storage
- Configuration data access
- Future configuration file parsing

### Supporting Structures

#### ClientInfo Structure
Tracks individual client connection state:
- Socket information
- Connection state (READING_REQUEST/SENDING_RESPONSE)
- Timeout management
- Request/response data buffers

#### Enums for Type Safety
- `Methods`: GET, POST, DELETE
- `ClientState`: Connection states
- `fileExtentions`: Supported file types

## Architecture Principles Demonstrated

### Single Responsibility
Each class handles one specific aspect of the web server functionality.

### Encapsulation
Internal implementation details are hidden behind clean public interfaces.

### Composition
Complex functionality built by combining simpler, focused classes.

### State Management
Clear state tracking for client connections and request processing.

## Class Interaction Flow

1. **Initialization**: Server creates Socket and starts listening
2. **Connection**: Server accepts clients and creates ClientInfo
3. **Request**: HttpRequest parses incoming data
4. **Processing**: Server determines method and handles request
5. **Response**: HttpResponse generates appropriate response
6. **Cleanup**: Connection state management and resource cleanup

This architecture provides a solid foundation for understanding how complex network applications can be structured using object-oriented design principles.