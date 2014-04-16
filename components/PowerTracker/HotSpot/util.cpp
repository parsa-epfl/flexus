#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <strings.h>
#include <cassert>

#include "util.h"

int32_t eq(float x, float y) {
  return (fabs(x - y) <  DELTA);
}

int32_t le(float x, float y) {
  return ((x < y) || eq(x, y));
}

int32_t ge(float x, float y) {
  return ((x > y) || eq(x, y));
}

void fatal(char * s) {
  fprintf(stderr, "error: %s", s);
  exit(1);
}

void warning(char * s) {
  fprintf(stderr, "warning: %s", s);
}

void swap_ival (int32_t * a, int32_t * b) {
  int32_t t = *a;
  *a = *b;
  *b = t;
}

void swap_dval (double * a, double * b) {
  double t = *a;
  *a = *b;
  *b = t;
}

int32_t tolerant_ceil(double val) {
  double nearest = rint(val);
  /* numbers close to integers	*/
  if (eq(val, nearest))
    return ((int) nearest);
  /* all others	*/
  else
    return ((int) ceil(val));
}

int32_t tolerant_floor(double val) {
  double nearest = rint(val);
  /* numbers close to integers	*/
  if (eq(val, nearest))
    return ((int) nearest);
  /* all others	*/
  else
    return ((int) floor(val));
}

double * dvector(int32_t n) {
  double * v;

  v = (double *)calloc(n, sizeof(double));
  if (!v) fatal("allocation failure in dvector()\n");

  return v;
}

void free_dvector(double * v) {
  free(v);
}

void dump_dvector (double * v, int32_t n) {
  int32_t i;
  for (i = 0; i < n; i++)
    fprintf(stdout, "%.5f\t", v[i]);
  fprintf(stdout, "\n");
}

void copy_dvector (double * dst, double * src, int32_t n) {
  memmove(dst, src, sizeof(double) * n);
}

void zero_dvector (double * v, int32_t n) {
  memset(v, 0, sizeof(double) * n);
}

/* sum of the elements	*/
double sum_dvector (double * v, int32_t n) {
  double sum = 0;
  int32_t i;
  for (i = 0; i < n; i++)
    sum += v[i];
  return sum;
}

int32_t * ivector(int32_t n) {
  int32_t * v;

  v = (int32_t *)calloc(n, sizeof(int));
  if (!v) fatal("allocation failure in ivector()\n");

  return v;
}

void free_ivector(int32_t * v) {
  free(v);
}

void dump_ivector (int32_t * v, int32_t n) {
  int32_t i;
  for (i = 0; i < n; i++)
    fprintf(stdout, "%d\t", v[i]);
  fprintf(stdout, "\n\n");
}

void copy_ivector (int32_t * dst, int32_t * src, int32_t n) {
  memmove(dst, src, sizeof(int) * n);
}

void zero_ivector (int32_t * v, int32_t n) {
  memset(v, 0, sizeof(int) * n);
}

/*
 * Thanks to Greg Link from Penn State University
 * for these memory allocators/deallocators
 */
double ** dmatrix(int32_t nr, int32_t nc) {
  int32_t i;
  double ** m;

  m = (double **) calloc (nr, sizeof(double *));
  assert(m != NULL);
  m[0] = (double *) calloc (nr * nc, sizeof(double));
  assert(m[0] != NULL);

  for (i = 1; i < nr; i++)
    m[i] =  m[0] + nc * i;

  return m;
}

void free_dmatrix(double ** m) {
  free(m[0]);
  free(m);
}

int32_t ** imatrix(int32_t nr, int32_t nc) {
  int32_t i;
  int32_t ** m;

  m = (int32_t **) calloc (nr, sizeof(int32_t *));
  assert(m != NULL);
  m[0] = (int32_t *) calloc (nr * nc, sizeof(int));
  assert(m[0] != NULL);

  for (i = 1; i < nr; i++)
    m[i] = m[0] + nc * i;

  return m;
}

void free_imatrix(int32_t ** m) {
  free(m[0]);
  free(m);
}

void dump_dmatrix (double ** m, int32_t nr, int32_t nc) {
  int32_t i;
  for (i = 0; i < nr; i++)
    dump_dvector(m[i], nc);
  fprintf(stdout, "\n");
}

void copy_dmatrix (double ** dst, double ** src, int32_t nr, int32_t nc) {
  memmove(dst[0], src[0], sizeof(double) * nr * nc);
}

void zero_dmatrix(double ** m, int32_t nr, int32_t nc) {
  memset(m[0], 0, sizeof(double) * nr * nc);
}

void resize_dmatrix(double ** m, int32_t nr, int32_t nc) {
  int32_t i;
  for (i = 1; i < nr; i++)
    m[i] = m[0] + nc * i;
}

/* allocate 3-d matrix with 'nr' rows, 'nc' cols,
 * 'nl' layers	and a tail of 'xtra' elements
 */
double ** *dcuboid_tail(int32_t nr, int32_t nc, int32_t nl, int32_t xtra) {
  int32_t i, j;
  double ** *m;

  /* 1-d array of pointers to the rows of the 2-d array below	*/
  m = (double ** *) calloc (nl, sizeof(double **));
  assert(m != NULL);
  /* 2-d array of pointers denoting (layer, row)	*/
  m[0] = (double **) calloc (nl * nr, sizeof(double *));
  assert(m[0] != NULL);
  /* the actual 3-d data array	*/
  m[0][0] = (double *) calloc (nl * nr * nc + xtra, sizeof(double));
  assert(m[0][0] != NULL);

  /* remaining pointers of the 1-d pointer array	*/
  for (i = 1; i < nl; i++)
    m[i] =  m[0] + nr * i;

  /* remaining pointers of the 2-d pointer array	*/
  for (i = 0; i < nl; i++)
    for (j = 0; j < nr; j++)
      /* to reach the jth row in the ith layer,
       * one has to cross i layers i.e., i*(nr*nc)
       * values first and then j rows i.e., j*nc
       * values next
       */
      m[i][j] =  m[0][0] + (nr * nc) * i + nc * j;

  return m;
}

void free_dcuboid(double ** *m) {
  free(m[0][0]);
  free(m[0]);
  free(m);
}

/* mirror the lower triangle to make 'm' fully symmetric	*/
void mirror_dmatrix(double ** m, int32_t n) {
  int32_t i, j;
  for (i = 0; i < n; i++)
    for (j = 0; j < i; j++)
      m[j][i] = m[i][j];
}

void dump_imatrix (int32_t ** m, int32_t nr, int32_t nc) {
  int32_t i;
  for (i = 0; i < nr; i++)
    dump_ivector(m[i], nc);
  fprintf(stdout, "\n");
}

void copy_imatrix (int32_t ** dst, int32_t ** src, int32_t nr, int32_t nc) {
  memmove(dst[0], src[0], sizeof(int) * nr * nc);
}

void resize_imatrix(int32_t ** m, int32_t nr, int32_t nc) {
  int32_t i;
  for (i = 1; i < nr; i++)
    m[i] = m[0] + nc * i;
}

/* initialize random number generator	*/
void init_rand(void) {
  srand(RAND_SEED);
}

/* random number within the range [0, max-1]	*/
int32_t rand_upto(int32_t max) {
  return (int) (max * (double) rand() / (RAND_MAX + 1.0));
}

/* random number in the range [0, 1)	*/
double rand_fraction(void) {
  return ((double) rand() / (RAND_MAX + 1.0));
}

/*
 * reads tab-separated name-value pairs from file into
 * a table of size max_entries and returns the number
 * of entries read successfully
 */
int32_t read_str_pairs(str_pair * table, int32_t max_entries, char * file) {
  int32_t i = 0;
  char str[LINE_SIZE], copy[LINE_SIZE];
  char name[STR_SIZE];
  char * ptr;
  FILE * fp = fopen (file, "r");
  if (!fp) {
    sprintf (str, "error: %s could not be opened for reading\n", file);
    fatal(str);
  }
  while (i < max_entries) {
    fgets(str, LINE_SIZE, fp);
    if (feof(fp))
      break;
    strcpy(copy, str);

    /* ignore comments and empty lines  */
    ptr = strtok(str, " \r\t\n");
    if (!ptr || ptr[0] == '#')
      continue;

    if ((sscanf(copy, "%s%s", name, table[i].value) != 2) || (name[0] != '-'))
      fatal("invalid file format\n");
    /* ignore the leading "-"	*/
    strcpy(table[i].name, &name[1]);
    i++;
  }
  fclose(fp);
  return i;
}

/*
 * same as above but from command line instead of a file. the command
 * line is of the form <prog_name> <name-value pairs> where
 * <name-value pairs> is of the form -<variable> <value>
 */
int32_t parse_cmdline(str_pair * table, int32_t max_entries, int32_t argc, char ** argv) {
  int32_t i, count;
  for (i = 1, count = 0; i < argc && count < max_entries; i++) {
    if (i % 2) {	/* variable name	*/
      if (argv[i][0] != '-')
        fatal("invalid command line. check usage\n");
      /* ignore the leading "-"	*/
      strncpy(table[count].name, &argv[i][1], STR_SIZE - 1);
      table[count].name[STR_SIZE-1] = '\0';
    } else {		/* value	*/
      strncpy(table[count].value, argv[i], STR_SIZE - 1);
      table[count].value[STR_SIZE-1] = '\0';
      count++;
    }
  }
  return count;
}

/* append the table onto a file	*/
void dump_str_pairs(str_pair * table, int32_t size, char * file, char * prefix) {
  int32_t i;
  char str[STR_SIZE];
  FILE * fp = fopen (file, "w");
  if (!fp) {
    sprintf (str, "error: %s could not be opened for writing\n", file);
    fatal(str);
  }
  for (i = 0; i < size; i++)
    fprintf(fp, "%s%s\t%s\n", prefix, table[i].name, table[i].value);
  fclose(fp);
}

/* table lookup	for a name */
int32_t get_str_index(str_pair * table, int32_t size, char * str) {
  int32_t i;

  if (!table)
    fatal("null pointer in get_str_index\n");

  for (i = 0; i < size; i++)
    if (!strcasecmp(str, table[i].name))
      return i;
  return -1;
}

/* delete entry at 'at'	*/
void delete_entry(str_pair * table, int32_t size, int32_t at) {
  int32_t i;
  /*
   * overwrite this entry using the next and
   * shift all later entries once
   */
  for (i = at + 1; i < size; i++) {
    strcpy(table[i-1].name, table[i].name);
    strcpy(table[i-1].value, table[i].value);
  }
}

/*
 * remove duplicate names in the table - the entries later
 * in the table are discarded. returns the new size of the
 * table
 */
int32_t str_pairs_remove_duplicates(str_pair * table, int32_t size) {
  int32_t i, j;

  for (i = 0; i < size - 1; i++)
    for (j = i + 1; j < size; j++)
      if (!strcasecmp(table[i].name, table[j].name)) {
        delete_entry(table, size, j);
        size--;
        j--;
      }
  return size;
}

/* debug	*/
void print_str_pairs(str_pair * table, int32_t size) {
  int32_t i;
  fprintf(stdout, "printing string table\n");
  for (i = 0; i < size; i++)
    fprintf(stdout, "%s\t%s\n", table[i].name, table[i].value);
}

/*
 * binary search a sorted double array 'arr' of size 'n'. if found,
 * the 'loc' pointer has the address of 'ele' and the return
 * value is TRUE. otherwise, the return value is FALSE and 'loc'
 * points to the 'should have been' location
 */
int32_t bsearch_double(double * arr, int32_t n, double ele, double ** loc) {
  if (n < 0)
    fatal("wrong index in binary search\n");

  if (n == 0) {
    *loc = arr;
    return FALSE;
  }

  if (eq(arr[n/2], ele)) {
    *loc = &arr[n/2];
    return TRUE;
  } else if (arr[n/2] < ele) {
    return bsearch_double(&arr[n/2+1], (n - 1) / 2, ele, loc);
  } else
    return bsearch_double(arr, n / 2, ele, loc);

}

/*
 * binary search and insert an element into a partially sorted
 * double array if not already present. returns FALSE if present
 */
int32_t bsearch_insert_double(double * arr, int32_t n, double ele) {
  double * loc;
  int32_t i;

  /* element found - nothing more left to do	*/
  if (bsearch_double(arr, n, ele, &loc))
    return FALSE;
  else {
    for (i = n - 1; i >= (loc - arr); i--)
      arr[i+1] = arr[i];
    arr[loc-arr] = ele;
  }
  return TRUE;
}

/*
 * population count of an 8-bit integer - using pointers from
 * http://aggregate.org/MAGIC/
 */
uint32_t ones8(register uint8_t n) {
  /* group the bits in two and compute the no. of 1's within a group
   * this works because 00->00, 01->01, 10->01, 11->10 or
   * n = n - (n >> 1). the 0x55 masking prevents bits flowing across
   * group boundary
   */
  n -= ((n >> 1) & 0x55);
  /* add the 2-bit sums into nibbles */
  n = ((n & 0x33) + ((n >> 2) & 0x33));
  /* add both the nibbles */
  n = ((n + (n >> 4)) & 0x0F);
  return n;
}

/*
 * find the number of non-empty, non-comment lines
 * in a file open for reading
 */
int32_t count_significant_lines(FILE * fp) {
  char str[LINE_SIZE], *ptr;
  int32_t count = 0;

  fseek(fp, 0, SEEK_SET);
  while (!feof(fp)) {
    fgets(str, LINE_SIZE, fp);
    if (feof(fp))
      break;

    /* ignore comments and empty lines	*/
    ptr = strtok(str, " \r\t\n");
    if (!ptr || ptr[0] == '#')
      continue;

    count++;
  }
  return count;
}

