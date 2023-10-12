// gen7 -- writes a wavetable from a linear bpf.
//		by r. luke dubois (luke@music.columbia.edu),
//			computer music center, columbia university, 2001.
//
//		ported from real-time cmix by brad garton and dave topper.
//			http://www.music.columbia.edu/cmix
//
//	objects and source are provided without warranty of any kind, express or implied.
//
//  ported to Pure-Data by Olaf Matthes <olaf.matthes@gmx.de>, May 2002

/* the required include files */
#ifdef MSP
#include "ext.h"
#endif
#ifdef PD
#include "m_pd.h"
#endif
#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif
#include <math.h>

// maximum number of p-fields specified in a list -- if you make it larger you better 
// allocated the memory dynamically (right now it's using built-in memory)...
#define MAXSIZE 64
#define BUFFER 32768
// maximum size of wavetable -- this memory is allocated with NewPtr()
#define      PI2    6.2831853 // the big number...

#ifdef MSP
// object definition structure...
typedef struct gen7
{
	Object g_ob;				// required header
	void *g_out;				// an outlet
	long g_numpoints;			// number of points in the bpf
	long g_buffsize;			// size of buffer
	long g_offset;				// offset into the output buffer (for list output)
	float g_args[MAXSIZE];		// array for the harmonic fields
	float *g_table;				// internal array for the wavetable
	long g_rescale;				// flag to rescale array
} gen7;

/* global necessary for 68K function macros to work */
fptr *FNS;

/* globalthat holds the class definition */
void *class;

// function prototypes here...
void gen7_list(gen7 *x, Symbol *s, short ac, Atom *av);
void gen7_assist(gen7 *x, void *b, long m, long a, char *s);
void gen7_bang(gen7 *x);
void gen7_offset(gen7 *x, long n);
void gen7_size(gen7 *x, long n);
void gen7_rescale(gen7 *x, long n);
void *gen7_new(long n, long o);
void *gen7_free(gen7 *x);
void DoTheDo(gen7 *x);

// init routine...
void main(fptr *f)
{
	
	// define the class
	setup(&class, gen7_new,gen7_free, (short)sizeof(gen7), 0L, A_DEFLONG, A_DEFLONG, 0);
	// methods, methods, methods...
	addbang((method)gen7_bang); /* put out the same shit */
	addmess((method)gen7_size, "size", A_DEFLONG, 0); /* change buffer */
	addmess((method)gen7_offset, "offset", A_DEFLONG, 0); /* change buffer offset */
	addmess((method)gen7_rescale, "rescale", A_DEFLONG, 0); /* change array rescaling */
	addmess((method)gen7_list, "list", A_GIMME, 0); /* the goods... */
	addmess((method)gen7_assist,"assist",A_CANT,0); /* help */
	
	post("gen7: by r. luke dubois, cmc");
}

// those methods

void gen7_bang(gen7 *x)
{
						
	DoTheDo(x);
	
}

void gen7_size(gen7 *x, long n)
{
	
	x->g_buffsize = n; // resize buffer
	if (x->g_buffsize>BUFFER) x->g_buffsize = BUFFER;

}

void gen7_offset(gen7 *x, long n)
{
	
	x->g_offset = n; // change buffer offset

}

void gen7_rescale(gen7 *x, long n)
{
	if(n>1) n = 1;
	if(n<0) n = 0;
	x->g_rescale = n; // change rescale flag

}


// instance creation...

void gen7_list(gen7 *x, Symbol *s, short argc, Atom *argv)
{

	// parse the list of incoming harmonics...
	register short i;
	for (i=0; i < argc; i++) {
		if (argv[i].a_type==A_LONG) {
			x->g_args[i] = (float)argv[i].a_w.w_long;
		}
		else if (argv[i].a_type==A_FLOAT) {
			x->g_args[i] = argv[i].a_w.w_float;
		}
	}
	x->g_numpoints = argc;
	DoTheDo(x);
}

