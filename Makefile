# Makefile pour le projet Gomoku
#
# Règles obligatoires selon le sujet : $(NAME), all, clean, fclean, re.
# Le programme ne doit PAS relinker inutilement -- on utilise la
# compilation incrémentale standard (fichiers objets + suivi des
# dépendances via -MMD/-MP) pour ne recompiler que ce qui a changé.

NAME = Gomoku

SRC_DIR = src
OBJ_DIR = obj

SRCS = main.cpp ui.cpp ai.cpp rules.cpp heuristic.cpp transposition.cpp
OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))
DEPS = $(OBJS:.o=.d)

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -MMD -MP
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system

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