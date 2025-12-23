NAME = woody_woodpacker

CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude

SRC = src/main.c src/elf_parser.c src/rc4.c src/packer.c
OBJ = $(SRC:.c=.o)

STUB_SRC = stub/stub.c
STUB_OBJ = stub/stub.o
STUB_BIN = stub/stub.bin

# Compilar stub independiente
$(STUB_OBJ): $(STUB_SRC)
	$(CC) -c -fPIC -nostdlib -fno-stack-protector $(STUB_SRC) -o $(STUB_OBJ)

# Extraer solo el código (sin headers ELF)
$(STUB_BIN): $(STUB_OBJ)
	objcopy -O binary -j .text $(STUB_OBJ) $(STUB_BIN)

all: $(NAME) $(STUB_BIN)

$(NAME): $(OBJ)
	$(CC) $(OBJ) -o $(NAME)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME) woody

re: fclean all