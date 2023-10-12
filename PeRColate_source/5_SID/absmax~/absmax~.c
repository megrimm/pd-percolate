// returns the maximum value of a signal based on absolute values.
// by r. luke dubois, cmc/cu, 2001.
// ported to Pd by Olaf Matthes (olaf.matthes@gmx.de), 2002

#include <math.h>
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

#ifdef MSP
void *sigabsmax_class;

typedef struct _sigabsmax
{
    t_pxobject x_obj;
    t_float x_val;
} t_sigabsmax;

void *sigabsmax_new(double val);
t_int *offset_perform(t_int *w);
t_int *sigabsmax2_perform(t_int *w);
void sigabsmax_float(t_sigabsmax *x, double f);
void sigabsmax_int(t_sigabsmax *x, long n);
void sigabsmax_dsp(t_sigabsmax *x, t_signal **sp, short *count);
void sigabsmax_assist(t_sigabsmax *x, void *b, long m, long a, char *s);

void main(void)
{
    setup(&sigabsmax_class, sigabsmax_new, (method)dsp_free, (short)sizeof(t_sigabsmax), 0L, A_DEFFLOAT, 0);
    addmess((method)sigabsmax_dsp, "dsp", A_CANT, 0);
    addfloat((method)sigabsmax_float);
    addint((method)sigabsmax_int);
    addmess((method)sigabsmax_assist,"assist",A_CANT,0);
    dsp_initclass();
    
    post("absmax~: by r. luke dubois, cmc");
}

void sigabsmax_assist(t_sigabsmax *x, void *b, long m, long a, char *s)
{
	switch(m) {
		case 1: // inlet
			switch(a) {
				case 0:
				sprintf(s, "(Signal) Input 1");
				break;
				case 1:
				sprintf(s, "(Signal) Input 2");
				break;
			}
		break;
		case 2: // outlet
			switch(a) {
				case 0:
				sprintf(s, "(Signal) Output");
				break;
			}
		break;
	}
}

void *sigabsmax_new(double val)
{
    t_sigabsmax *x = (t_sigabsmax *)newobject(sigabsmax_class);
    dsp_setup((t_pxobject *)x,2);
    outlet_new((t_pxobject *)x, "signal");
    x->x_val = val;
    return (x);
}

// this routine covers both inlets. It doesn't matter which one is involved

void sigabsmax_float(t_sigabsmax *x, double f)
{
	x->x_val = f;
}

void sigabsmax_int(t_sigabsmax *x, long n)
{
	x->x_val = (float)n;
}

t_int *offset_perform(t_int *w)
{
    t_float *in = (t_float *)(w[1]);
    t_float *out = (t_float *)(w[2]);
	t_sigabsmax *x = (t_sigabsmax *)(w[3]);
	float val2 = fabs(x->x_val);
	float val1;
	int n = (int)(w[4]);
	
	if (x->x_obj.z_disabled)
		goto out;
	
	while (--n) {
		val1 = *++in;
    	if (fabs(val1)>=fabs(val2)) {
	    		*++out = val1;
    	}
		else {
    		*++out = val2;
    	}
	}
    
out:
    return (w+5);
}

t_int *sigabsmax2_perform(t_int *w)
{
	t_float *in1,*in2,*out;
	int n;
	float val1, val2;

	if (*(long *)(w[1]))
	    goto out;

	in1 = (t_float *)(w[2]);
	in2 = (t_float *)(w[3]);
	out = (t_float *)(w[4]);
	n = (int)(w[5]);
	while (--n) {
		val1 = *++in1;
		val2 = *++in2;
    	if (fabs(val1)>=fabs(val2)) {
	    		*++out = val1;
    	}
		else {
    		*++out = val2;
    	}
	}
out:
	return (w+6);
}		

void sigabsmax_dsp(t_sigabsmax *x, t_signal **sp, short *count)
{
	long i;
		
	if (!count[0])
		dsp_add(offset_perform, 4, sp[1]->s_vec-1, sp[2]->s_vec-1, x, sp[0]->s_n+1);
	else if (!count[1])
		dsp_add(offset_perform, 4, sp[0]->s_vec-1, sp[2]->s_vec-1, x, sp[0]->s_n+1);
	else
		dsp_add(sigabsmax2_perform, 5, &x->x_obj.z_disabled, sp[0]->s_vec-1, sp[1]->s_vec-1, sp[2]->s_vec-1, sp[0]->s_n+1);
}
#endif /* MSP */
/* ----------------------------------- Pure Data ------------------------------- */
#ifdef PD
static t_class *sigabsmax_class;

typedef struct _sigabsmax
{
    t_object x_obj;
    t_float x_val;
	t_int x_nochannel; /* number of channels */
} t_sigabsmax;


static void sigabsmax_float(t_sigabsmax *x, t_floatarg f)
{
	x->x_val = f;
}


static t_int *offset_perform(t_int *w)
{
    t_float *in = (t_float *)(w[1]);
    t_float *out = (t_float *)(w[2]);
	t_sigabsmax *x = (t_sigabsmax *)(w[3]);
	float val2 = fabs(x->x_val);
	float val1;
	int n = (int)(w[4]);
	
	while (--n) {
		val1 = *++in;
    	if (fabs(val1)>=fabs(val2)) {
	    		*++out = val1;
    	}
		else {
    		*++out = val2;
    	}
	}
    return (w+5);
}

static t_int *sigabsmax2_perform(t_int *w)
{
	t_float *in1,*in2,*out;
	int n;
	float val1, val2;

	in1 = (t_float *)(w[1]);
	in2 = (t_float *)(w[2]);
	out = (t_float *)(w[3]);
	n = (int)(w[4]);
	while (--n) {
		val1 = *++in1;
		val2 = *++in2;
    	if (fabs(val1)>=fabs(val2)) {
	    		*++out = val1;
    	}
		else {
    		*++out = val2;
    	}
	}
	return (w+5);
}		

static void sigabsmax_dsp(t_sigabsmax *x, t_signal **sp)
{
	if (x->x_nochannel == 1)
		dsp_add(offset_perform, 4, sp[0]->s_vec-1, sp[1]->s_vec-1, x, sp[0]->s_n+1);
	else /* two cahnnels */
		dsp_add(sigabsmax2_perform, 4, sp[0]->s_vec-1, sp[1]->s_vec-1, sp[2]->s_vec-1, sp[0]->s_n+1);
}

static void *sigabsmax_new(t_floatarg val)
{
    t_sigabsmax *x = (t_sigabsmax *)pd_new(sigabsmax_class);
	outlet_new(&x->x_obj, gensym("signal"));
    x->x_val = val;
	if(!val)
	{
		x->x_nochannel = 2;
		inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym ("signal"), gensym ("signal"));
	}
	else
	{
		x->x_nochannel = 1;
		inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("value"));
	}
    return (x);
    post("absmax~: by r. luke dubois, cmc");
}

void absmax_tilde_setup(void)
{
    sigabsmax_class = class_new(gensym("absmax~"), (t_newmethod)sigabsmax_new, 0,
        sizeof(t_sigabsmax), 0, A_DEFFLOAT, 0);
    class_addmethod(sigabsmax_class, nullfn, gensym("signal"), A_NULL);
    class_addmethod(sigabsmax_class, (t_method)sigabsmax_dsp, gensym("dsp"), A_NULL);
	class_addfloat(sigabsmax_class, (t_method)sigabsmax_float);
    class_addmethod(sigabsmax_class, (t_method)sigabsmax_float, gensym("value"), A_FLOAT, A_NULL);
    class_sethelpsymbol(sigabsmax_class, gensym("help-absmax~.pd"));
}
#endif
