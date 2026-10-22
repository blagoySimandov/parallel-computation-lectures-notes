LABS := $(patsubst %/Makefile,%,$(filter-out template/Makefile,$(wildcard */Makefile)))

.PHONY: new all clean list

new:
	@test -n "$(L)" || { echo "usage: make new L=<dir>" >&2; exit 1; }
	@test ! -e "$(L)" || { echo "$(L) already exists" >&2; exit 1; }
	cp -R template "$(L)"

all:
	@for d in $(LABS); do $(MAKE) -C $$d all || exit 1; done

clean:
	@for d in $(LABS); do $(MAKE) -C $$d clean; done

list:
	@for d in $(LABS); do printf '%s: ' $$d; $(MAKE) -s -C $$d list; done
