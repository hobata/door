/* 
  Transport Door Chime Program 

  sudo apt-get install libasound2-dev libfftw3-dev libsndfile1-dev
  gcc -o door -lasound -lm -lfftw3 -lsndfile main.c

  arecord -l
  alsamixer

  ./door hw:1

*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <alsa/asoundlib.h>
#include <complex.h>
#include <fftw3.h>
#include <sndfile.h>

//#define GET_PEAK // search peak data: ./door hw:1 > out.txt

#define SAMPLES		44100
#define REF_FILE	"ref.wav"

#define CH		1
typedef struct {
  int ce;
  int wi;
  int th;
} peak_t;

peak_t pk_ref[] = { {753,2,500}, {602,2,500}, {901,2,400} };

short  *ps_raw; /* pcm data */
double *pd_ref = NULL;	/* after DCT ref. */
double *fftw_in;	/* input DCT */
fftw_complex *fftw_out;	/* output DCT */
int sigTermArrived = 0;
fftw_plan plan;
int ref_peak;

void dft(int n, short *s_in);
int read_file( const char *in_fname, short** pptr, unsigned int *len);

void print_size(void)
{
  printf("short:%d\n", sizeof(short)); 
}
void fin(void)
{
  fftw_free(fftw_in);
  fftw_free(fftw_out);
  fftw_destroy_plan(plan);
  if (pd_ref != NULL) free(pd_ref);
}
void sigint(int sig) {
        sigTermArrived = 1;
}
void mk_ref(int num, double *pd_ref, fftw_complex *p)
{
  int i;

  printf("mk_ref:in\n");
  for (i=0; i<num; i++, p++, pd_ref++){
    *pd_ref = csqrt(*p);
  }
  printf("mk_ref:out\n");
}
void show_peak(int num, double *p)
{
  int i;
  double val, max = 0;
  int index = 0;

  printf("show_peak:in\n");
  for (i=0; i<num; i++, p++){
    val = *p;
#ifdef GET_PEAK
    printf("ref:i:%4d:%f\n", i, val);
#endif
    if (max < val){
      max = val;
      index = i;
    }
  }
  printf("ref:idx:%4d sqrt:%f ", index, max);
  ref_peak = index;
#ifdef GET_PEAK
  exit(0);
#endif
}
int chk_peak(fftw_complex *p)
{
  int i, j;
  int pk_num = sizeof(pk_ref)/sizeof(peak_t);
  int flag = 0;
  peak_t *pr = pk_ref;

  for (i=0; i<pk_num; i++, pr++){
    for (j= pr->ce - pr->wi; j< pr->ce + pr->wi; j++){
      if ((double)csqrt(p[j]) > pr->th){
        flag |= 0x1 << i;
        break;
      }
    }
  }
  printf("peak_flag:%4d\n", flag);
  //exit(0);
  if (flag == 0x7){
    return 1;
  }
  return 0;
}

void init(void)
{
  unsigned int len; //not use (dummy)

  //print_size();

        if (SIG_ERR == signal(SIGINT, sigint)) { /* CTRL+C */
                printf("failed to set signal handler SIGINT\n");
                exit(1);
        }
        if (SIG_ERR == signal(SIGTERM, sigint)) { /* SIGTERM */
                printf("failed to set signal handler SIGTERM\n");
                exit(1);
        }

#ifdef GET_PEAK
  /* get ps_raw mem. is in the func. */
  read_file(REF_FILE, &ps_raw, &len);
#if 1
  printf("wav:len:%d - %5d %5d %5d %5d - %5d %5d %5d %5d\n", len,
	*ps_raw, *(ps_raw+1), *(ps_raw+2), *(ps_raw+3),
	*(ps_raw+4), *(ps_raw+5), *(ps_raw+6), *(ps_raw+7) );
#endif
  dft(SAMPLES, ps_raw);
  pd_ref = (double*)malloc(SAMPLES*sizeof(double));
  mk_ref(SAMPLES, pd_ref, fftw_out);
  show_peak(SAMPLES>>5, pd_ref);
  free(ps_raw);
#endif /* GET_PEAK */
}

