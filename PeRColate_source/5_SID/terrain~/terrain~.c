// terrain~ -- interpolates between two or more wavetable 'frames' stored in a buffer.
// ported to Pd by Olaf Matthes (olaf.matthes@gmx.de)

#ifdef MSP
#include "ext.h"
#include "z_dsp.h"
#include "buffer.h"
#endif
#ifdef PD
#include "m_pd.h"
#endif
#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#define MAXFRAMES 32

/* -------------------------------------- MSP ----------------------------------- */
#ifdef MSP
void *terrain_class;

typedef struct _terrain
{
    t_pxobject l_obj;
    t_symbol *l_sym;
    t_buffer *l_buf;
    long l_framesize;
    long l_numframes;
    float l_hopsize;
} t_terrain;

float FConstrain(float v, float lo, float hi);
long IConstrain(long v, long lo, long hi);
t_int *terrain_perform(t_int *w);
void terrain_dsp(t_terrain *x, t_signal **sp);
void terrain_set(t_terrain *x, t_symbol *s);
void *terrain_new(t_symbol *s, long n, long f);
void terrain_assist(t_terrain *x, void *b, long m, long a, char *s);
void terrain_size(t_terrain *x, long size);
void terrain_frames(t_terrain *x, long frames);

t_symbol *ps_buffer;

void main(void)
{
	setup(&terrain_class, terrain_new, (method)dsp_free, (short)sizeof(t_terrain), 0L, 
		A_SYM, A_DEFLONG, A_DEFLONG, 0);
	addmess((method)terrain_dsp, "dsp", A_CANT, 0);
	addmess((method)terrain_set, "set", A_SYM, 0);
	addmess((method)terrain_assist, "assist", A_CANT, 0);
	addmess((method)terrain_size, "size", A_DEFLONG, 0);
	addmess((method)terrain_frames, "frames", A_DEFLONG, 0);
	dsp_initclass();
	ps_buffer = gensym("buffer~");
}

long IConstrain(long v, long lo, long hi)
{
	if (v < lo)
		return lo;
	else if (v > hi)
		return hi;
	else
		return v;
}

float FConstrain(float v, float lo, float hi)
{
	if (v < lo)
		return lo;
	else if (v > hi)
		return hi;
	else
		return v;
}

t_int *terrain_perform(t_int *w)
{
    t_terrain *x = (t_terrain *)(w[1]);
    t_float *inx = (t_float *)(w[2]);
    t_float *iny = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    int n = (int)(w[5]);
	t_buffer *b = x->l_buf;
	float *tab;
	float x_coords, y_coords, hopsize;
	long index, index2, chan,frames,nc,fsize, nframes;
	register short p,q;
	
	if (x->l_obj.z_disabled)
		goto out;
	if (!b)
		goto zero;
	if (!b->b_valid)
		goto zero;
	tab = b->b_samples;
	frames = b->b_frames;
	nc = b->b_nchans;
	fsize=x->l_framesize;
	nframes = x->l_numframes;
	hopsize = x->l_hopsize;
	while (--n) {
		x_coords = *++inx;
		y_coords = *++iny; 
		if (y_coords<0.) y_coords = 0.;
		if (y_coords>1.0) y_coords = 1.0; // constrain y coordinates to 0 to 1
		index = x_coords*fsize;
		if (index<0) index = 0;
		if (index>=fsize) index = fsize-1; // constrain index coordinates to the frame boundaries
		// check buffer~ boundaries
		if (index >= frames)
			index = frames - 1;
		if (index+((nframes-1)*fsize) >= frames) // out of bounds
			goto zero;
		// figure out which two frames we're between...
		q=0;
		while (++q<nframes) {
			if (y_coords<=(hopsize*q)) {
				p = q-1;
				q=nframes;
			}
		}
		index = index+(p*fsize); // current frame
		index2 = index+fsize; // one frame later
		if (nc > 1) {
			index = index * nc;
			index2 = index2 * nc;
		}
		y_coords = (y_coords*(nframes-1))-p;
		*++out = ((1.-y_coords)*tab[index])+(y_coords*tab[index2]); // this is wrong -- y_coords amp isn't scaling yet.
	}
	return (w+6);
zero:
	while (--n) *++out = 0.;
out:
	return (w+6);
}

