// gen20 -- random number generator.
//		by r. luke dubois (luke@music.columbia.edu),
//			computer music center, columbia university, 2000.
//
//		ported from real-time cmix by brad garton and dave topper.
//			http://www.music.columbia.edu/cmix
//
//	objects and source are provided without warranty of any kind, express or implied.
//
//  ported to pure-data by Olaf Matthes <olaf.matthes@gmx.de>, Mai 2002
//

/* the required include files */
#ifdef MSP
#include "ext.h"
#include "z_dsp.h"
#include <SetUpA4.h>
#include <A4Stuff.h>
#endif
#ifdef PD
#include "m_pd.h"
#endif
#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif
#include <math.h>
#include <time.h>

#define BUFFER 32768
#define PI 3.141592654 

#ifdef MSP
// object definition structure...
typedef struct gen20
{
	Object g_ob;				// required header
	void *g_out;				// an outlet
	long g_mode;				// random mode
	long g_buffsize;			// size of buffer
	long g_offset;				// offset into output buffer (for list output)
	float *g_table;				// internal array for computing the transfer function
} gen20;

/* global necessary for 68K function macros to work */
fptr *FNS;

/* global that holds the class definition */
void *class;



// those methods

void gen20_bang(gen20 *x)
{
	EnterCallback();
						
	DoTheDo(x);
	
	ExitCallback();		// undoes EnterCallback
}


void gen20_size(gen20 *x, long n)
{
	EnterCallback();
	
	x->g_buffsize = n; // resize buffer
	if (x->g_buffsize>BUFFER) x->g_buffsize = BUFFER; // don't go beyond max limit of buffer

	ExitCallback();
}

void gen20_offset(gen20 *x, long n)
{
	EnterCallback();
	
	x->g_offset = n; // change buffer offset

	ExitCallback();
}

// instance creation...

void gen20_int(gen20 *x, long mode)
{

	// parse the mode...
	register short i;
	EnterCallback();
	if(mode<0) {
	error("don't know about mode %i", mode);
	x->g_mode = 0;
	}
	else if(mode>5) {
	error("don't know about mode %i", mode);
	x->g_mode = 0;
	}
	else {
	x->g_mode = mode;
	post("mode set to %i", mode);
	DoTheDo(x);
	}
	ExitCallback();
}

void DoTheDo(gen20 *x)
{
	register short i,j;
	int k;
	Atom thestuff[2];
	int N=12;
	float halfN = 6;
	float scale = 1;
	float sum=0;
	float mu = 0.5;
	float sigma = .166666;
	float randnum = 0.0;
	float randnum2 = 0.0;
	float output;
	float alpha = .00628338;
	float randfunc();
	static long randx=1;

randx = gettime();

switch(x->g_mode) {
	case 0: // even distribution
	   for(i=0;i<x->g_buffsize;i++){
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     x->g_table[i] = (float)k/32768.0;
   }
   break;
 case 1: /* low weighted */
   for(i=0;i<x->g_buffsize;i++) {
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     randnum = (float)k/32768.0;
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     randnum2 = (float)k/32768.0;
     if(randnum2 < randnum) {
       randnum = randnum2;
     }
     x->g_table[i] = randnum;
   }
   break;
 case 2: /* high weighted */
   for(i=0;i<x->g_buffsize;i++) {
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     randnum = (float)k/32768.0;
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     randnum2 = (float)k/32768.0;
     if(randnum2 > randnum) {
       randnum = randnum2;
     }
     x->g_table[i] = randnum;
   }
   break;
 case 3: /* triangle */
   for(i=0;i<x->g_buffsize;i++) {
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     randnum = (float)k/32768.0;
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     randnum2 = (float)k/32768.0;
     x->g_table[i] = 0.5*(randnum+randnum2);
   }
   break;
 case 4: /* gaussian */
   i=0;
   while(i<x->g_buffsize) {
     randnum = 0.0;
     for(j=0;j<N;j++)
       {
	 k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
	 randnum += (float)k/32768.0;
       }
     output = sigma*scale*(randnum-halfN)+mu;
     if((output<=1.0) && (output>=0.0)) {
       x->g_table[i] = output;
       i++;
     }
   }
   break;
 case 5: /* cauchy */
   i=0;
   while(i<x->g_buffsize)
     {
       do {
	 k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
       randnum = (float)k/32768.0;
       }
       while(randnum==0.5);
       randnum = randnum*PI;
       output=(alpha*tan(randnum))+0.5;
       if((output<=1.0) && (output>=0.0)) {
	 x->g_table[i] = output;
	 i++;
       }
     }
   break;   
   default:
   		for(i=0;i<x->g_buffsize;i++) {
   			x->g_table[i]=0.;
   		}
   	break;
}
	// rescale the function to make sure it stays between -1. and 1.
	for(j = 0; j < x->g_buffsize; j++) {
		x->g_table[j] = (x->g_table[j]*2.0)-1.;
	}

	// output the random series in index/amplitude pairs...
	for(i=0;i<x->g_buffsize;i++) {
		SETLONG(thestuff,i+(x->g_offset*x->g_buffsize));
		SETFLOAT(thestuff+1,x->g_table[i]);
		outlet_list(x->g_out,0L,2,thestuff);
	}
}