void DoTheDo(gen7 *x)
{
	register short j,k,l;
	Atom thestuff[2];
	float scaler, amp2, amp1, wmax, xmax=0.0;
	int i=0;

	amp2 = x->g_args[0];
	for(k=1; k<x->g_numpoints; k += 2)   {
		amp1 = amp2;
		amp2 = x->g_args[k+1];
		j = i + 1;
		i = j + x->g_args[k] - 1;
		     for(l=j; l<=i; l++) {
				if(l <= x->g_buffsize)
				   x->g_table[l-1] = amp1 +
				      (amp2-amp1)*(float)(l-j)/(i-j+1);
					}
			}

if(x->g_rescale) {
	// rescale the wavetable to go between -1. and 1.
	for(j = 0; j < x->g_buffsize; j++) {
		if ((wmax = fabs(x->g_table[j])) > xmax) xmax = wmax;
	}
	for(j = 0; j < x->g_buffsize; j++) {
		x->g_table[j] /= xmax;
	}
}

	// output the wavetable in index, amplitude pairs...
	for(i=0;i<x->g_buffsize;i++) {
		SETLONG(thestuff,i+(x->g_offset*x->g_buffsize));
		SETFLOAT(thestuff+1,x->g_table[i]);
		outlet_list(x->g_out,0L,2,thestuff);
	}
}

void *gen7_new(long n, long o)
{
	gen7 *x;
	register short c;
	
	x = newobject(class);		// get memory for the object
	
	x->g_offset = 0;
	if (o) {
		x->g_offset = o;
	}

	for(c=0;c<MAXSIZE;c++) // initialize bpf array (static memory)
	{
		x->g_args[c] =0.0;
	}

// initialize wavetable size (must allocate memory)
	x->g_buffsize=512;

	x->g_rescale=1;

if (n) {
	x->g_buffsize=n;
	if (x->g_buffsize>BUFFER) x->g_buffsize=BUFFER; // size the wavetable
	}

	x->g_table=NULL;
	x->g_table = (float*) NewPtr(sizeof(float) * BUFFER);
	if (x->g_table == NULL) {
		error("memory allocation error\n"); // whoops, out of memory...
		return (x);
	}

	for(c=0;c<x->g_buffsize;c++)
	{
		x->g_table[c]=0.0;
	}
	x->g_out = listout(x);				// create a list outlet
	return (x);							// return newly created object and go go go...
}

void *gen7_free(gen7 *x)
{
	if (x != NULL) {
		if (x->g_table != NULL) {
			DisposePtr((Ptr) x->g_table); // free the memory allocated for the table...
		}
	}
}

