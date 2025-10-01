# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/30 00:00:00 by antonsplavn      #+#    #+#              #
#    Updated: 2025/09/30 00:00:00 by antonsplavn      ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Test executable name
TEST_NAME	= run_tests

# Compiler and flags
CXX			= c++
CXXFLAGS	= -std=c++20 -Wall -Wextra -Werror -pedantic
GTEST_LIB	= $(shell brew --prefix googletest)/lib
TEST_FLAGS	= -L$(GTEST_LIB) -lgtest -lgtest_main -pthread

# Directories
ROOT_DIR	= ..
SRC_DIR		= $(ROOT_DIR)/src
OBJ_DIR		= obj

# Include paths
GTEST_DIR	= $(shell brew --prefix googletest)
INCLUDES	=  -I$(SRC_DIR)/http_request \
			  -I$(GTEST_DIR)/include

# Source files from main project (exclude main.cpp)
PROJECT_SRC	=  $(SRC_DIR)/http_request/http_request.cpp \


# Test source files
TEST_SRC	= $(wildcard http_request/*.cpp)

# Object files
PROJECT_OBJ	= $(PROJECT_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TEST_OBJ	= $(TEST_SRC:%.cpp=$(OBJ_DIR)/%.o)
ALL_OBJ		= $(PROJECT_OBJ) $(TEST_OBJ)

# Colors
RED			= \033[0;31m
GREEN		= \033[0;32m
YELLOW		= \033[0;33m
CYAN		= \033[0;36m
MAGENTA		= \033[0;35m
RESET		= \033[0m

# Default target
all: $(TEST_NAME)

# Test executable
$(TEST_NAME): $(ALL_OBJ)
	@echo "$(GREEN)Linking $(TEST_NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(ALL_OBJ) -o $(TEST_NAME) $(TEST_FLAGS)
	@echo "$(GREEN)✓ $(TEST_NAME) created successfully!$(RESET)"

# Compile project source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)Compiling $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile test files
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)Compiling test $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run tests
run: $(TEST_NAME)
	@echo "$(CYAN)Running tests...$(RESET)"
	@./$(TEST_NAME)

# Clean object files
clean:
	@echo "$(RED)Cleaning test object files...$(RESET)"
	@rm -rf $(OBJ_DIR)
	@echo "$(RED)✓ Test object files cleaned!$(RESET)"

# Clean everything
fclean: clean
	@echo "$(RED)Cleaning $(TEST_NAME)...$(RESET)"
	@rm -f $(TEST_NAME)
	@echo "$(RED)✓ $(TEST_NAME) cleaned!$(RESET)"

# Rebuild everything
re: fclean all

# Run with valgrind
valgrind: $(TEST_NAME)
	@echo "$(MAGENTA)Running tests with valgrind...$(RESET)"
	@valgrind --leak-check=full --show-leak-kinds=all ./$(TEST_NAME)

# Help
help:
	@echo "$(CYAN)Available targets:$(RESET)"
	@echo "  $(GREEN)all$(RESET)      - Build test executable"
	@echo "  $(GREEN)run$(RESET)      - Build and run tests"
	@echo "  $(GREEN)clean$(RESET)    - Remove object files"
	@echo "  $(GREEN)fclean$(RESET)   - Remove object files and test executable"
	@echo "  $(GREEN)re$(RESET)       - Rebuild everything"
	@echo "  $(GREEN)valgrind$(RESET) - Run tests with valgrind"
	@echo "  $(GREEN)help$(RESET)     - Show this help message"

# Phony targets
.PHONY: all run clean fclean re valgrind help

# Silent mode
.SILENT:
