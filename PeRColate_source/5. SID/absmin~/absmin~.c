// returns the minimum value of a signal based on absolute values.
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
void *sigabsmin_class;

typedef struct _sigabsmin
{
    t_pxobject x_obj;
    t_float x_val;
} t_sigabsmin;

void *sigabsmin_new(double val);
t_int *offset_perform(t_int *w);
t_int *sigabsmin2_perform(t_int *w);
void sigabsmin_float(t_sigabsmin *x, double f);
void sigabsmin_int(t_sigabsmin *x, long n);
void sigabsmin_dsp(t_sigabsmin *x, t_signal **sp, short *count);
void sigabsmin_assist(t_sigabsmin *x, void *b, long m, long a, char *s);

void main(void)
{
    setup(&sigabsmin_class, sigabsmin_new, (method)dsp_free, (short)sizeof(t_sigabsmin), 0L, A_DEFFLOAT, 0);
    addmess((method)sigabsmin_dsp, "dsp", A_CANT, 0);
    addfloat((method)sigabsmin_float);
    addint((method)sigabsmin_int);
    addmess((method)sigabsmin_assist,"assist",A_CANT,0);
    dsp_initclass();
    
    post("absmin~: by r. luke dubois, cmc");
}

void sigabsmin_assist(t_sigabsmin *x, void *b, long m, long a, char *s)
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

void *sigabsmin_new(double val)
{
    t_sigabsmin *x = (t_sigabsmin *)newobject(sigabsmin_class);
    dsp_setup((t_pxobject *)x,2);
    outlet_new((t_pxobject *)x, "signal");
    x->x_val = val;
    return (x);
}

// this routine covers both inlets. It doesn't matter which one is involved

void sigabsmin_float(t_sigabsmin *x, double f)
{
	x->x_val = f;
}

void sigabsmin_int(t_sigabsmin *x, long n)
{
	x->x_val = (float)n;
}

t_int *offset_perform(t_int *w)
{
    t_float *in = (t_float *)(w[1]);
    t_float *out = (t_float *)(w[2]);
	t_sigabsmin *x = (t_sigabsmin *)(w[3]);
	float val2 = fabs(x->x_val);
	float val1;
	int n = (int)(w[4]);
	
	if (x->x_obj.z_disabled)
		goto out;
	
	while (--n) {
		val1 = *++in;
    	if (fabs(val1)<=fabs(val2)) {
	    		*++out = val1;
    	}
		else {
    		*++out = val2;
    	}
	}
    
out:
    return (w+5);
}

t_int *sigabsmin2_perform(t_int *w)
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
    	if (fabs(val1)<=fabs(val2)) {
	    		*++out = val1;
    	}
		else {
    		*++out = val2;
    	}
	}
out:
	return (w+6);
}		

void sigabsmin_dsp(t_sigabsmin *x, t_signal **sp, short *count)
{
	long i;
		
	if (!count[0])
		dsp_add(offset_perform, 4, sp[1]->s_vec-1, sp[2]->s_vec-1, x, sp[0]->s_n+1);
	else if (!count[1])
		dsp_add(offset_perform, 4, sp[0]->s_vec-1, sp[2]->s_vec-1, x, sp[0]->s_n+1);
	else
		dsp_add(sigabsmin2_perform, 5, &x->x_obj.z_disabled, sp[0]->s_vec-1, sp[1]->s_vec-1, sp[2]->s_vec-1, sp[0]->s_n+1);
}
#endif /* MSP */
/* ----------------------------------- Pure Data ------------------------------- */
#ifdef PD
static t_class *sigabsmin_class;

typedef struct _sigabsmin
{
    t_object x_obj;
    t_float x_val;
	t_int x_nochannel; /* number of channels */
} t_sigabsmin;


static void sigabsmin_float(t_sigabsmin *x, t_floatarg f)
{
	x->x_val = f;
}


static t_int *offset_perform(t_int *w)
{
    t_float *in = (t_float *)(w[1]);
    t_float *out = (t_float *)(w[2]);
	t_sigabsmin *x = (t_sigabsmin *)(w[3]);
	float val2 = fabs(x->x_val);
	float val1;
	int n = (int)(w[4]);
	
	while (--n) {
		val1 = *++in;
    	if (fabs(val1)<=fabs(val2)) {
	    		*++out = val1;
    	}
		else {
    		*++out = val2;
    	}
	}
    return (w+5);
}

static t_int *sigabsmin2_perform(t_int *w)
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
    	if (fabs(val1)<=fabs(val2)) {
	    		*++out = val1;
    	}
		else {
    		*++out = val2;
    	}
	}
	return (w+5);
}		

static void sigabsmin_dsp(t_sigabsmin *x, t_signal **sp)
{
	if (x->x_nochannel == 1)
		dsp_add(offset_perform, 4, sp[0]->s_vec-1, sp[1]->s_vec-1, x, sp[0]->s_n+1);
	else /* two cahnnels */
		dsp_add(sigabsmin2_perform, 4, sp[0]->s_vec-1, sp[1]->s_vec-1, sp[2]->s_vec-1, sp[0]->s_n+1);
}

static void *sigabsmin_new(t_floatarg val)
{
    t_sigabsmin *x = (t_sigabsmin *)pd_new(sigabsmin_class);
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
    post("absmin~: by r. luke dubois, cmc");
    return (x);
}

void absmin_tilde_setup(void)
{
    sigabsmin_class = class_new(gensym("absmin~"), (t_newmethod)sigabsmin_new, 0,
        sizeof(t_sigabsmin), 0, A_DEFFLOAT, 0);
    class_addmethod(sigabsmin_class, nullfn, gensym("signal"), A_NULL);
    class_addmethod(sigabsmin_class, (t_method)sigabsmin_dsp, gensym("dsp"), A_NULL);
	class_addfloat(sigabsmin_class, (t_method)sigabsmin_float);
    class_addmethod(sigabsmin_class, (t_method)sigabsmin_float, gensym("value"), A_FLOAT, A_NULL);
    class_sethelpsymbol(sigabsmin_class, gensym("help-absmin~.pd"));
}
#endif