void terrain_set(t_terrain *x, t_symbol *s)
{
	t_buffer *b;
	
	x->l_sym = s;
	if ((b = (t_buffer *)(s->s_thing)) && ob_sym(b) == ps_buffer) {
		x->l_buf = b;
	} else {
		error("terrain~: no buffer~ %s", s->s_name);
		x->l_buf = 0;
	}
}

void terrain_size(t_terrain *x, long size)
{

if(size<1) size = 1;
x->l_framesize = size;

}

void terrain_frames(t_terrain *x, long frames)
{

if(frames<2) frames = 2;
if(frames>MAXFRAMES) frames = MAXFRAMES;
x->l_numframes=frames;
x->l_hopsize = 1./(float)(x->l_numframes-1);


}


void terrain_dsp(t_terrain *x, t_signal **sp)
{
    terrain_set(x,x->l_sym);
    dsp_add(terrain_perform, 5, x, sp[0]->s_vec-1, sp[1]->s_vec-1, sp[2]->s_vec-1, sp[0]->s_n+1);
}


void terrain_assist(t_terrain *x, void *b, long m, long a, char *s)
{
	switch(m) {
		case 1: // inlet
			switch(a) {
				case 0:
				sprintf(s, "(signal) X Index");
				break;
				case 1:
				sprintf(s, "(signal) Y Index");
				break;
			}
		break;
		case 2: // outlet
			switch(a) {
				case 0:
				sprintf(s, "(signal) Z Output");
				break;
			}
		break;
	}

}

void *terrain_new(t_symbol *s, long n, long f)
{
	t_terrain *x = (t_terrain *)newobject(terrain_class);
	dsp_setup((t_pxobject *)x, 2);
	outlet_new((t_object *)x, "signal");
	x->l_sym = s;
	x->l_framesize = 512;
	x->l_numframes = 2;
	if(n) x->l_framesize = n;
	if(f) x->l_numframes = IConstrain(f, 2, MAXFRAMES);
	x->l_hopsize = 1./(float)(x->l_numframes-1);
	return (x);
}
#endif /* MSP */

/* ------------------------------------- Pure Data ---------------------------------- */
#ifdef PD

#define t_buffer  t_garray	/* use t_garray instead of Max's t_buffer */

static t_class *terrain_class;

typedef struct _terrain
{
    t_object l_obj;
    t_symbol *l_sym;
    t_buffer *l_buf;
    long l_framesize;
    long l_numframes;
    t_float l_hopsize;
} t_terrain;


// static t_symbol *ps_buffer;


static long IConstrain(long v, long lo, long hi)
{
	if (v < lo)
		return lo;
	else if (v > hi)
		return hi;
	else
		return v;
}

static float FConstrain(float v, float lo, float hi)
{
	if (v < lo)
		return lo;
	else if (v > hi)
		return hi;
	else
		return v;
}

