CC        = gcc
CCFLAGS   = -I.
INCLUDES  = setrate.h
OBJS      = setrate_main.o setrate_alsa.o setrate_lws.o setrate_signal.o setrate_util.o setrate_fsm.o
BIN       = camilladsp-setrate
BINDIR    = ./bin
LDLIBS    = -lasound -lwebsockets
LOCBINDIR = /usr/local/bin
SERVICE   = camilladsp-setrate.service 

%.o: %.c $(INCLUDES)
	$(CC) $(CCFLAGS) -c $< -o $@

$(BINDIR)/$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

install:
	sudo cp $(BINDIR)/$(BIN) $(LOCBINDIR)

uninstall:
	sudo rm -f $(LOCBINDIR)/$(BIN)

clean:
	rm -f $(OBJS)
