// gen25 -- fills a buffer with a hamming or hanning window.
//		by r. luke dubois (luke@music.columbia.edu),
//			computer music center, columbia university, 2000.
//
//		ported from real-time cmix by brad garton and dave topper.
//			http://www.music.columbia.edu/cmix
//
//	objects and source are provided without warranty of any kind, express or implied.
//

/* the required include files */
//  ported to pure-data by Olaf Matthes <olaf.matthes@gmx.de>, Mai 2002

/* the required include files */
#ifdef MSP
#include "ext.h"
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

#define BUFFER 32768
// maximum size of table -- this memory is allocated with NewPtr()
#ifndef M_PI
#define      M_PI     3.141592654 // the big number...
#endif

#ifdef MSP
// object definition structure...
typedef struct gen25
{
	Object g_ob;				// required header
	void *g_out;				// an outlet
	long g_wintype;			// number of points in the bpf
	long g_buffsize;			// size of buffer
	long g_offset;				// offset into the output buffer (for list output)
	float *g_table;				// internal array for the wavetable
} gen25;

/* global necessary for 68K function macros to work */
fptr *FNS;

/* globalthat holds the class definition */
void *class;

// function prototypes here...
void gen25_int(gen25 *x, long n);
void gen25_assist(gen25 *x, void *b, long m, long a, char *s);
void gen25_bang(gen25 *x);
void gen25_offset(gen25 *x, long n);
void gen25_size(gen25 *x, long n);
void gen25_size(gen25 *x, long n);
void *gen25_new(long n, long o);
void *gen25_free(gen25 *x);
void DoTheDo(gen25 *x);

// init routine...
void main(fptr *f)
{
	long oldA4;
	
	// this code is a 68K-only macro
	oldA4 = SetCurrentA4();			// get correct value of A4
	RememberA4();					// store inside code resource
	// this is not necessary (but harmless) on PowerPC
	FNS = f;	
	
	// define the class
	setup(&class, gen25_new,gen25_free, (short)sizeof(gen25), 0L, A_DEFLONG, A_DEFLONG, 0);
	// methods, methods, methods...
	addbang((method)gen25_bang); /* put out the same shit */
	addmess((method)gen25_size, "size", A_DEFLONG, 0); /* change buffer */
	addmess((method)gen25_offset, "offset", A_DEFLONG, 0); /* change buffer offset */
	addint((method)gen25_int); /* the goods... */
	addmess((method)gen25_assist,"assist",A_CANT,0); /* help */
	// restore old value of A4 (68K only)
	RestoreA4(oldA4);
	
	post("gen25: by r. luke dubois, cmc");
}

// those methods

void gen25_bang(gen25 *x)
{
	EnterCallback();
						
	DoTheDo(x);
	
	ExitCallback();		// undoes EnterCallback
}

void gen25_size(gen25 *x, long n)
{
	EnterCallback();
	
	x->g_buffsize = n; // resize buffer
	if (x->g_buffsize>BUFFER) x->g_buffsize = BUFFER;

	ExitCallback();
}

void gen25_offset(gen25 *x, long n)
{
	EnterCallback();
	
	x->g_offset = n; // change buffer offset

	ExitCallback();
}


// instance creation...

void gen25_int(gen25 *x, long n)
{
	EnterCallback();
	switch (n) {
		case 1: // hanning
			x->g_wintype = 1;
			break;
		case 2: // hamming
			x->g_wintype = 2;
			break;
		default:
			x->g_wintype = 1;
			error("only two window types (yet)");
			break;	
	}
	DoTheDo(x);
	ExitCallback();
}

void DoTheDo(gen25 *x)
{
	register short i, j;
	Atom thestuff[2];
	float wmax, xmax=0.0;

	switch (x->g_wintype) {
		case 1: /* hanning window */
			for (i = 0; i < x->g_buffsize; i++)
				x->g_table[i] = -cos(2.0*M_PI * (float)i/(float)(x->g_buffsize)) * 0.5 + 0.5;
			break;
		case 2: /* hamming window */
			for (i = 0; i < x->g_buffsize; i++)
				x->g_table[i] = 0.54 - 0.46*cos(2.0*M_PI * (float)i/(float)(x->g_buffsize));
			break;
		}

	// rescale the wavetable to go between -1. and 1.
	for(j = 0; j < x->g_buffsize; j++) {
		if ((wmax = fabs(x->g_table[j])) > xmax) xmax = wmax;
	}
	for(j = 0; j < x->g_buffsize; j++) {
		x->g_table[j] /= xmax;
	}

	// output the wavetable in index, amplitude pairs...
	for(i=0;i<x->g_buffsize;i++) {
		SETLONG(thestuff,i+(x->g_offset*x->g_buffsize));
		SETFLOAT(thestuff+1,x->g_table[i]);
		outlet_list(x->g_out,0L,2,thestuff);
	}
}

