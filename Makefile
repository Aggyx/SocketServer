
#COMP
COMP = c++ -std=c++98 #-fsanitize=address

RM = rm -f

# FILE MANAGEMENT
NAME = Webserv

SRC = main.cpp \
	Client/Client.cpp \
	Http/Http.cpp \
	Logger/Logger.cpp \
	Sockets/MotherSocket.cpp \
	Server/Server.cpp \
	Config/ConfigParser.cpp \
	Request/Request.cpp \
	Response/Response.cpp \
	Router/Router.cpp \
	Headers/HeadersUtils.cpp \
	Handlers/handlers_utils.cpp \
	Handlers/GetHandler.cpp \
	Handlers/PostHandler.cpp \
	Handlers/cgiHandler.cpp  \
	Handlers/DeleteHandler.cpp

OBJS = $(patsubst %.cpp, %.o, $(SRC))

## COLORS ##
END = \033[0m
RED = \033[1;31m
GREEN = \033[1;32m
YELLOW = \033[1;33m
BLUE = \033[1;34m
WHITE = \033[1;37m

## RULES ##
all: $(NAME)

%.o: %.cpp
	@$(COMP) -c -o $@  $<

$(NAME): $(OBJS)
	@$(COMP) $(OBJS) -o $(NAME)
	@echo "$(GREEN)You Created $(NAME)$(END)"

clean:
	@$(RM) -r $(OBJS)
	@echo "$(GREEN)$(NAME): $(RED) → $(YELLOW) the files(*.o) were deleted $(END)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(GREEN)$(NAME): $(RED) → $(BLUE)was deleted$(END)"

re: fclean all

.PHONY: all re clean fclean
