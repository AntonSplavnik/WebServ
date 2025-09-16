# WebServ Makefile

NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g

# Directories
SRCDIR = src
OBJDIR = obj

# Source files
SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/server/server.cpp \
          $(SRCDIR)/socket/socket.cpp \
          $(SRCDIR)/config/config.cpp

# Object files
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Include directories
INCLUDES = -I$(SRCDIR)

# Default target
all: $(NAME)

# Create executable
$(NAME): $(OBJECTS)
	@echo "Linking $(NAME)..."
	@$(CXX) $(OBJECTS) -o $(NAME)
	@echo "✅ $(NAME) created successfully!"

# Compile source files to object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean object files
clean:
	@echo "Cleaning object files..."
	@rm -rf $(OBJDIR)
	@echo "✅ Clean complete!"

# Clean everything
fclean: clean
	@echo "Cleaning executable..."
	@rm -f $(NAME)
	@echo "✅ Full clean complete!"

# Rebuild everything
re: fclean all

# Test the server (compile and run)
test: $(NAME)
	@echo "🚀 Starting WebServ on port 8080..."
	@echo "Test with: curl http://localhost:8080"
	@echo "Or compile the test client: c++ test/TCP_client.cpp -o test_client && ./test_client"
	@echo "Press Ctrl+C to stop"
	@./$(NAME)

# Compile test client
test_client:
	@echo "Compiling test client..."
	@$(CXX) $(CXXFLAGS) test/TCP_client.cpp -o test_client
	@echo "✅ Test client ready! Run with: ./test_client"

# Help
help:
	@echo "Available targets:"
	@echo "  all          - Build the server"
	@echo "  clean        - Remove object files"
	@echo "  fclean       - Remove object files and executable"
	@echo "  re           - Rebuild everything"
	@echo "  test         - Build and run the server"
	@echo "  test_client  - Build the test client"
	@echo "  help         - Show this help"

.PHONY: all clean fclean re test test_client help
