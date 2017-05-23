TARGET = umonitor
LIBS = -lX11 -lxcb-randr -lxcb -lconfig
CC = gcc
BIN_DIR = /usr/bin
SYSTEMD_DIR = /usr/lib/systemd
SYSTEMD_USER_DIR = $(SYSTEMD_DIR)/user
SYSTEMD_SYSTEM_DIR = $(SYSTEMD_DIR)/system
LICENSE_DIR = /usr/share/licenses/$(TARGET)

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
	install -d $(DESTDIR)$(BIN_DIR)
	install -p -m755 $(TARGETDIR)/$(TARGET) $(DESTDIR)$(BIN_DIR)
	install -d $(DESTDIR)$(SYSTEMD_USER_DIR)
	install -p -m644 "umonitor.service" $(DESTDIR)$(SYSTEMD_USER_DIR)
	install -d $(DESTDIR)$(SYSTEMD_SYSTEM_DIR)
	install -p -m644 "udev_trigger.service" $(DESTDIR)$(SYSTEMD_SYSTEM_DIR)
	install -d $(DESTDIR)$(LICENSE_DIR)
	install -p -m644 "LICENSE" $(DESTDIR)$(LICENSE_DIR)

uninstall:
	rm -f $(DESTDIR)$(BIN_DIR)/$(TARGET)
	rm -f "$(DESTDIR)$(SYSTEMD_USER_DIR)/umonitor.service"
	rm -f "$(DESTDIR)$(SYSTEMD_SYSTEM_DIR)/udev_trigger.service"

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(TARGETDIR)/$(TARGET)