void *gen25_new(long n, long o)
{
	gen25 *x;
	register short c;
	
	EnterCallback();
	x = newobject(class);		// get memory for the object
	
	x->g_offset = 0;
	if (o) {
		x->g_offset = o;
	}

// initialize wavetable size (must allocate memory)
	x->g_buffsize=512;

if (n) {
	x->g_buffsize=n;
	if (x->g_buffsize>BUFFER) x->g_buffsize=BUFFER; // size the wavetable
	}

	x->g_table=NULL;
	x->g_table = (float*) NewPtr(sizeof(float) * BUFFER);
	if (x->g_table == NULL) {
		error("memory allocation error\n"); // whoops, out of memory...
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

void *gen25_free(gen25 *x)
{
	EnterCallback();
	if (x != NULL) {
		if (x->g_table != NULL) {
			DisposePtr((Ptr) x->g_table); // free the memory allocated for the table...
		}
	}
	ExitCallback();
}

void gen25_assist(gen25 *x, void *b, long msg, long arg, char *dst)
{
	switch(msg) {
		case 1: // inlet
			switch(arg) {
				case 0:
				sprintf(dst, "(int) Window Type (1 or 2)");
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
typedef struct gen25
{
	t_object g_ob;				// required header
	t_outlet *g_out;				// an outlet
	t_int g_wintype;				// window type
	t_int g_buffsize;			// size of buffer
	t_int g_offset;				// offset into output buffer (for list output)
	t_float *g_table;				// internal array for computing the transfer function
} gen25;


/* global that holds the class definition */
static t_class *gen25_class;


// those methods

static void DoTheDo(gen25 *x)
{
	register short i, j;
	t_atom thestuff[2];
	float wmax, xmax=0.0;

	switch (x->g_wintype) {
		case 1: /* hanning window */
			for (i = 0; i < x->g_buffsize; i++)
				x->g_table[i] = -cos(2.0*M_PI * (float)i/(float)(x->g_buffsize)) * 0.5 + 0.5;
			break;
		case 2: /* hamming window */
			for (i = 0; i < x->g_buffsize; i++)
				x->g_table[i] = 0.54 - 0.46*cos(2.0*M_PI * (float)i/(float)(x->g_buffsize));
			break;
		}

	// rescale the wavetable to go between -1. and 1.
	for(j = 0; j < x->g_buffsize; j++) {
		if ((wmax = fabs(x->g_table[j])) > xmax) xmax = wmax;
	}
	for(j = 0; j < x->g_buffsize; j++) {
		x->g_table[j] /= xmax;
	}

	// output the wavetable in index, amplitude pairs...
	for(i=0;i<x->g_buffsize;i++) {
		SETFLOAT(thestuff,i+(x->g_offset*x->g_buffsize));
		SETFLOAT(thestuff+1,x->g_table[i]);
		outlet_list(x->g_out,0L,2,thestuff);
	}
}

static void gen25_bang(gen25 *x)
{
						
	DoTheDo(x);
	
}


static void gen25_size(gen25 *x, long n)
{
	x->g_buffsize = n; // resize buffer
	if (x->g_buffsize>BUFFER) x->g_buffsize = BUFFER; // don't go beyond max limit of buffer
}

static void gen25_offset(gen25 *x, long n)
{
	x->g_offset = n; // change buffer offset
}

// instance creation...

static void gen25_int(gen25 *x, t_floatarg mode)
{
	// parse the mode...
	register short i;
	if(mode<1) {
	pd_error(NULL, "don't know about mode %g", mode);
	x->g_wintype = 1;
	}
	else if(mode>2) {
	pd_error(NULL, "don't know about mode %g", mode);
	x->g_wintype = 2;
	}
	else {
	x->g_wintype = (t_int)mode;
	post("window type set to %d", x->g_wintype);
	DoTheDo(x);
	}
}


static void *gen25_new(t_floatarg n, t_floatarg o)
{
	register short c;
	
	gen25 *x = (gen25 *)pd_new(gen25_class);		// get memory for the object

	x->g_offset = 0;
	if (o) {
		x->g_offset = (t_int)o;
	}


// initialize function table size (must allocate memory)
	x->g_buffsize=512;
	x->g_wintype = 1; // init window type

if (n) {
	x->g_buffsize=(t_int)n;
	if (x->g_buffsize>BUFFER) x->g_buffsize=BUFFER; // size the function array
	}

	x->g_table=NULL;
	x->g_table = (t_float*) getbytes(sizeof(t_float) * BUFFER);
	if (x->g_table == NULL) {
		perror("memory allocation error"); // whoops, out of memory...
		return (x);
	}

	for(c=0;c<x->g_buffsize;c++)
	{
		x->g_table[c]=0.0;
	}
	x->g_out = outlet_new(&x->g_ob, gensym("float"));				// create a list outlet
	
	post("gen25: by r. luke dubois, cmc");

	return (x);							// return newly created object and go go go...
}

static void *gen25_free(gen25 *x)
{
	if (x != NULL) {
		if (x->g_table != NULL) {
			freebytes(x->g_table, sizeof(t_float) * BUFFER); // free the memory allocated for the table...
		}
	}
	return(x);
}

// init routine...
void gen25_setup(void)
{
	gen25_class = class_new(gensym("gen25"), (t_newmethod)gen25_new, (t_method)gen25_free,
        sizeof(gen25), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addbang(gen25_class, (t_method)gen25_bang); /* put out the same shit */
	class_addfloat(gen25_class, gen25_int);
    class_addmethod(gen25_class, (t_method)gen25_size, gensym("size"), A_FLOAT, 0);	/* change buffer */
    class_addmethod(gen25_class, (t_method)gen25_offset, gensym("offset"), A_FLOAT, 0);	/* change buffer offset */
    class_sethelpsymbol(gen25_class, gensym("help-gen25.pd"));
}
#endif /* PD */

	
