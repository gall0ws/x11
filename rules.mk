all:  $(BINS) $(BIN)

install: $(BINS) $(BIN) $(BINDIR)
	$(STRIP) $(BINS) $(BIN)
	cp $^

clean:
	rm -f $(OFILES) $(BINS) $(BIN)
	
$(BIN): $(OFILES)
	$(LD) $(LDFLAGS) $(LDLIBS) -o $@ $^
	
$(BINDIR):
	mkdir -p $@
	
*.o: $(HFILES)

.PHONY: all install clean
