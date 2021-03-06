include config.mk

-include $(DEPS)

all: $(TARGET)
debug: $(DTARGET)

options:
	@echo "$(PROJECT) build options:"
	@echo "LIBS    = $(LIBS)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "CC      = $(CC)"

src/hints.o: src/hints.js.h
src/hints.do: src/hints.js.h

src/hints.js.h: src/hints.js
	@echo "minify $<"
	@cat $< | src/js2h.sh > $@

$(OBJ): src/config.h config.mk
$(DOBJ): src/config.h config.mk

$(TARGET): $(OBJ)
	@echo "$(CC) $@"
	@$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

$(DTARGET): $(DOBJ)
	@echo "$(CC) $@"
	@$(CC) $(DFLAGS) $(DOBJ) -o $(DTARGET) $(DLDFLAGS)

src/config.h:
	@echo create $@ from src/config.def.h
	@cp src/config.def.h $@

%.o: %.c %.h
	@echo "${CC} $<"
	@$(CC) -c -o $@ $< $(CFLAGS)

%.do: %.c %.h
	@echo "${CC} $<"
	@$(CC) -c -o $@ $< $(DFLAGS)

install: $(TARGET) doc/$(MAN1)
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(MANDIR1)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	@echo "install -m 644 src/$(MAN1) $(DESTDIR)$(MANDIR1)/$(MAN1)"
	@sed -e "s/VERSION/$(VERSION)/g" \
		-e "s/DATE/`date +'%m %Y'`/g" < doc/$(MAN1) > $(DESTDIR)$(MANDIR1)/$(MAN1)
	@chmod 644 $(DESTDIR)$(MANDIR1)/$(MAN1)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(TARGET)
	$(RM) $(DESTDIR)$(MANDIR1)/$(MAN1)

clean:
	$(RM) src/*.o src/*.do src/hints.js.h $(TARGET) $(DTARGET)

dist: distclean
	@echo "Creating tarball."
	@git archive --format tar -o $(DIST_FILE) HEAD

distclean:
	$(RM) $(DIST_FILE)

.PHONY: clean debug all install uninstall options dist
