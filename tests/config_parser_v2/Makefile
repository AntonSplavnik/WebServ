# Test program for Config Parser V2
NAME = test_parser_v2

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic -g

# Source directories
CONFIG_V2_DIR = ../../src/config_v2
LEXER_DIR = $(CONFIG_V2_DIR)/lexer
PARSER_DIR = $(CONFIG_V2_DIR)/parser
VALIDATOR_DIR = $(CONFIG_V2_DIR)/validator
BUILDER_DIR = $(CONFIG_V2_DIR)/builder
EXCEPTIONS_DIR = $(CONFIG_V2_DIR)/exceptions
UTILS_DIR = $(CONFIG_V2_DIR)/utils

# Include paths
INCLUDES = -I$(CONFIG_V2_DIR) \
           -I$(LEXER_DIR) \
           -I$(PARSER_DIR) \
           -I$(VALIDATOR_DIR) \
           -I$(BUILDER_DIR) \
           -I$(EXCEPTIONS_DIR) \
           -I$(UTILS_DIR) \
           -I../../src

# Source files - ALL from config_v2, NO dependencies on old config
SRCS = test_parser_v2.cpp \
       $(LEXER_DIR)/token.cpp \
       $(LEXER_DIR)/lexer.cpp \
       $(PARSER_DIR)/directive.cpp \
       $(PARSER_DIR)/parser.cpp \
       $(VALIDATOR_DIR)/directive_rules.cpp \
       $(VALIDATOR_DIR)/validator.cpp \
       $(BUILDER_DIR)/config_builder.cpp \
       $(CONFIG_V2_DIR)/config_structures.cpp \
       $(CONFIG_V2_DIR)/config_v2.cpp \
       $(UTILS_DIR)/config_utils.cpp

OBJS = $(SRCS:.cpp=.o)

# Build
all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

test: all
	./$(NAME) config_example_not_for_execution.conf

.PHONY: all clean fclean re test