static t_int *terrain_perform(t_int *w)
{
    t_terrain *x = (t_terrain *)(w[1]);
    t_float *inx = (t_float *)(w[2]);
    t_float *iny = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    int n = (int)(w[5]);
	t_buffer *b = x->l_buf;
	float *tab;
	float x_coords, y_coords, hopsize;
	long index1, index2, chan, nc,fsize, nframes;
	  size_t frames;
	register short p = 0,q;
	
	if (!b)
		goto zero;
	/* changes suggested by Krzysztof */
		 if (!garray_getfloatarray(b, &frames, &tab))
		goto zero;
	nc = 1;
	/* end of changes */
	fsize=x->l_framesize;
	nframes = x->l_numframes;
	hopsize = x->l_hopsize;
	while (--n) {
		x_coords = *++inx;
		y_coords = *++iny; 
		if (y_coords<0.) y_coords = 0.;
		if (y_coords>1.0) y_coords = 1.0; // constrain y coordinates to 0 to 1
		index1 = x_coords*fsize;
		if (index1<0) index1 = 0;
		if (index1>=fsize) index1 = fsize-1; // constrain index coordinates to the frame boundaries
		// check buffer~ boundaries
		if (index1 >= (long)frames)
			index1 = frames - 1;
		if (index1+((nframes-1)*fsize) >= (long)frames) // out of bounds
			goto zero;
		// figure out which two frames we're between...
		q=0;
		while (++q<nframes) {
			if (y_coords<=(hopsize*q)) {
				p = q-1;
				q=nframes;
			}
		}
		index1 = index1+(p*fsize); // current frame
		index2 = index1+fsize; // one frame later
		if (nc > 1) {
			index1 = index1 * nc;
			index2 = index2 * nc;
		}
		y_coords = (y_coords*(nframes-1))-p;
		*++out = ((1.-y_coords)*tab[index1])+(y_coords*tab[index2]); // this is wrong -- y_coords amp isn't scaling yet.
	}
	return (w+6);
zero:
	while (--n) *++out = 0.;
	return (w+6);
}

static void terrain_set(t_terrain *x, t_symbol *s)
{
	t_buffer *b;
	
	x->l_sym = s;

	if ((b = (t_buffer *)pd_findbyclass(s, garray_class)))
	{
		x->l_buf = b;
	} else {
		pd_error(NULL, "terrain~: no buffer~ %s (error %d)", s->s_name, b);
		x->l_buf = 0;
	}
}

static void terrain_size(t_terrain *x, t_floatarg size)
{
	if(size<1) size = 1;
	x->l_framesize = size;
}

static void terrain_frames(t_terrain *x, t_floatarg frames)
{
	if(frames<2) frames = 2;
	if(frames>MAXFRAMES) frames = MAXFRAMES;
	x->l_numframes=frames;
	x->l_hopsize = 1./(t_float)(x->l_numframes-1);
}


static void terrain_dsp(t_terrain *x, t_signal **sp)
{
    terrain_set(x,x->l_sym);
    dsp_add(terrain_perform, 5, x, sp[0]->s_vec-1, sp[1]->s_vec-1, sp[2]->s_vec-1, sp[0]->s_n+1);
}



static void *terrain_new(t_symbol *s, t_floatarg n, t_floatarg f)
{
	t_terrain *x = (t_terrain *)pd_new(terrain_class);

	outlet_new(&x->l_obj, gensym("signal"));
	inlet_new(&x->l_obj, &x->l_obj.ob_pd, gensym ("signal"), gensym ("signal"));
	x->l_sym = s;
	x->l_framesize = 512;
	x->l_numframes = 2;
	if(n) x->l_framesize = n;
	if(f) x->l_numframes = IConstrain(f, 2, MAXFRAMES);
	x->l_hopsize = 1./(t_float)(x->l_numframes-1);
	return (x);
}

void terrain_tilde_setup(void)
{
	terrain_class = class_new(gensym("terrain~"), (t_newmethod)terrain_new, 0,
        sizeof(t_terrain), 0, A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(terrain_class, nullfn, gensym("signal"), A_NULL);
    class_addmethod(terrain_class, (t_method)terrain_dsp, gensym("dsp"), A_NULL);
    class_addmethod(terrain_class, (t_method)terrain_set, gensym("set"), A_SYMBOL, 0);
    class_addmethod(terrain_class, (t_method)terrain_size, gensym("size"), A_FLOAT, 0);
    class_addmethod(terrain_class, (t_method)terrain_frames, gensym("frames"), A_FLOAT, 0);
//	ps_buffer = gensym("buffer~");
    class_sethelpsymbol(terrain_class, gensym("help-terrain~.pd"));
}
#endif /* PD */
