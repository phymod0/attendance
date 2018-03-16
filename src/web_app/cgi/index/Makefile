
PROGRAM = index.cgi

C_FILES = $(wildcard *.c)
O_FILES = $(patsubst %.c,%.o,$(C_FILES))

$(PROGRAM): $(O_FILES)
	$(CC) -o $(PROGRAM) $(O_FILES)
$(O_FILES):
	$(CC) -c $(patsubst %.o,%.c,$@)
clean:
	rm -f *.o $(PROGRAM)