void main (int argc, char *argv[])
{
  int i;
  int err;
  char *buffer;
  int ret;

  init();

  int buffer_frames = SAMPLES;
  unsigned int rate = SAMPLES;

  snd_pcm_t *capture_handle;
  snd_pcm_hw_params_t *hw_params;
  snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

  if ((err = snd_pcm_open (&capture_handle, argv[1], SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    fprintf (stderr, "cannot open audio device %s (%s)\n", 
             argv[1],
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "audio interface opened\n");
		   
  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
    fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "hw_params allocated\n");
				 
  if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "hw_params initialized\n");
	
  if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "hw_params access setted\n");
	
  if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, format)) < 0) {
    fprintf (stderr, "cannot set sample format (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "hw_params format setted\n");
	
  if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0) {
    fprintf (stderr, "cannot set sample rate (%s)\n",
             snd_strerror (err));
    exit (1);
  }
	
  fprintf(stdout, "hw_params rate setted:rate:%d\n", rate);

#if 0
  if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, CH)) < 0) {
    fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "hw_params channels setted\n");
#endif
	
  if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "hw_params setted\n");
	
  snd_pcm_hw_params_free (hw_params);

  fprintf(stdout, "hw_params freed\n");
	
  if ((err = snd_pcm_prepare (capture_handle)) < 0) {
    fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror (err));
    exit (1);
  }

  fprintf(stdout, "audio interface prepared\n");

  buffer = malloc(SAMPLES * snd_pcm_format_width(format) / 8 * CH);

  fprintf(stdout, "buffer allocated\n");

  while (1) {
    if ((err = snd_pcm_readi (capture_handle, buffer, buffer_frames)) != buffer_frames) {
      fprintf (stderr, "read from audio interface failed (%s)\n",
               err, snd_strerror (err));
      exit (1);
    }
    /* show result */
    fprintf(stdout, "cap :ret:%d ", ret );
    short *pbuf = (short*)buffer; 
    printf("raw:%5d %5d %5d %5d ", *pbuf, *(pbuf+1), *(pbuf+2), *(pbuf+3)); 
    if (CH ==2){
      short *pbuf_tmp = pbuf; 
      /* 2ch to 1ch */
      for (int i=0; i<SAMPLES; i++, pbuf_tmp++){
        *pbuf_tmp = *(pbuf+i*2);
      }
    }

    /* analysis */
    dft(SAMPLES, pbuf);
    int ret = chk_peak( fftw_out );
    if (ret){
      system("nc 192.168.0.17 12346 -F");
    }
    if (sigTermArrived) break;
  }

  free(buffer);

  fprintf(stdout, "buffer freed\n");
	
  snd_pcm_close (capture_handle);
  fprintf(stdout, "audio interface closed\n");

  fin();
  exit (0);
}

void dft(int n, short *s_in)
{
  int i;
  double *tmp_in;
  static int flag = 0;

  if (flag==0){
  	fftw_in = fftw_alloc_real(SAMPLES*sizeof(double));
  	fftw_out = fftw_alloc_complex( SAMPLES*sizeof(fftw_complex) );
  	plan = fftw_plan_dft_r2c_1d(n, fftw_in, fftw_out, 0);
	flag++;
  }
  /* set in data */
  tmp_in = fftw_in;
  for (i=0; i < n; i++, tmp_in++, s_in++ ){
    *tmp_in = *s_in;
  }
  fftw_execute(plan);
}

int read_file( const char *in_fname, short** pptr, unsigned int *len)
/* Note:len is samples */
{
	SNDFILE		*sfp;
	SF_INFO		sfinfo;
	int		cnt;

	sfinfo.format = 0;

	sfp = sf_open(in_fname, SFM_READ, &sfinfo);
	if (sfp == NULL){
		printf("sf_open(%s) err\n", in_fname);
		return -1;
	}
#if 1
	printf("wav file: cnt:%lld, rate:%d, ch:%d, fmt:0x%x\n",
	  sfinfo.frames, sfinfo.samplerate, sfinfo.channels,
	  sfinfo.format);
#endif

	*pptr = calloc( sfinfo.frames, sizeof(short) );
	if  (*pptr == NULL){
        	printf("pcm.c calloc err\n");
		sf_close(sfp);
        	return -2;
    	}
	cnt = sf_read_short(sfp, *pptr, sfinfo.frames);
	if  (cnt != sfinfo.frames){
        	printf("sfinfo.frames:%lld vs cnt:%d\n", sfinfo.frames, cnt);
		sf_close(sfp);
        	return -3;
    	}
	//printf("read %d samples\n", cnt);
	*len = sfinfo.frames;
	sf_close(sfp);
	//printf("1st pcm:%02x\n", *(*pptr) );
	return 0;
}