void gen7_assist(gen7 *x, void *b, long msg, long arg, char *dst)
{
	switch(msg) {
		case 1: // inlet
			switch(arg) {
				case 0:
				sprintf(dst, "(list) Breakpoint Function for Waveform");
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


/* -------------------------------------- pure-data ---------------------------- */
#ifdef PD
// object definition structure...
typedef struct gen7
{
	t_object g_ob;				// required header
	t_outlet *g_out;				// an outlet
	t_int g_numpoints;			// number of points in the bpf
	t_int g_buffsize;			// size of buffer
	t_int g_offset;				// offset into the output buffer (for list output)
	t_float g_args[MAXSIZE];		// array for the harmonic fields
	t_float *g_table;				// internal array for the wavetable
	t_int g_rescale;				// flag to rescale array
} gen7;


	/* globalthat holds the class definition */
static t_class *gen7_class;

// those methods
static void DoTheDo(gen7 *x)
{
	register short j,k,l;
	t_atom thestuff[2];
	t_float scaler, amp2, amp1, wmax, xmax=0.0;
	t_int i=0;

	amp2 = x->g_args[0];
	for(k=1; k<x->g_numpoints; k += 2)   {
		amp1 = amp2;
		amp2 = x->g_args[k+1];
		j = i + 1;
		i = j + x->g_args[k] - 1;
		     for(l=j; l<=i; l++) {
				if(l <= x->g_buffsize)
				   x->g_table[l-1] = amp1 +
				      (amp2-amp1)*(t_float)(l-j)/(i-j+1);
					}
			}

	if(x->g_rescale) {
		// rescale the wavetable to go between -1. and 1.
		for(j = 0; j < x->g_buffsize; j++) {
			if ((wmax = fabs(x->g_table[j])) > xmax) xmax = wmax;
		}
		for(j = 0; j < x->g_buffsize; j++) {
			x->g_table[j] /= xmax;
		}
	}

	// output the wavetable in index, amplitude pairs...
	for(i=0;i<x->g_buffsize;i++) {
		SETFLOAT(thestuff,i+(x->g_offset*x->g_buffsize));
		SETFLOAT(thestuff+1,x->g_table[i]);
		outlet_list(x->g_out,0L,2,thestuff);
	}
}

static void gen7_bang(gen7 *x)
{
						
	DoTheDo(x);
	
}

static void gen7_size(gen7 *x, t_floatarg n)
{
	
	x->g_buffsize = n; // resize buffer
	if (x->g_buffsize>BUFFER) x->g_buffsize = BUFFER;

}

static void gen7_offset(gen7 *x, t_floatarg n)
{
	
	x->g_offset = n; // change buffer offset

}

static void gen7_rescale(gen7 *x, t_floatarg n)
{
	if(n>1) n = 1;
	if(n<0) n = 0;
	x->g_rescale = (t_int)n; // change rescale flag

}

// instance creation...

static void gen7_list(gen7 *x, t_symbol *s, t_int argc, t_atom *argv)
{

	// parse the list of incoming harmonics...
	register short i;
	for (i=0; i < argc; i++) {
		if (argv[i].a_type==A_FLOAT) {
			x->g_args[i] = argv[i].a_w.w_float;
		}
	}
	x->g_numpoints = argc;
	DoTheDo(x);
}


static void *gen7_new(t_floatarg n, t_floatarg o)
{
	register short c;
	
	gen7 *x = (gen7 *)pd_new(gen7_class);		// get memory for the object
	
	x->g_offset = 0;
	if (o) {
		x->g_offset = (t_int)o;
	}

	for(c=0;c<MAXSIZE;c++) // initialize bpf array (static memory)
	{
		x->g_args[c] =0.0;
	}

// initialize wavetable size (must allocate memory)
	x->g_buffsize=512;
	
	x->g_rescale=1;

	if (n)
	{
		x->g_buffsize=(t_int)n;
		if (x->g_buffsize>BUFFER) x->g_buffsize=BUFFER; // size the wavetable
	}

	x->g_table=NULL;
	x->g_table = (t_float*) getbytes(sizeof(t_float) * BUFFER);
	if (x->g_table == NULL) {
		perror("memory allocation error\n"); // whoops, out of memory...
		return (x);
	}

	for(c=0;c<x->g_buffsize;c++)
	{
		x->g_table[c]=0.0;
	}

	x->g_out = outlet_new(&x->g_ob, gensym("float"));				// create a list outlet
	post("gen7: by r. luke dubois, cmc");
	return (x);							// return newly created object and go go go...
}

static void *gen7_free(gen7 *x)
{
	if (x != NULL) {
		if (x->g_table != NULL) {
			freebytes(x->g_table, sizeof(t_float) * BUFFER); // free the memory allocated for the table...
		}
	}
	return(x);
}

// init routine...
void gen7_setup(void)
{
	gen7_class = class_new(gensym("gen7"), (t_newmethod)gen7_new, (t_method)gen7_free,
        sizeof(gen7), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addbang(gen7_class, (t_method)gen7_bang); /* put out the same shit */
    class_addmethod(gen7_class, (t_method)gen7_size, gensym("size"), A_FLOAT, 0);	/* change buffer */
    class_addmethod(gen7_class, (t_method)gen7_offset, gensym("offset"), A_FLOAT, 0);	/* change buffer offset */
	class_addmethod(gen7_class, (t_method)gen7_rescale, gensym("rescale"), A_FLOAT, 0);	/* change array rescaling */
	class_addmethod(gen7_class, (t_method)gen7_list, gensym("list"), A_GIMME, 0);	/* the goods... */
    class_sethelpsymbol(gen7_class, gensym("help-gen7.pd"));
}
#endif /* PD */
