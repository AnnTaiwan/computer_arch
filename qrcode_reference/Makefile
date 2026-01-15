SRC = qrcode.c
OBJ = $(patsubst %.c, %.o, $(SRC))
EXE = $(patsubst %.c, %, $(SRC))
CMD = gcc

all: $(EXE)

# Link object files to create executable
$(EXE): $(OBJ)
	$(CMD) -o $@ $^

# Compile .c files into .o files
%.o: %.c
	$(CMD) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXE)
