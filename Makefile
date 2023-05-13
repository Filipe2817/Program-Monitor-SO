# Recursive wildcard 
# The first parameter ($1) is a list of directories, and the second ($2) is a list of patterns to match
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Default to release build
# Override with `make client/server BUILD=debug` or `make client/server BUILD=sanitizer`
BUILD := release

# Compiler
CC = gcc

# Flags
FLAGS.release = -Wall -Wextra -Wdouble-promotion -Werror=pedantic -Werror=vla -pedantic-errors -Wfatal-errors -flto -O3 -mtune=native -march=native
FLAGS.sanitizer = -Wall -Wextra -Wdouble-promotion -Werror=pedantic -Werror=vla -pedantic-errors -Wfatal-errors -g -fsanitize=address
FLAGS.debug = -Wall -Wextra -Wdouble-promotion -Werror=pedantic -Werror=vla -pedantic-errors -Wfatal-errors -O0 -g
FLAGS := $(FLAGS.$(BUILD))

# Makefile Flags
MAKEFLAGS += --no-print-directory

# Directories
OBJ_DIR := build

# Get sources, headers and object files for client
CLIENT_SRCS = $(call rwildcard, client/src, *.c)
CLIENT_HEADERS = $(call rwildcard, client/include, *.h)
CLIENT_OBJS = $(CLIENT_SRCS:client/src/%.c=$(OBJ_DIR)/client/%.o)

# Get sources, headers and object files for server
SERVER_SRCS = $(call rwildcard, server/src, *.c)
SERVER_HEADERS = $(call rwildcard, server/include, *.h)
SERVER_OBJS = $(SERVER_SRCS:server/src/%.c=$(OBJ_DIR)/server/%.o)

# Get sources, headers, and object files for common
COMMON_SRCS = $(call rwildcard, common/src, *.c)
COMMON_HEADERS = $(call rwildcard, common/include, *.h)
COMMON_OBJS = $(COMMON_SRCS:common/src/%.c=$(OBJ_DIR)/common/%.o)

# Other stuff to compile with
LIBS = -lm
INCLUDE = -Iclient/include -Iserver/include -Icommon/include
CLIENT_TARGET = tracer
SERVER_TARGET = monitor

