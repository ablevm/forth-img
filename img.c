#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

extern char *__progname;

void
usage() {
	fprintf(stderr, "usage: %s [-hvf] -c size file\n", __progname);
	fprintf(stderr, "       %s [-hv] -r size file\n", __progname);
	fprintf(stderr, "       %s [-hv] [-s addr] -i size file < data\n", __progname);
	fprintf(stderr, "       %s [-hv] [-s addr] -o size file > data\n", __progname);
	exit(1);
}

long
eatoi(const char *s) {
	long v;
	char *ep;
	v = strtol(s, &ep, 10);
	switch (ep[0]) {
		case 'g':
		case 'G':
			v *= 1024;
		case 'm':
		case 'M':
			v *= 1024;
		case 'b':
		case 'B':
		case 'k':
		case 'K':
			v *= 1024;
	}
	return v;
}

int
main(int argc, char *argv[]) {
	int vopt;
	vopt = 0;
	bool fopt;
	fopt = false;
	bool sopt;
	sopt = false;
	uintptr_t soptarg;
	soptarg = 0;
	char mopt;
	mopt = '\0';
	size_t moptarg;
	moptarg = 0;

	char ch;
	while ((ch = getopt(argc, argv, "hvfs:c:r:i:o:")) != -1) {
		switch (ch) {
			case 'v':
				vopt++;
				break;
			case 'f':
				if (fopt)
					usage();
				fopt = true;
				break;
			case 's':
				if (sopt)
					usage();
				sopt = true;
				soptarg = eatoi(optarg);
				break;
			case 'c':
			case 'r':
			case 'i':
			case 'o':
				if (mopt != '\0')
					usage();
				mopt = ch;
				moptarg = eatoi(optarg);
				break;
			case 'h':
			default:
				usage();
		}
	}

	if (mopt == '\0')
		usage();

	switch (mopt) {
		case 'c':
			if (sopt)
				usage();
			break;
		case 'r':
			if (fopt || sopt)
				usage();
			break;
		case 'i':
		case 'o':
			if (fopt)
				usage();
			break;
	}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	char *ifn;
	ifn = argv[0];

	switch (mopt) {
		case 'c':
		case 'r': {
			int ifd;
			if (mopt == 'c') {
				if (access(ifn, F_OK) == 0) {
					if (!fopt)
						errx(2, "%s: File exists", ifn);
					int y;
					y = unlink(ifn);
					if (y != 0)
						err(2, "%s", ifn);
				}
				ifd = open(ifn, O_WRONLY|O_CREAT, 0600);
			} else {
				if (access(ifn, F_OK) != 0)
					errx(2, "%s: File not found", ifn);
				ifd = open(ifn, O_WRONLY);
			}
			if (ifd == -1)
				err(2, "%s", ifn);

			int y;
			y = ftruncate(ifd, moptarg);
			if (y == -1)
				err(3, NULL);
			break;
		}
		case 'i':
		case 'o': {
			struct stat ifs;
			int y;
			y = stat(ifn, &ifs);
			if (y == -1)
				err(2, "%s", ifn);
			size_t mc;
			mc = ifs.st_size;

			int ifd;
			ifd = open(ifn, O_RDWR);
			if (ifd == -1)
				errx(2, "%s: File not found", ifn);
			char *m;
			m = mmap(NULL, mc, PROT_READ|PROT_WRITE, MAP_SHARED, ifd, 0);
			if (m == MAP_FAILED)
				err(3, NULL);
			close(ifd);

			uintptr_t a;
			a = soptarg;
			size_t dc;
			dc = moptarg;
			if (a > UINTPTR_MAX - dc || a + dc > mc)
				errx(4, "Address out of range");

			if (mopt == 'i') {
				size_t n;
				n = fread(m + a, sizeof(char), dc, stdin);
				if (vopt >= 1)
					fprintf(stderr, "Wrote %zu bytes of %zu bytes\n", n, dc);
			} else {
				size_t n;
				n = fwrite(m + a, sizeof(char), dc, stdout);
				if (vopt >= 1)
					fprintf(stderr, "Read %zu bytes of %zu bytes\n", n, dc);
			}

			munmap(m, mc);
			break;
		}
	}
}
