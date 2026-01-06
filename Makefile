NAME = woody_woodpacker

CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude
NASM = nasm

SRC = src/main.c src/elf_parser.c src/rc4.c src/packer.c
OBJ = $(SRC:.c=.o)

# Stub assembly
STUB_ASM = stub/stub.asm
STUB_OBJ = stub/stub.o
STUB_BIN = stub/stub.bin

all: $(NAME) $(STUB_BIN)

$(NAME): $(OBJ)
	$(CC) $(OBJ) -o $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar stub assembly
$(STUB_OBJ): $(STUB_ASM)
	$(NASM) -f elf64 $(STUB_ASM) -o $(STUB_OBJ)

$(STUB_BIN): $(STUB_OBJ)
	objcopy -O binary $(STUB_OBJ) $(STUB_BIN)
	@echo "Stub compiled: $(STUB_BIN) ($$(wc -c < $(STUB_BIN)) bytes)"

clean:
	rm -f $(OBJ) $(STUB_OBJ)

fclean: clean
	rm -f $(NAME) woody $(STUB_BIN)

re: fclean all

.PHONY: all clean fclean re