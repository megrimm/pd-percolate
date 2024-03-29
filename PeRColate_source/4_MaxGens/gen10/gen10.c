// gen10 -- takes a list of harmonic values and writes a wavetable.
//		by r. luke dubois (luke@music.columbia.edu),
//			computer music center, columbia university, 2000.
//
//		ported from real-time cmix by brad garton and dave topper.
//			http://www.music.columbia.edu/cmix
//
//	objects and source are provided without warranty of any kind, express or implied.
//
//  ported to pure-data by Olaf Matthes <olaf.matthes@gmx.de>, Mai 2002

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

// maximum number of harmonics specified in a list -- if you make it larger you better 
// allocated the memory dynamically (right now it's using built-in memory)...
#define MAXSIZE 64
#define BUFFER 32768
// maximum size of wavetable -- this memory is allocated with NewPtr()
#define      PI2    6.2831853 // the big number...

#ifdef MSP
// object definition structure...
typedef struct gen10
{
	Object g_ob;				// required header
	void *g_out;				// an outlet
	long g_numharmonics;		// number of harmonics
	long g_buffsize;			// size of buffer
	long g_offset;				// offset into the output buffer (for list output)
	float g_args[MAXSIZE];		// array for the harmonic fields
	float *g_table;				// internal array for the wavetable
	long g_rescale;				// flag to rescale array
} Gen10;

/* globalthat holds the class definition */
void *class;

// function prototypes here...
void gen10_list(Gen10 *x, Symbol *s, short ac, Atom *av);
void gen10_assist(Gen10 *x, void *b, long m, long a, char *s);
void gen10_bang(Gen10 *x);
void gen10_offset(Gen10 *x, long n);
void gen10_size(Gen10 *x, long n);
void gen10_rescale(Gen10 *x, long n);
void *gen10_new(long n, long o);
void *gen10_free(Gen10 *x);
void DoTheDo(Gen10 *x);

// init routine...
void main(fptr *f)
{
	
	// define the class
	setup(&class, gen10_new,gen10_free, (short)sizeof(Gen10), 0L, A_DEFLONG, A_DEFLONG, 0);
	// methods, methods, methods...
	addbang((method)gen10_bang); /* put out the same shit */
	addmess((method)gen10_size, "size", A_DEFLONG, 0); /* change buffer */
	addmess((method)gen10_offset, "offset", A_DEFLONG, 0); /* change buffer offset */
	addmess((method)gen10_rescale, "rescale", A_DEFLONG, 0); /* change array rescaling */
	addmess((method)gen10_list, "list", A_GIMME, 0); /* the goods... */
	addmess((method)gen10_assist,"assist",A_CANT,0); /* help */
	// restore old value of A4 (68K only)
	
	post("gen10: by r. luke dubois, cmc");
}

// those methods

void gen10_bang(Gen10 *x)
{
						
	DoTheDo(x);
	
}

void gen10_size(Gen10 *x, long n)
{
	
	x->g_buffsize = n; // resize buffer
	if (x->g_buffsize>BUFFER) x->g_buffsize = BUFFER;

}

void gen10_offset(Gen10 *x, long n)
{
	
	x->g_offset = n; // change buffer offset

}

void gen10_rescale(Gen10 *x, long n)
{
	if(n>1) n = 1;
	if(n<0) n = 0;
	x->g_rescale = n; // change rescale flag

}


// instance creation...

void gen10_list(Gen10 *x, Symbol *s, short argc, Atom *argv)
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
	x->g_numharmonics = argc;
	DoTheDo(x);
}

