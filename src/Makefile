
.PHONY: all clean lib install uninstall install-html

# by default we compile the lib directory, which in turn requests all the needeed modules
all: lib

lib: 
	$(MAKE) -C lib

install:
	$(MAKE) -C lib install

install-html:
	$(MAKE) -C lib install-html

uninstall:
	$(MAKE) -C lib uninstall

# list of all the contained directories which can be cleaned
SUBDIRS :=$(subst /Makefile,,$(wildcard */Makefile))
SUBDIRSCLEAN :=$(addsuffix .clean,$(SUBDIRS))
.PHONY: $(SUBDIRS)

# this is cleaning all subdirectories
clean: $(SUBDIRSCLEAN)
	rm -f cppcheck.log cppcheck.fatal

$(SUBDIRSCLEAN): %.clean:
	$(MAKE) -C $* clean

.PHONY: cppcheck
cppcheck:
	maketools/cppcheck

.PHONY: help
help:
	@echo "Available targets are"
	@echo "  all: compile everything (default)"
	@echo "  clean: clean everything"
	@echo "  cppcheck: check source (requires cppcheck installed)"


