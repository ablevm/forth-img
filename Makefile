BIN=img

PREFIX?=/usr/local
BINDIR=${PREFIX}/bin

CFLAGS+=-g -Wno-unused-result

.PHONY: build clean install uninstall

build: ${BIN}

clean:
	-rm ${BIN}

install: ${BIN}
	@mkdir -p ${BINDIR}
	cp ${BIN} ${BINDIR}/${BIN}

uninstall:
	rm ${BINDIR}/${BIN}
