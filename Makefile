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
INCLUDES	= -Isrc/network/socket -Isrc/network/connection -Isrc/network/connection_pool \
			  -Isrc/network/listening_socket_manager -Isrc/http/http_request -Isrc/http/http_response \
			  -Isrc/event_loop -Isrc/config/config -Isrc/config/config_parser \
			  -Isrc/config/config_helpers -Isrc/config/config_exceptions \
			  -Isrc/cgi/cgi -Isrc/cgi/cgi_executor -Isrc/request_handler \
			  -Isrc/request_router -Isrc/statistics -Isrc/logging

# Directories
SRC_DIR		= src
OBJ_DIR		= obj

# Network layer
SOCKET_DIR	= $(SRC_DIR)/network/socket
CONNECTION_DIR	= $(SRC_DIR)/network/connection
CONN_POOL_DIR	= $(SRC_DIR)/network/connection_pool
LISTEN_MGR_DIR	= $(SRC_DIR)/network/listening_socket_manager

# HTTP layer
HTTP_REQ_DIR	= $(SRC_DIR)/http/http_request
HTTP_RES_DIR	= $(SRC_DIR)/http/http_response

# Core
EVENT_LOOP_DIR	= $(SRC_DIR)/event_loop
LOGGING_DIR	= $(SRC_DIR)/logging

# Config
CONFIG_DIR	= $(SRC_DIR)/config/config
CONFIG_PARSER_DIR = $(SRC_DIR)/config/config_parser
CONFIG_HELPERS_DIR = $(SRC_DIR)/config/config_helpers
CONFIG_EXCEPT_DIR = $(SRC_DIR)/config/config_exceptions

# CGI
CGI_DIR		= $(SRC_DIR)/cgi/cgi
CGI_EXEC_DIR	= $(SRC_DIR)/cgi/cgi_executor

# Request handling
REQ_HANDLER_DIR	= $(SRC_DIR)/request_handler
REQ_ROUTER_DIR	= $(SRC_DIR)/request_router

# Statistics
STATISTICS_DIR	= $(SRC_DIR)/statistics

# Source files
SRC_FILES	= main.cpp \
			  $(SOCKET_DIR)/socket.cpp \
			  $(CONNECTION_DIR)/connection.cpp \
			  $(CONN_POOL_DIR)/connection_pool_manager.cpp \
			  $(LISTEN_MGR_DIR)/listening_socket_manager.cpp \
			  $(HTTP_REQ_DIR)/request.cpp \
			  $(HTTP_RES_DIR)/response.cpp \
			  $(EVENT_LOOP_DIR)/event_loop.cpp \
			  $(CONFIG_DIR)/config.cpp \
			  $(CONFIG_PARSER_DIR)/directives_parsers.cpp \
			  $(CONFIG_HELPERS_DIR)/helpers.cpp \
			  $(CGI_DIR)/cgi.cpp \
			  $(CGI_DIR)/cgi_helpers.cpp \
			  $(CGI_EXEC_DIR)/cgi_executor.cpp \
			  $(REQ_HANDLER_DIR)/request_handler.cpp \
			  $(REQ_HANDLER_DIR)/post_handler.cpp \
			  $(REQ_ROUTER_DIR)/request_router.cpp \
			  $(STATISTICS_DIR)/statistics_collector.cpp \
			  $(LOGGING_DIR)/logger.cpp

# Object files
OBJ_FILES	= $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

# Header files for dependencies
HEADERS		= $(SOCKET_DIR)/socket.hpp \
			  $(CONNECTION_DIR)/connection.hpp \
			  $(CONN_POOL_DIR)/connection_pool_manager.hpp \
			  $(LISTEN_MGR_DIR)/listening_socket_manager.hpp \
			  $(HTTP_REQ_DIR)/request.hpp \
			  $(HTTP_RES_DIR)/response.hpp \
			  $(EVENT_LOOP_DIR)/event_loop.hpp \
			  $(CONFIG_DIR)/config.hpp \
			  $(CONFIG_HELPERS_DIR)/helpers.hpp \
			  $(CONFIG_EXCEPT_DIR)/config_exceptions.hpp \
			  $(CGI_DIR)/cgi.hpp \
			  $(CGI_EXEC_DIR)/cgi_executor.hpp \
			  $(REQ_HANDLER_DIR)/request_handler.hpp \
			  $(REQ_HANDLER_DIR)/post_handler.hpp \
			  $(REQ_ROUTER_DIR)/request_router.hpp \
			  $(STATISTICS_DIR)/statistics_collector.hpp \
			  $(LOGGING_DIR)/logger.hpp

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
