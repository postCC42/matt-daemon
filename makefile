# NAME := Matt_daemon
# SRC_DIR := src
# INCLUDE_DIR := include
# BUILD_DIR := build

# SRC_FILES := main.cpp MattDaemon.cpp TintinReporter.cpp Utils.cpp
# SRCS := $(addprefix $(SRC_DIR)/,$(SRC_FILES))
# OBJS := $(SRC_FILES:%.cpp=$(BUILD_DIR)/%.o)
# DEPS := $(SRC_FILES:%.cpp=$(BUILD_DIR)/%.d)
# CXX_DEFS := NAME=\"$(NAME)\"

# CXX := g++
# CXX_FLAGS := -Wextra -Werror -Wall -std=c++17 -O2 -g3
# CXX_LIBS :=

# CXX_HEADERS := -I$(INCLUDE_DIR)

# CXX_DEPS_FLAGS := -MP -MMD
# CXX_DEFS_FLAGS := $(foreach def,$(CXX_DEFS),-D $(def))

# all: $(NAME)

# $(NAME): $(OBJS)
# 	$(CXX) $(CXX_FLAGS) $(CXX_HEADERS) $(OBJS) $(CXX_LIBS) -o $@

# $(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
# 	@mkdir -p $(@D)
# 	$(CXX) $(CXX_FLAGS) $(CXX_DEPS_FLAGS) $(CXX_DEFS_FLAGS) $(CXX_HEADERS) -c $< -o $@

# include $(wildcard $(DEPS))

# clean:
# 	@rm -rf $(BUILD_DIR)

# fclean: clean
# 	@rm -f $(NAME)

# re: fclean all

# .PHONY: all clean fclean re

NAME := Matt_daemon
WATCHDOG_NAME := watchdog

SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build

SRC_FILES := main.cpp MattDaemon.cpp TintinReporter.cpp Utils.cpp
WATCHDOG_SRC := watchdog.cpp
CLIENT_SRC := Client.cpp

SRCS := $(addprefix $(SRC_DIR)/,$(SRC_FILES))
OBJS := $(SRC_FILES:%.cpp=$(BUILD_DIR)/%.o)
WATCHDOG_OBJ := $(WATCHDOG_SRC:%.cpp=$(BUILD_DIR)/%.o)
CLIENT_OBJ := $(CLIENT_SRC:%.cpp=$(BUILD_DIR)/%.o)

DEPS := $(SRC_FILES:%.cpp=$(BUILD_DIR)/%.d) $(WATCHDOG_SRC:%.cpp=$(BUILD_DIR)/%.d)

CXX_DEFS := NAME=\"$(NAME)\"

CXX := g++
CXX_FLAGS := -Wextra -Werror -Wall -std=c++17 -O2 -g3
CLIENT_LIBS := -lncurses

CXX_HEADERS := -I$(INCLUDE_DIR)

CXX_DEPS_FLAGS := -MP -MMD
CXX_DEFS_FLAGS := $(foreach def,$(CXX_DEFS),-D $(def))

all: $(NAME) $(WATCHDOG_NAME)

client: $(CLIENT_OBJ)
	$(CXX) $(CXX_FLAGS) $(CXX_HEADERS) $(CLIENT_OBJ) $(CLIENT_LIBS) -o $@

$(NAME): $(OBJS)
	$(CXX) $(CXX_FLAGS) $(CXX_HEADERS) $(OBJS) -o $@

$(WATCHDOG_NAME): $(WATCHDOG_OBJ)
	$(CXX) $(CXX_FLAGS) $(CXX_HEADERS) $(WATCHDOG_OBJ) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(CXX_DEPS_FLAGS) $(CXX_DEFS_FLAGS) $(CXX_HEADERS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(CXX_DEPS_FLAGS) $(CXX_DEFS_FLAGS) $(CXX_HEADERS) -c $< -o $@

include $(wildcard $(DEPS))

clean:
	@rm -rf $(BUILD_DIR)

fclean: clean
	@rm -f $(NAME) $(WATCHDOG_NAME)

re: fclean all

.PHONY: all clean fclean re