void *gen20_new(long n, long o)
{
	gen20 *x;
	register short c;
	
	EnterCallback();
	x = newobject(class);		// get memory for the object

	x->g_offset = 0;
	if (o) {
		x->g_offset = o;
	}


// initialize function table size (must allocate memory)
	x->g_buffsize=512;
	x->g_mode = 0; // init mode

if (n) {
	x->g_buffsize=n;
	if (x->g_buffsize>BUFFER) x->g_buffsize=BUFFER; // size the function array
	}

	x->g_table=NULL;
	x->g_table = (float*) NewPtr(sizeof(float) * BUFFER);
	if (x->g_table == NULL) {
		error("memory allocation error"); // whoops, out of memory...
		ExitCallback();
		return (x);
	}

	for(c=0;c<x->g_buffsize;c++)
	{
		x->g_table[c]=0.0;
	}
	x->g_out = listout(x);				// create a list outlet
	ExitCallback();
	return (x);							// return newly created object and go go go...


}

void *gen20_free(gen20 *x)
{
	EnterCallback();
	if (x != NULL) {
		if (x->g_table != NULL) {
			DisposePtr((Ptr) x->g_table); // free the memory allocated for the table...
		}
	}
	ExitCallback();
}

void gen20_assist(gen20 *x, void *b, long msg, long arg, char *dst)
{
	switch(msg) {
		case 1: // inlet
			switch(arg) {
				case 0:
				sprintf(dst, "(int) Mode");
				break;
			}
		break;
		case 2: // outlet
			switch(arg) {
				case 0:
				sprintf(dst, "(list) Index/Amplitude Pairs");
				break;
			}
		break;
	}
}
#endif /* MSP */

/* --------------------------- pure-data --------------------------------------- */
#ifdef PD
// object definition structure...
typedef struct gen20
{
	t_object g_ob;				// required header
	t_outlet *g_out;				// an outlet
	t_int g_mode;				// random mode
	t_int g_buffsize;			// size of buffer
	t_int g_offset;				// offset into output buffer (for list output)
	t_float *g_table;				// internal array for computing the transfer function
} gen20;


/* global that holds the class definition */
static t_class *gen20_class;


// those methods

