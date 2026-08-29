#==============================================================================#
## MAKEFILE
#==============================================================================#

## VARIABLES
CXX                     =       c++
CXXFLAGS                =       -Wall -Wextra -Werror -std=c++98 -I./include
NAME                    =       webserv

## FILES
SRC_DIR					=		source
HDR_DIR					=		include
OBJ_DIR					=		obj
SRC_FILES               =       $(wildcard $(SRC_DIR)/*.cpp) main.cpp
HDR_FILES               =       $(wildcard $(HDR_DIR)/*.hpp)
OBJ_FILES               =       $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(filter $(SRC_DIR)/%.cpp,$(SRC_FILES))) \
                                $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(filter-out $(SRC_DIR)/%.cpp,$(SRC_FILES)))

## PROGRESS TRACKING
TOTAL_FILES             =       $(words $(SRC_FILES))
COUNTER_FILE            =       .compile_counter

## COLORS
GREEN                   =       \033[0;32m
CYAN                    =       \033[0;36m
YELLOW                  =       \033[0;33m
RED                     =       \033[0;31m
PURPLE                  =       \033[0;35m
NC                      =       \033[0m

## RULES 🔨
all:                            $(NAME)
				@printf "$(GREEN)🎉 $(NAME) has been successfully compiled! 🎉$(NC)\n"

$(NAME):                        $(OBJ_FILES)
				@printf "$(YELLOW)🔗 Linking objects into executable...$(NC)\n"
				@$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJ_FILES)
				@printf "$(GREEN)✅ Executable $(NAME) created! 📞$(NC)\n"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HDR_FILES)
				@mkdir -p $(OBJ_DIR)
				@if [ ! -f $(COUNTER_FILE) ]; then echo "0" > $(COUNTER_FILE); fi
				@COMPILED_COUNT=$$(cat $(COUNTER_FILE)); \
				 COMPILED_COUNT=$$(($$COMPILED_COUNT + 1)); \
				 echo "$$COMPILED_COUNT" > $(COUNTER_FILE); \
				 printf "\r$(CYAN)🔄 Compiling: $< $$COMPILED_COUNT/$(TOTAL_FILES)$(NC) 🟡"; \
				 if [ $$COMPILED_COUNT -eq $(TOTAL_FILES) ]; then echo; rm -f $(COUNTER_FILE); fi
				@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp $(HDR_FILES)
				@mkdir -p $(OBJ_DIR)
				@if [ ! -f $(COUNTER_FILE) ]; then echo "0" > $(COUNTER_FILE); fi
				@COMPILED_COUNT=$$(cat $(COUNTER_FILE)); \
				 COMPILED_COUNT=$$(($$COMPILED_COUNT + 1)); \
				 echo "$$COMPILED_COUNT" > $(COUNTER_FILE); \
				 printf "\r$(CYAN)🔄 Compiling: $< $$COMPILED_COUNT/$(TOTAL_FILES)$(NC) 🟡"; \
				 if [ $$COMPILED_COUNT -eq $(TOTAL_FILES) ]; then echo; rm -f $(COUNTER_FILE); fi
				@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
				@printf "$(YELLOW)🧹 Cleaning object files...$(NC)\n"
				@rm -rf $(OBJ_DIR)
				@rm -f $(COUNTER_FILE)
				@printf "$(GREEN)✨ Object files cleaned!$(NC)\n"

fclean:                         clean
				@printf "$(YELLOW)🗑️  Cleaning $(NAME) executable...$(NC)\n"
				@rm -f $(NAME)
				@printf "$(GREEN)🧼 All cleaned up!$(NC)\n"

re:                             fclean all
				@printf "$(PURPLE)🔄 Project rebuilt from scratch! 🔄$(NC)\n"

.PHONY:                         all clean fclean re
