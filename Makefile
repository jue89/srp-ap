CC=gcc
INSTALL=install
MKDIR=mkdir -p
CFLAGS=-c -Wall
LDFLAGS=-lcurl -lwjelement -lwjwriter -lwjreader
SOURCES=ctrl.c init.c conf.c exec.c main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=api

OBJECTDIR=obj/
SRCDIR=src/
COBJECTS=$(addprefix $(OBJECTDIR),$(OBJECTS))

.PHONY: all
all: $(EXECUTABLE)

.PHONY: clean
clean:
	$(RM) $(COBJECTS) $(EXECUTABLE)

$(EXECUTABLE): $(COBJECTS)
	$(CC) $(COBJECTS) $(LDFLAGS) -o $@

$(OBJECTDIR)%.o: $(SRCDIR)%.c
	$(CC) $(CFLAGS) $< -o $@
