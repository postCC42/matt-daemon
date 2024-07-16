NAME := Matt_daemon
WATCHDOG_NAME := watchdog

SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build

SRC_FILES := main.cpp MattDaemon.cpp Tintin_reporter.cpp Utils.cpp
WATCHDOG_SRC := Watchdog.cpp
CLIENT_SRC := Client.cpp

SRCS := $(addprefix $(SRC_DIR)/,$(SRC_FILES))
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
WATCHDOG_OBJ := $(BUILD_DIR)/Watchdog.o
CLIENT_OBJ := $(CLIENT_SRC:%.cpp=$(BUILD_DIR)/%.o)

DEPS := $(OBJS:%.o=%.d) $(WATCHDOG_OBJ:%.o=%.d)

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
	$(CXX) $(CXX_FLAGS) $(CXX_HEADERS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(CXX_DEPS_FLAGS) $(CXX_DEFS_FLAGS) $(CXX_HEADERS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(CXX_DEPS_FLAGS) $(CXX_DEFS_FLAGS) $(CXX_HEADERS) -c $< -o $@

-include $(DEPS)

clean:
	@rm -rf $(BUILD_DIR)

fclean: clean
	@rm -f $(NAME) $(WATCHDOG_NAME) client

re: fclean all

.PHONY: all clean fclean re client $(WATCHDOG_NAME)
