#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <err.h>

extern char *__progname;

void
usage() {
	fprintf(stderr, "usage  %s [-hv]\n", __progname);
	fprintf(stderr, "       %s [-hv] -c file size\n", __progname);
	fprintf(stderr, "       %s [-hv] -t file size\n", __progname);
	fprintf(stderr, "       %s [-hv] -r file addr size > data\n", __progname);
	fprintf(stderr, "       %s [-hv] -w file addr size < data\n", __progname);
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
	char mopt;
	mopt = 0;
	int topt;
	topt = 0;
	char *ifn;
	ifn = NULL;
	size_t ms;
	ms = 0;
	intptr_t a;
	a = 0;
	size_t ds;
	ds = 0;
		
	char ch;
	while ((ch = getopt(argc, argv, "hvctrw")) != -1) {
		switch (ch) {
			case 'v':
				vopt++;
				break;
			case 'c':
			case 't':
			case 'r':
			case 'w':
				if (mopt != 0)
					usage();
				mopt = ch;
				break;
			case 'h':
			default:
				usage();
		}
	}

	if (mopt == 0)
		usage();
	
	argc -= optind;
	argv += optind;

	switch (mopt) {
		case 'c':
		case 't':
			if (argc != 2)
				usage();
			ifn = argv[0];
			ms = eatoi(argv[1]);
			break;
		case 'r':
		case 'w':
			if (argc != 3)
				usage();
			ifn = argv[0];
			int rc;
			struct stat ifs;
			rc = stat(ifn, &ifs);
			if (rc == -1)
				err(2, "%s", ifn);
			ms = ifs.st_size;
			a = eatoi(argv[1]);
			ds = eatoi(argv[2]);
	}
	
	int ifd;
	ifd = open(ifn, O_RDWR);
	if (ifd == -1) {
		if (mopt == 'c')
			ifd = open(ifn, O_RDWR|O_CREAT, 0600);
		if (ifd == -1)
			err(2, "%s", ifn);
		int rc;
		rc = ftruncate(ifd, ms);
		if (rc == -1)
			err(3, NULL);
		if (mopt == 'c')
			exit(0);
	} else {
		if (mopt == 'c')
			errx(2, "%s: File exists", ifn);
		if (mopt == 't') {
			int rc;
			rc = ftruncate(ifd, ms);
			if (rc == -1)
				err(3, NULL);
			exit(0);
		}
	}

	if (a + ds > ms)
		errx(4, "Address out of range");
	
	if (ds == 0)
		exit(0);
		
	char *e;
	e = mmap(NULL, ms, PROT_READ|PROT_WRITE, MAP_SHARED, ifd, 0);
	if (e == MAP_FAILED)
		exit(2);
	close(ifd);

	switch (mopt) {
		case 'r': {
			size_t n;
			n = fwrite(e + a, sizeof(char), ds, stdout);
			if (vopt >= 1)
				fprintf(stderr, "Read %zu bytes of %zu bytes\n", n, ds);
			break;
		}
		case 'w': {
			size_t n;
			n = fread(e + a, sizeof(char), ds, stdin);
			if (vopt >= 1)
				fprintf(stderr, "Wrote %zu bytes of %zu bytes\n", n, ds);
			break;
		}
	}

	munmap(e, ms);
}