static void DoTheDo(gen20 *x)
{
	register short i,j;
	int k;
	t_atom thestuff[2];
	int N=12;
	float halfN = 6;
	float scale = 1;
	float sum=0;
	float mu = 0.5;
	float sigma = .166666;
	float randnum = 0.0;
	float randnum2 = 0.0;
	float output;
	float alpha = .00628338;
	float randfunc(void);
	static long randx=1;

randx = clock_getlogicaltime();

switch(x->g_mode) {
	case 0: // even distribution
	   for(i=0;i<x->g_buffsize;i++){
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     x->g_table[i] = (float)k/32768.0;
   }
   break;
 case 1: /* low weighted */
   for(i=0;i<x->g_buffsize;i++) {
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     randnum = (float)k/32768.0;
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     randnum2 = (float)k/32768.0;
     if(randnum2 < randnum) {
       randnum = randnum2;
     }
     x->g_table[i] = randnum;
   }
   break;
 case 2: /* high weighted */
   for(i=0;i<x->g_buffsize;i++) {
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     randnum = (float)k/32768.0;
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     randnum2 = (float)k/32768.0;
     if(randnum2 > randnum) {
       randnum = randnum2;
     }
     x->g_table[i] = randnum;
   }
   break;
 case 3: /* triangle */
   for(i=0;i<x->g_buffsize;i++) {
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     randnum = (float)k/32768.0;
     k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
     randnum2 = (float)k/32768.0;
     x->g_table[i] = 0.5*(randnum+randnum2);
   }
   break;
 case 4: /* gaussian */
   i=0;
   while(i<x->g_buffsize) {
     randnum = 0.0;
     for(j=0;j<N;j++)
       {
	 k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
	 randnum += (float)k/32768.0;
       }
     output = sigma*scale*(randnum-halfN)+mu;
     if((output<=1.0) && (output>=0.0)) {
       x->g_table[i] = output;
       i++;
     }
   }
   break;
 case 5: /* cauchy */
   i=0;
   while(i<x->g_buffsize)
     {
       do {
	 k = ((randx = randx*1103515245 + 12345)>>16) & 077777;
       randnum = (float)k/32768.0;
       }
       while(randnum==0.5);
       randnum = randnum*PI;
       output=(alpha*tan(randnum))+0.5;
       if((output<=1.0) && (output>=0.0)) {
	 x->g_table[i] = output;
	 i++;
       }
     }
   break;   
   default:
   		for(i=0;i<x->g_buffsize;i++) {
   			x->g_table[i]=0.;
   		}
   	break;
}
	// rescale the function to make sure it stays between -1. and 1.
	for(j = 0; j < x->g_buffsize; j++) {
		x->g_table[j] = (x->g_table[j]*2.0)-1.;
	}

	// output the random series in index/amplitude pairs...
	for(i=0;i<x->g_buffsize;i++) {
		SETFLOAT(thestuff,i+(x->g_offset*x->g_buffsize));
		SETFLOAT(thestuff+1,x->g_table[i]);
		outlet_list(x->g_out,0L,2,thestuff);
	}
}

static void gen20_bang(gen20 *x)
{
						
	DoTheDo(x);
	
}


static void gen20_size(gen20 *x, long n)
{
	x->g_buffsize = n; // resize buffer
	if (x->g_buffsize>BUFFER) x->g_buffsize = BUFFER; // don't go beyond max limit of buffer
}

static void gen20_offset(gen20 *x, long n)
{
	x->g_offset = n; // change buffer offset
}

// instance creation...

static void gen20_int(gen20 *x, t_floatarg mode)
{
	// parse the mode...
	register short i;
	if(mode<0) {
	error("don't know about mode %g", mode);
	x->g_mode = 0;
	}
	else if(mode>5) {
	error("don't know about mode %g", mode);
	x->g_mode = 0;
	}
	else {
	x->g_mode = (t_int)mode;
	post("mode set to %d", x->g_mode);
	DoTheDo(x);
	}
}


static void *gen20_new(t_floatarg n, t_floatarg o)
{
	register short c;
	
	gen20 *x = (gen20 *)pd_new(gen20_class);		// get memory for the object

	x->g_offset = 0;
	if (o) {
		x->g_offset = (t_int)o;
	}


// initialize function table size (must allocate memory)
	x->g_buffsize=512;
	x->g_mode = 0; // init mode

if (n) {
	x->g_buffsize=(t_int)n;
	if (x->g_buffsize>BUFFER) x->g_buffsize=BUFFER; // size the function array
	}

	x->g_table=NULL;
	x->g_table = (t_float*) getbytes(sizeof(t_float) * BUFFER);
	if (x->g_table == NULL) {
		error("memory allocation error"); // whoops, out of memory...
		return (x);
	}

	for(c=0;c<x->g_buffsize;c++)
	{
		x->g_table[c]=0.0;
	}
	x->g_out = outlet_new(&x->g_ob, gensym("float"));				// create a list outlet
	
	post("gen20: by r. luke dubois, cmc");

	return (x);							// return newly created object and go go go...
}

static void *gen20_free(gen20 *x)
{
	if (x != NULL) {
		if (x->g_table != NULL) {
			freebytes(x->g_table, sizeof(t_float) * BUFFER); // free the memory allocated for the table...
		}
	}
	return(x);
}

// init routine...
void gen20_setup(void)
{
	gen20_class = class_new(gensym("gen20"), (t_newmethod)gen20_new, (t_method)gen20_free,
        sizeof(gen20), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addbang(gen20_class, (t_method)gen20_bang); /* put out the same shit */
	class_addfloat(gen20_class, gen20_int);
    class_addmethod(gen20_class, (t_method)gen20_size, gensym("size"), A_FLOAT, 0);	/* change buffer */
    class_addmethod(gen20_class, (t_method)gen20_offset, gensym("offset"), A_FLOAT, 0);	/* change buffer offset */
    class_sethelpsymbol(gen20_class, gensym("help-gen20.pd"));
}
#endif /* PD */
