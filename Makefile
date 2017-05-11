TARGET = umonitor
LIBS = -lX11 -lxcb-randr -lxcb -lconfig
CC = gcc
CFLAGS = -Wall -g

.PHONY: default all clean

default: $(TARGET) directories
all: default

OBJDIR = obj
SRCDIR = src
TARGETDIR = bin

OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.c))
HEADERS = $(wildcard $(SRCDIR)/*.h)

directories:
	@mkdir -p $(TARGETDIR)
	@mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGETDIR)/$(TARGET) $(OBJDIR)/$(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $(TARGETDIR)/$@

clean:
	-rm -f $(OBJDIR)/*.o
	-rm -f $(TARGETDIR)/$(TARGET)
