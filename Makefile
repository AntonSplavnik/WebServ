# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/16 00:00:00 by vnik              #+#    #+#              #
#    Updated: 2025/10/14 14:25:16 by antonsplavn      ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Program name
NAME		= webserv

# Compiler and flags
CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 -pedantic
DEBUG_FLAGS	= -g -fsanitize=address -fsanitize=undefined
INCLUDES	= -Isrc/server -Isrc/socket -Isrc/config -Isrc/http -Isrc/http_request -Isrc/http_response -Isrc/helpers -Isrc/server_controller -Isrc/logging -Isrc/exceptions -Isrc/cgi


# Directories
SRC_DIR		= src
OBJ_DIR		= obj
SERVER_DIR	= $(SRC_DIR)/server
SOCKET_DIR	= $(SRC_DIR)/socket
CONFIG_DIR	= $(SRC_DIR)/config
HELPERS_DIR	= $(SRC_DIR)/helpers
HTTP_REQ_DIR	= $(SRC_DIR)/http_request
HTTP_RES_DIR	= $(SRC_DIR)/http_response
SERVER_MGR_DIR	= $(SRC_DIR)/server_controller
LOGGING_DIR	= $(SRC_DIR)/logging
EXCEPTIONS_DIR	= $(SRC_DIR)/exceptions
CGI_DIR		= $(SRC_DIR)/cgi

# Source files
SRC_FILES	= main.cpp \
			  $(SERVER_DIR)/server.cpp \
			  $(SERVER_DIR)/post_handler.cpp \
			  $(SOCKET_DIR)/socket.cpp \
			  $(CONFIG_DIR)/config.cpp \
			  $(CONFIG_DIR)/directives_parsers.cpp \
			  $(HTTP_REQ_DIR)/http_request.cpp \
			  $(HTTP_RES_DIR)/http_response.cpp \
			  $(SERVER_MGR_DIR)/server_controller.cpp \
			  $(LOGGING_DIR)/logger.cpp \
			  $(HELPERS_DIR)/helpers.cpp \
			  $(CGI_DIR)/cgi.cpp


# Object files
OBJ_FILES	= $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

# Header files for dependencies
HEADERS		= $(SERVER_DIR)/server.hpp \
			  $(SERVER_DIR)/post_handler.hpp \
			  $(SERVER_DIR)/client_info.hpp \
			  $(SOCKET_DIR)/socket.hpp \
			  $(CONFIG_DIR)/config.hpp \
			  $(HTTP_REQ_DIR)/http_request.hpp \
			  $(HTTP_RES_DIR)/http_response.hpp \
			  $(SERVER_MGR_DIR)/server_controller.hpp \
			  $(LOGGING_DIR)/logger.hpp \
			  $(EXCEPTIONS_DIR)/config_exceptions.hpp \
			  $(HELPERS_DIR)/helpers.hpp \
			  $(CGI_DIR)/cgi.hpp

# Colors for pretty output
RED			= \033[0;31m
GREEN		= \033[0;32m
YELLOW		= \033[0;33m
BLUE		= \033[0;34m
MAGENTA		= \033[0;35m
CYAN		= \033[0;36m
RESET		= \033[0m

# Default target
all: $(NAME)

# Main executable
$(NAME): $(OBJ_FILES)
	@echo "$(GREEN)Linking $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJ_FILES) -o $(NAME)
	@echo "$(GREEN)✓ $(NAME) created successfully!$(RESET)"

# Object file compilation - define explicit rules for each source file
define compile_rule
$(OBJ_DIR)/$(1:.cpp=.o): $(1) $(HEADERS)
	@mkdir -p $$(dir $$@)
	@echo "$$(YELLOW)Compiling $$<...$$(RESET)"
	@$$(CXX) $$(CXXFLAGS) $$(INCLUDES) -c $$< -o $$@
endef

$(foreach src,$(SRC_FILES),$(eval $(call compile_rule,$(src))))

# Debug build
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: clean $(NAME)
	@echo "$(CYAN)✓ Debug build created!$(RESET)"

# Clean object files
clean:
	@echo "$(RED)Cleaning object files...$(RESET)"
	@rm -rf $(OBJ_DIR)
	@echo "$(RED)✓ Object files cleaned!$(RESET)"

# Clean everything
fclean: clean
	@echo "$(RED)Cleaning $(NAME)...$(RESET)"
	@rm -f $(NAME)
	@echo "$(RED)✓ $(NAME) cleaned!$(RESET)"

# Rebuild everything
re: fclean all

# Test targets
test: $(NAME)
	@echo "$(BLUE)Running basic server test...$(RESET)"
	@./$(NAME) &
	@sleep 1
	@curl -s http://localhost:8080 || echo "Server test complete"
	@pkill -f $(NAME) 2>/dev/null || true

# Run with valgrind (if available)
valgrind: $(NAME)
	@echo "$(MAGENTA)Running with valgrind...$(RESET)"
	@valgrind --leak-check=full --show-leak-kinds=all ./$(NAME)

# Show help
help:
	@echo "$(CYAN)Available targets:$(RESET)"
	@echo "  $(GREEN)all$(RESET)      - Build the project"
	@echo "  $(GREEN)clean$(RESET)    - Remove object files"
	@echo "  $(GREEN)fclean$(RESET)   - Remove object files and executable"
	@echo "  $(GREEN)re$(RESET)       - Rebuild everything"
	@echo "  $(GREEN)debug$(RESET)    - Build with debug flags"
	@echo "  $(GREEN)test$(RESET)     - Run basic functionality test"
	@echo "  $(GREEN)valgrind$(RESET) - Run with valgrind memory check"
	@echo "  $(GREEN)help$(RESET)     - Show this help message"

# Phony targets
.PHONY: all clean fclean re debug test valgrind help

# Silent mode
.SILENT:
