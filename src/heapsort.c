

/*
 *  heapsort.c
 *
 *
 */
typedef int (*__comp_func)(const void *, const void *);
typedef void (*__cpy1_func)(unsigned long, unsigned long);
typedef int (*__comp1_func)(unsigned long, unsigned long);

void heapsort(void *, unsigned long, unsigned long, __comp_func);
void _heapsort(unsigned long, __comp1_func, __cpy1_func);

static char *hBase;
static unsigned long hSize;
static __comp_func hCompare;

static int std_compare(unsigned long, unsigned long);
static void std_copy(unsigned long, unsigned long);

static void heapsort1(unsigned long);
static __comp1_func h1Compare;
static __cpy1_func h1Copy;


/* ---------- standard heapsort ---------- */


void heapsort(void *base, unsigned long n, unsigned long size, __comp_func compare)
{
        hBase = base;
        hSize = size;
        hCompare = compare;
        _heapsort(n, std_compare, std_copy);
}


static int std_compare(unsigned long i, unsigned long j)
{
        return((*hCompare)(hBase + i * hSize, hBase + j * hSize));
}

static void std_copy(unsigned long i, unsigned long j)
{
        memcpy (hBase + i * hSize, hBase + j * hSize, hSize);
}

/* ---------- general heapsort ---------- */


void _heapsort(unsigned long n, __comp1_func compare, __cpy1_func cpy)
{
        h1Compare = compare;
        h1Copy = cpy;
        heapsort1(n);
}


/*
 *  sort elements 
 *
 */

static void heapsort1(unsigned long n)
{
        register unsigned long l,j,ir,i;               
 
        l=(n >> 1)+1;
        ir=n;
        for (;;) {
                if (l > 1)
                        (*h1Copy)(n, (--l-1));
                else {
                        (*h1Copy)(n, (ir-1));
                        (*h1Copy)((ir-1), 0);
                        if (--ir == 1) {
                        (*h1Copy)(0,n);
                                return;
                        }
                }
                i=l;
                j=l << 1;
                while (j <= ir) {
                        if (j < ir && (*h1Compare)(j-1, j) < 0) ++j;
                        if ((*h1Compare)(n, j-1) < 0) {
                                (*h1Copy)(i-1, (j-1));
                                j += (i=j);
                        }
                        else j=ir+1;
                }
                (*h1Copy)(i-1, n);
        }
 }

