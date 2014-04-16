#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <ppm.h>

#include <assert.h>
#include <pthread.h>

#include "flexus_utils.h"

#define NTRIALS 3
int trial = 0;

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)>(b)?(b):(a))

typedef struct {
	int       width;
	int       height;
	double ** filter;
} filter_t;

// GLOBALS
int        n_threads = 0;
int *      thread_ids;
filter_t   filt;
pixel **   in_img;
int        width;
int        height;
pixel **   out_img;
pixval *   maxval;

// silly barrier implementation, better use asm volatile(...)
typedef struct t_barrier {
  pthread_mutex_t mut;
  volatile int reached;
  volatile unsigned long bitmask;
} t_barrier;
t_barrier global_barrier;

void barrier_init(void) {
  pthread_mutex_init(&global_barrier.mut, NULL);
  global_barrier.reached = 0;
  global_barrier.bitmask = 0;
}

void barrier(int myid)
{
  pthread_mutex_lock(&global_barrier.mut);
  global_barrier.reached ++;
  if (global_barrier.reached < n_threads) {
    global_barrier.bitmask |= (1<<myid);
    pthread_mutex_unlock(&global_barrier.mut);
    while (global_barrier.bitmask & (1<<myid)) ;
  }
  else {
    global_barrier.reached = 0;
    global_barrier.bitmask = 0;
    pthread_mutex_unlock(&global_barrier.mut);
  }
}


void * filter(void * an_id)
{
	int w,h;
	int fw,fh;
	double r,g,b;
	pixval newmaxval = 0;
    int myid;
    int my_img_hstart;
    int my_img_hend;
    int h_blk, w_blk;
    int L2_h_blk, L2_w_blk;

    // get my thread id
    myid = *(int *)an_id;

    // block input image and take care of boundary conditions
    my_img_hstart = myid * height/n_threads;
    my_img_hend = my_img_hstart + height/n_threads + filt.height-1;
    my_img_hend = MIN(height, my_img_hend);

    // fprintf(stderr, "thread %d/%d started: img=%d x %d,  my copy is %d x %d [%d..%d x %d..%d]\n",
    //         myid, n_threads, height, width, height/n_threads, width, my_img_hstart+filt.height/2, my_img_hend-filt.height/2 , 0, width);

    barrier(myid); // all threads should sync here to get a correct parallel execution measurement

    ///////////////// SIGNAL FLEXUS: entering new iteration
    flexBkp_Iteration();
    FLEXPRINT_MARKER(my_id, trial);


#define L1_BLOCKING  32
#define L2_BLOCKING  256

    for(L2_h_blk = 0; L2_h_blk < height/L2_BLOCKING+1; ++L2_h_blk) {
      for(L2_w_blk = 0; L2_w_blk < width/L2_BLOCKING+1; ++L2_w_blk) {
        for(h_blk = L2_h_blk*L2_BLOCKING/L1_BLOCKING; h_blk < MIN((L2_h_blk+1)*L2_BLOCKING/L1_BLOCKING, height/L1_BLOCKING+1); ++h_blk) {
          for(w_blk = L2_w_blk*L2_BLOCKING/L1_BLOCKING; w_blk < MIN((L2_w_blk+1)*L2_BLOCKING/L1_BLOCKING, width/L1_BLOCKING+1); ++w_blk) {
            for(h = MAX(h_blk*L1_BLOCKING, my_img_hstart+filt.height/2); h < MIN((h_blk+1)*L1_BLOCKING, my_img_hend-filt.height/2); ++h) {
              for(w = MAX(filt.width/2, w_blk*L1_BLOCKING); w < MIN((w_blk+1)*L1_BLOCKING, width-filt.width/2); ++w) {

                r = 0.; g = 0.; b = 0.;
                for(fh = -filt.height/2; fh <= filt.height/2; ++fh) {
                  for(fw = -filt.width/2; fw <= filt.width/2; ++fw) {
                    double scale = filt.filter[fh+filt.height/2][fw+filt.width/2];
                    r += PPM_GETR(in_img[h+fh][w+fw])*scale;
                    g += PPM_GETG(in_img[h+fh][w+fw])*scale;
                    b += PPM_GETB(in_img[h+fh][w+fw])*scale;
                  }
                }
                if (r<0.) r = 0.; if (g<0.) g=0.; if (b<0.) b=0.;
                PPM_ASSIGN(out_img[h][w],r,g,b);

                if (r > newmaxval) newmaxval = r;
                if (g > newmaxval) newmaxval = g;
                if (b > newmaxval) newmaxval = b;
              }
            }
          }
        }
      }
	}
	maxval[myid] = newmaxval;

    barrier(myid); // sync again to make sure maxval has been updated by all before collecting it

    if (myid != 0) {
      pthread_exit(0);
    }

    return NULL;  // compiler happy
}


