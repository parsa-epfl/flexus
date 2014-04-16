#include <stdio.h>
#include <string.h>

#define MEM_MAX	1024		/* size of microcode */

int map[MEM_MAX] = {0};
unsigned long *cnt;
char *ids;

FILE *src_fp, *cnt_fp, *rom_fp;

main (argc, argv)
	int argc;
	char *argv[];
{
	int i, j, k;
	unsigned long x;
	char buf[256];

	if (argc != 4 || !(src_fp = fopen(argv[1], "r"))  ||
		!(cnt_fp = fopen(argv[2], "r"))  ||
		!(rom_fp = fopen(argv[3], "r")) ) {
		fprintf (stderr, "anno <source-file> <counter-file> <rom-file>\n");
		exit(1);
	}

	/*
	 * read rom to establish a map
	 */
	fgets (buf, 250, rom_fp);
	ids = strdup(buf);
	k = 0;
	while (fgets (buf, 250, rom_fp)) {
		if (2 != sscanf(buf, "%d%*d%*d%*d%*d%*d ;%d", &i, &j) ||
			i < 0 || i >= MEM_MAX || map[i] || j < 1) {
			fprintf(stderr, "Invalid line in rom file:\n%s\n", buf);
			exit(1);
		}
		map[i] = j;
		if (k < j) k = j;
	}

	/*
	 * Allocate source array
	 */
	cnt = (unsigned long *) calloc (k, sizeof(unsigned long));

	/*
	 * read counters
	 */
	fgets (buf, 250, cnt_fp);
	if (strcmp(ids, buf)) {
		fprintf (stderr, "Rom and counters don't match\n");
		exit(1);
	}
	for (i = 0; i < MEM_MAX; i++) {
		if (!fgets (buf, 250, cnt_fp) || 1 != sscanf(buf, "%lu", &x)) {
			fprintf(stderr, "counter file problem\n", buf);
			exit(1);
		}
		if (x && 0 == map[i]) {
			fprintf(stderr, "None-zero counter for unused rom location %d - ignored\n", i);
		} else
			cnt[map[i] - 1] += x;
	}

	for (i = 0; fgets(buf, 250, src_fp); i++) {
		x = 0;
		if (i < k) x = cnt[i];
		if (x > 0)
			printf ("%6u\t%s", x, buf);
		else
			printf ("\t%s", buf);
	}

	exit(0);
}
