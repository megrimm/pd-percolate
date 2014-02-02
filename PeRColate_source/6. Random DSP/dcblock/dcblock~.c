/* ported to PD by Olaf Matthes, 2002 */
#ifdef MSP
#include "ext.h"
#include "z_dsp.h"
#endif
#ifdef PD
#include "m_pd.h"
#endif
#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define TWOPI 6.283185307

/* ---------------------------- MSP ------------------- */
#ifdef MSP
void *dcblock_class;

typedef struct _dcblock
{
	//header
    t_pxobject x_obj;

    //DC blocker
    float dc_input, dc_output;
    
    float srate, one_over_srate;
} t_dcblock;

/**** PROTOTYPES for MSP ****/
//setup funcs
void *dcblock_new(double val);
void dcblock_dsp(t_dcblock *x, t_signal **sp, short *count); 
t_int *dcblock_perform(t_int *w);
void dcblock_assist(t_dcblock *x, void *b, long m, long a, char *s);

//DC blocker
float dcblock_tick(t_dcblock *x, float sample);

/****ASSIST FUNCTION FOR MSP****/

void dcblock_assist(t_dcblock *x, void *b, long m, long a, char *s)
{
	assist_string(3214,m,a,1,2,s);
}

//DC blocker
float dcblock_tick(t_dcblock *x, float sample)
{
	x->dc_output = sample - x->dc_input + (0.99 * x->dc_output);
	x->dc_input = sample;
	return x->dc_output;
}

//primary MSP funcs
void *dcblock_new(double initial_coeff)
{
	int i;

    t_dcblock *x = (t_dcblock *)newobject(dcblock_class);
    //zero out the struct, to be careful (takk to jkclayton)
    if (x) { 
        for(i=sizeof(t_pxobject);i<sizeof(t_dcblock);i++)  
                ((char *)x)[i]=0; 
	} 
	
    dsp_setup((t_pxobject *)x,1);
    outlet_new((t_object *)x, "signal");

    x->srate = sys_getsr();
    x->one_over_srate = 1./x->srate;
    
    x->dc_input = 0.; x->dc_output = 0.;
    
    return (x);
}

t_int *dcblock_perform(t_int *w)
{
	t_dcblock *x = (t_dcblock *)(w[1]);

	float *in = (float *)(w[2]);
	float *out = (float *)(w[3]);
	long n = w[4];
	
	while(n--) {		
		*out++ = dcblock_tick(x, *in++);
	}
	return w + 5;
}

void dcblock_dsp(t_dcblock *x, t_signal **sp, short *count)
{
	x->srate = sp[0]->s_sr;
	x->one_over_srate = 1./x->srate;

	dsp_add(dcblock_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);	
}

void main(void)
{
    setup((struct messlist **)&dcblock_class, (method)dcblock_new, (method)dsp_free, (short)sizeof(t_dcblock), 0L, A_DEFFLOAT, 0);
    addmess((method)dcblock_dsp, "dsp", A_CANT, 0);
    addmess((method)dcblock_assist,"assist",A_CANT,0);
    dsp_initclass();
    rescopy('STR#',3214);
}
#endif /* MSP */


/* ---------------------------- Pure Data ------------------------ */
#ifdef PD

static t_class *dcblock_class;

typedef struct _dcblock
{
	//header
	t_object x_obj;

    //DC blocker
    t_float dc_input;
	t_float dc_output;
    
    t_float srate, one_over_srate;
} t_dcblock;


//DC blocker
static t_float dcblock_tick(t_dcblock *x, t_float sample)
{
	x->dc_output = sample - x->dc_input + (0.99 * x->dc_output);
	x->dc_input = sample;
	return x->dc_output;
}

//primary MSP funcs

static t_int *dcblock_perform(t_int *w)
{
	t_dcblock *x = (t_dcblock *)(w[1]);

	t_float *in = (t_float *)(w[2]);
	t_float *out = (t_float *)(w[3]);
	long n = w[4];
	
	while(n--) {		
		*out++ = dcblock_tick(x, *in++);
	}
	return w + 5;
}

static void dcblock_dsp(t_dcblock *x, t_signal **sp)
{
	x->srate = sp[0]->s_sr;
	x->one_over_srate = 1./x->srate;

	dsp_add(dcblock_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);	
}

static void *dcblock_new(void)
{
	unsigned int i;
    t_dcblock *x = (t_dcblock *)pd_new(dcblock_class);
    //zero out the struct, to be careful (takk to jkclayton)
    if (x) { 
        for(i=sizeof(t_object);i<sizeof(t_dcblock);i++)  
                ((char *)x)[i]=0; 
	} 
	
	outlet_new(&x->x_obj, gensym("signal"));

    x->srate = sys_getsr();
    x->one_over_srate = 1./x->srate;
    
    x->dc_input = 0.; x->dc_output = 0.;
    
    return (x);
}

void dcblock_tilde_setup(void)
{
	dcblock_class = class_new(gensym("dcblock~"), (t_newmethod)dcblock_new, 0,
        sizeof(t_dcblock), 0, 0);
    class_addmethod(dcblock_class, nullfn, gensym("signal"), A_NULL);
    class_addmethod(dcblock_class, (t_method)dcblock_dsp, gensym("dsp"), A_NULL);
    class_sethelpsymbol(dcblock_class, gensym("help-dcblock~.pd"));
}
#endif /* PD */

