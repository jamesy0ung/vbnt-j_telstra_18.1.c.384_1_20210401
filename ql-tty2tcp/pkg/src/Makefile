EXECUTABLE   = ql-tty2tcp
SOURCE_FILES = ql-tty2tcp.c
LDFLAGS     := -pthread $(LDFLAGS)

.PHONY: all
all: $(EXECUTABLE)

.PHONY: clean
clean:
	rm *.o *~

$(EXECUTABLE): $(SOURCE_FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@
	$(STRIP) $@