void DoTheDo(Gen10 *x)
{
	register short i,j;
	Atom thestuff[2];
	float wmax, xmax=0.0;
	
	// compute the wavetable...
	for(i = 0; i<x->g_buffsize; i++) x->g_table[i] = 0.0;
	j=x->g_numharmonics;
	while(j--) {
		if(x->g_args[j] != 0.0) {
			for(i=0; i<x->g_buffsize; i++) {
				x->g_table[i]+=sin((double)(PI2*(float)i/
				(x->g_buffsize/(j+1))))*x->g_args[j];
			}
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

void *gen10_new(long n, long o)
{
	Gen10 *x;
	register short c;
	
	x = newobject(class);		// get memory for the object
	
	x->g_offset = 0;
	if (o) {
		x->g_offset = o;
	}

	for(c=0;c<MAXSIZE;c++) // initialize harmonics array (static memory)
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

void *gen10_free(Gen10 *x)
{
	if (x != NULL) {
		if (x->g_table != NULL) {
			DisposePtr((Ptr) x->g_table); // free the memory allocated for the table...
		}
	}
}

void gen10_assist(Gen10 *x, void *b, long msg, long arg, char *dst)
{
	switch(msg) {
		case 1: // inlet
			switch(arg) {
				case 0:
				sprintf(dst, "(list) Harmonic Partial Amplitudes");
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

/* -------------------------------------- pure data ---------------------------------- */
#ifdef PD
// object definition structure...
typedef struct gen10
{
	t_object g_ob;				// required header
	t_outlet *g_out;				// an outlet
	long g_numharmonics;			// number of points in the bpf
	long g_buffsize;			// size of buffer
	long g_offset;				// offset into the output buffer (for list output)
	float g_args[MAXSIZE];		// array for the harmonic fields
	float *g_table;				// internal array for the wavetable
	long g_rescale;				// flag to rescale array
} gen10;

/* globalthat holds the class definition */
static t_class *gen10_class;

// those methods

static void DoTheDo(gen10 *x)
{
	register short i,j;
	t_atom thestuff[2];
	float wmax, xmax=0.0;
	
	// compute the wavetable...
	for(i = 0; i<x->g_buffsize; i++) x->g_table[i] = 0.0;
	j=x->g_numharmonics;
	while(j--) {
		if(x->g_args[j] != 0.0) {
			for(i=0; i<x->g_buffsize; i++) {
				x->g_table[i]+=sin((double)(PI2*(float)i/
				(x->g_buffsize/(j+1))))*x->g_args[j];
			}
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

static void gen10_bang(gen10 *x)
{
						
	DoTheDo(x);
	
}

static void gen10_size(gen10 *x, t_floatarg n)
{
	
	x->g_buffsize = n; // resize buffer
	if (x->g_buffsize>BUFFER) x->g_buffsize = BUFFER;

}

static void gen10_offset(gen10 *x, t_floatarg n)
{
	
	x->g_offset = (long)n; // change buffer offset

}

static void gen10_rescale(gen10 *x, t_floatarg n)
{
	if(n>1) n = 1.0;
	if(n<0) n = 0.0;
	x->g_rescale = (long)n; // change rescale flag

}

// instance creation...

void gen10_list(gen10 *x, t_symbol *s, t_int argc, t_atom *argv)
{

	// parse the list of incoming harmonics...
	register short i;
	for (i=0; i < argc; i++)
	{
		if (argv[i].a_type==A_FLOAT)
		{
			x->g_args[i] = (float)argv[i].a_w.w_float;
		}
	}
	x->g_numharmonics = argc;
	DoTheDo(x);
}


void *gen10_new(t_floatarg n, t_floatarg o)
{
	register short c;
	
	gen10 *x = (gen10 *)pd_new(gen10_class);
	
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


	if (n)
	{
		x->g_buffsize=n;
		if (x->g_buffsize>BUFFER) x->g_buffsize=BUFFER; // size the wavetable
	}

	x->g_table=NULL;
	x->g_table = (float*)getbytes(sizeof(float) * BUFFER);
	if (x->g_table == NULL)
	{
		perror("memory allocation error\n"); // whoops, out of memory...
		return (x);
	}

	for(c=0;c<x->g_buffsize;c++)
	{
		x->g_table[c]=0.0;
	}
	x->g_out = outlet_new(&x->g_ob, gensym("float"));
	post("gen10: by r. luke dubois, cmc");
	return (x);							// return newly created object and go go go...
}

static void *gen10_free(gen10 *x)
{
	if (x != NULL)
	{
		if (x->g_table != NULL)
		{
			freebytes(x->g_table, sizeof(float) * BUFFER); // free the memory allocated for the table...
		}
	}
	return(x);
}
// init routine...
void gen10_setup(void)
{
	gen10_class = class_new(gensym("gen10"), (t_newmethod)gen10_new, (t_method)gen10_free,
        sizeof(gen10), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addbang(gen10_class, (t_method)gen10_bang); /* put out the same shit */
    class_addmethod(gen10_class, (t_method)gen10_size, gensym("size"), A_FLOAT, 0);	/* change buffer */
    class_addmethod(gen10_class, (t_method)gen10_offset, gensym("offset"), A_FLOAT, 0);	/* change buffer offset */
	class_addmethod(gen10_class, (t_method)gen10_rescale, gensym("rescale"), A_FLOAT, 0);	/* change array rescaling */
	class_addmethod(gen10_class, (t_method)gen10_list, gensym("list"), A_GIMME, 0);	/* the goods... */
    class_sethelpsymbol(gen10_class, gensym("help-gen10.pd"));
}
#endif /* PD */
	

	
