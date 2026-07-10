
NAME = Gomoku

SRC_DIR = src
OBJ_DIR = obj

SRCS = main.cpp ui.cpp ai.cpp rules.cpp heuristic.cpp transposition.cpp
OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))
DEPS = $(OBJS:.o=.d)

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -MMD -MP
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system

SFML_PREFIX ?= $(shell brew --prefix sfml 2>/dev/null)
ifneq ($(SFML_PREFIX),)
CXXFLAGS += -I$(SFML_PREFIX)/include
LDFLAGS += -L$(SFML_PREFIX)/lib -Wl,-rpath,$(SFML_PREFIX)/lib
endif

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(OBJS) -o $(NAME) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

# Inclusion des fichiers de dépendances générés automatiquement (.d),
# pour recompiler un .cpp si un .hpp qu'il inclut a changé -- pas
# seulement si le .cpp lui-même a changé.
-include $(DEPS)

.PHONY: all clean fclean re