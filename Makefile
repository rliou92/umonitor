TARGET = umonitor
LIBS = -lX11 -lxcb-randr -lxcb -lconfig
CC = gcc

ifeq ($(DEBUG),1)
CFLAGS = -Wall -g
else
CFLAGS = 
endif

.PHONY: default all clean install

default: directories $(TARGET) 
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
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $(TARGETDIR)/$@

install:
	@install -m755 $(TARGETDIR)/$(TARGET) "/usr/bin"
	@install -m644 "umonitor.service" "/usr/lib/systemd/user"
	@install -m644 "udev_trigger.service" "/usr/lib/systemd/system"

uninstall:
	@rm -f "/usr/bin/$(TARGET)"
	@rm -f "/usr/lib/systemd/user/umonitor.service"
	@rm -f "/usr/lib/systemd/system/udev_trigger.service"

clean:
	-rm -f $(OBJDIR)/*.o
	-rm -f $(TARGETDIR)/$(TARGET)