int main(int argc,char *argv[]) {
	FILE *fp;
	pixval orig_maxval;
    pixel ** out_img_tmp[NTRIALS+1];
	struct timeval start, stop;
	unsigned long usec;
	int tw,th;
    pthread_t * thread_array;
    int i;
    int myid = 0;

	if (((argc != 3) && (argc != 4)) || !(n_threads = atoi(argv[2]))) {
		fprintf(stderr,"Usage: %s <filter.file> <#threads> < in.ppm > out.ppm\n",argv[0]);
		return -1;
	}

	fp = fopen(argv[1],"r");
	if (!fp) {
		fprintf(stderr,"%s: Can't open %s: %s\n",argv[0],argv[1],strerror(errno));
		return -2;
	}
	fscanf(fp,"%d %d",&filt.width,&filt.height);
	filt.filter = (double**)calloc(filt.height,sizeof(double*));
	for(th = 0; th < filt.height; ++th) {
		filt.filter[th] = (double*)calloc(filt.width,sizeof(double));
		for(tw = 0; tw < filt.width; ++tw) {
			fscanf(fp,"%lg",&filt.filter[th][tw]);
		}
	}

	ppm_init(&argc,argv);
	in_img = ppm_readppm(stdin,&width,&height,&orig_maxval);
	if (!in_img) return -3;

    barrier_init();
	usec = 0;

  ///////////////// SIGNAL FLEXUS: entering warmup phase
  FLEXPRINT_MARKER(0, 0);
  flexBkp_StartWarmup();

	for(trial = 0; trial < (NTRIALS+1); ++trial) {

    if (trial == 1) {
      ///////////////// SIGNAL FLEXUS: entering parallel phase for measurement
      flexBkp_StartParallel();
      FLEXPRINT_MARKER(0, 0);
    }


		out_img_tmp[trial] = ppm_allocarray(width,height);
        out_img = out_img_tmp[trial];

        // create threads
        maxval = (pixval*) calloc(n_threads, sizeof(pixval));
        assert(maxval);
        thread_array = (pthread_t*) calloc(n_threads, sizeof(pthread_t));
        assert(thread_array);
        thread_ids = (int*) calloc(n_threads, sizeof(int));
        assert(thread_ids);
        for (i=1; i < n_threads; i++) {
          thread_ids[i]=i;
          assert(pthread_create(&thread_array[i], NULL, filter, &thread_ids[i]) == 0);
        }

        // timing
		gettimeofday(&start, NULL);
		filter(&myid);
		gettimeofday(&stop, NULL);

        // collect threads
        for (i=1; i < n_threads; i++) {
          pthread_join(thread_array[i], NULL);
        }
        for (orig_maxval=maxval[0], i=1; i < n_threads; i++) {
          orig_maxval = MAX(orig_maxval, maxval[i]);
        }

        // free
		if (trial > 0) {
			usec += 1./NTRIALS * (stop.tv_sec - start.tv_sec) * 1e6;
			usec += 1./NTRIALS * (stop.tv_usec - start.tv_usec);
			ppm_freearray(out_img_tmp[trial],height);
		}
        free(maxval);
        free(thread_array);

	}

  flexBkp_EndParallel();
  FLEXPRINT_MARKER(0, 0);

	if (argc==3) {
		ppm_writeppm(stdout,out_img_tmp[0],width,height,orig_maxval,0);
		fprintf(stdout,"\n"); // "convert" adds a newline to the eof
		fprintf(stderr,"%d threads, avg. of %d trials: %.3g sec\n",n_threads,NTRIALS,usec/(double)1e6);
	} else {
		fprintf(stdout,"%.3g ",usec/(double)1e6);
	}

	ppm_freearray(in_img,height);
	ppm_freearray(out_img_tmp[0],height);

	return 0;
}