# Pretty stuff (using %b in printf to interpret escape sequences as special characters) (alternatively echo -e "string" can be used)
NO_COLOR = \033[m
LINKING_COLOR = \033[0;33m
COMPILING_COLOR = \033[0;33m
DELETING_COLOR = \033[0;31m
OK_COLOR = \033[0;32m
COMPILING_STRING = "[COMPILING]"
LINKING_STRING = "[LINKING]"
DELETING_STRING = "[DELETING]"
RESETTING_STRING = "[RESETTING]"
OK_STRING = "[OK]"

# Link files for common
$(OBJ_DIR)/common/%.o: common/src/%.c $(COMMON_HEADERS)
	@mkdir -p $(dir $@)
	@printf "%b" "$(LINKING_COLOR)$(LINKING_STRING) $(NO_COLOR)$@\n"
	@$(CC) $(FLAGS) -c -o $@ $(INCLUDE) $< $(LIBS)

# Link files for client
$(OBJ_DIR)/client/%.o: client/src/%.c $(CLIENT_HEADERS)
	@mkdir -p $(dir $@)
	@printf "%b" "$(LINKING_COLOR)$(LINKING_STRING) $(NO_COLOR)$@\n"
	@$(CC) $(FLAGS) -c -o $@ $(INCLUDE) $< $(LIBS)

# Link files for server
$(OBJ_DIR)/server/%.o: server/src/%.c $(SERVER_HEADERS)
	@mkdir -p $(dir $@)
	@printf "%b" "$(LINKING_COLOR)$(LINKING_STRING) $(NO_COLOR)$@\n"
	@$(CC) $(FLAGS) -c -o $@ $(INCLUDE) $< $(LIBS)

# Create client executable
.PHONY: client
client: $(CLIENT_OBJS) $(COMMON_OBJS)
	@printf "%b" "$(COMPILING_COLOR)$(COMPILING_STRING) $(NO_COLOR)$(CLIENT_TARGET)\n"
	@$(CC) $(FLAGS) -o $(CLIENT_TARGET) $(CLIENT_OBJS) $(COMMON_OBJS) $(LIBS)
	@printf "%b" "$(OK_COLOR)$(OK_STRING) $(NO_COLOR)\n"

# Create server executable
.PHONY: server
server: $(SERVER_OBJS) $(COMMON_OBJS)
	@printf "%b" "$(COMPILING_COLOR)$(COMPILING_STRING) $(NO_COLOR)$(SERVER_TARGET)\n"
	@$(CC) $(FLAGS) -o $(SERVER_TARGET) $(SERVER_OBJS) $(COMMON_OBJS) $(LIBS)
	@printf "%b" "$(OK_COLOR)$(OK_STRING) $(NO_COLOR)\n"

# Run gdb (clean up any existing build and build debug mode)
# Warning: make gdb client will execute "gdb client" and then "client" (pain to debug)
# Solution: make gdb cli and make gdb srv
.PHONY: gdb
gdb:
	@printf "%b" "$(COMPILING_COLOR)Deleting old build!$(NO_COLOR)\n"
	@make clean
	@if [ $(words $(MAKECMDGOALS)) -eq 2 ] && [ $(word 2, $(MAKECMDGOALS)) = cli ]; then \
		printf "%b" "$(COMPILING_COLOR)Building debug mode for client!$(NO_COLOR)\n"; \
		make client BUILD=debug; \
		gdb --args $(CLIENT_TARGET); \
	elif [ $(words $(MAKECMDGOALS)) -eq 2 ] && [ $(word 2, $(MAKECMDGOALS)) = srv ]; then \
		printf "%b" "$(COMPILING_COLOR)Building debug mode for server!$(NO_COLOR)\n"; \
		make server BUILD=debug; \
		gdb --args $(SERVER_TARGET); \
	else \
		printf "%b" "$(COMPILING_COLOR)Invalid command!$(NO_COLOR)\n"; \
	fi

# Run valgrind (clean up any existing build and build debug mode)
.PHONY: valgrind
valgrind:
	@printf "%b" "$(COMPILING_COLOR)Deleting old build!$(NO_COLOR)\n"
	@make clean
	@if [ $(words $(MAKECMDGOALS)) -eq 2 ] && [ $(word 2, $(MAKECMDGOALS)) = cli ]; then \
		printf "%b" "$(COMPILING_COLOR)Building debug mode for client!$(NO_COLOR)\n"; \
		make client BUILD=debug; \
		valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all -s ./$(CLIENT_TARGET); \
	elif [ $(words $(MAKECMDGOALS)) -eq 2 ] && [ $(word 2, $(MAKECMDGOALS)) = srv ]; then \
		printf "%b" "$(COMPILING_COLOR)Building debug mode for server!$(NO_COLOR)\n"; \
		make server BUILD=debug; \
		valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all -s ./$(SERVER_TARGET); \
	else \
		printf "%b" "$(COMPILING_COLOR)Invalid command!$(NO_COLOR)\n"; \
	fi

# Add no-op for this rules since they are just used to identify the targets in the previous rules
.PHONY: cli srv
cli:
	@:

srv:
	@:

# Remove the object directory (and its contents), client and server executables and clean path variables
.PHONY: clean
clean:
	@printf "%b" "$(DELETING_COLOR)$(DELETING_STRING)$(NO_COLOR) $(CLIENT_TARGET) and $(SERVER_TARGET) executables\n"
	@printf "%b" "$(DELETING_COLOR)$(DELETING_STRING)$(NO_COLOR) $(OBJ_DIR) directory\n"
	@rm -rf $(CLIENT_TARGET) $(SERVER_TARGET) $(OBJ_DIR)
	@printf "%b" "$(OK_COLOR)$(OK_STRING)$(NO_COLOR)\n"
