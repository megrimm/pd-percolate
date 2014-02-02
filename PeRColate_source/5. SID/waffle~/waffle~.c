// waffle~ -- interpolates between two or more wavetable 'frames' stored in a buffer.
// ported to Pd by Olaf Matthes (olaf.matthes@gmx.de), 2002 

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

#define MAXFRAMES 32

#ifdef MSP

void *waffle_class;

typedef struct _waffle
{
    t_pxobject l_obj;
} t_waffle;

float FConstrain(float v, float lo, float hi);
long IConstrain(long v, long lo, long hi);
t_int *waffle_perform(t_int *w);
void waffle_dsp(t_waffle *x, t_signal **sp);
void *waffle_new(void);
void waffle_assist(t_waffle *x, void *b, long m, long a, char *s);
#endif /* MSP */

#ifdef PD
static t_class *waffle_class;

typedef struct _waffle
{
    t_object l_obj;
} t_waffle;
#endif

t_symbol *ps_buffer;

#ifdef MSP
long IConstrain(long v, long lo, long hi)
#endif
#ifdef PD
static long IConstrain(long v, long lo, long hi)
#endif
{
	if (v < lo)
		return lo;
	else if (v > hi)
		return hi;
	else
		return v;
}

#ifdef MSP
float FConstrain(float v, float lo, float hi)
#endif
#ifdef PD
static float FConstrain(float v, float lo, float hi)
#endif
{
	if (v < lo)
		return lo;
	else if (v > hi)
		return hi;
	else
		return v;
}

#ifdef MSP
t_int *waffle_perform(t_int *w)
#endif
#ifdef PD
static t_int *waffle_perform(t_int *w)
#endif
{
    t_waffle *x = (t_waffle *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *cross = (t_float *)(w[3]);
    t_float *sync = (t_float *)(w[4]);
    t_float *out1 = (t_float *)(w[5]);
    t_float *out2 = (t_float *)(w[6]);
    int n = (int)(w[7]);
#ifdef MSP
	if (x->l_obj.z_disabled)
	goto out;
#endif

	while (--n) {
		if(*++sync<*++cross) {
		*++out1 = *++in;
		*++out2 = 0.;
		}
		else {
		*++out1 = 0;
		*++out2 = *++in;
		}
	}
	return (w+8);
#ifdef MSP
out:
	return (w+8);
#endif
}

#ifdef MSP
void waffle_dsp(t_waffle *x, t_signal **sp)
{
    dsp_add(waffle_perform, 7, x, sp[0]->s_vec-1, sp[1]->s_vec-1, sp[2]->s_vec-1, sp[3]->s_vec-1, sp[4]->s_vec-1, sp[0]->s_n+1);
}
#endif
#ifdef PD
static void waffle_dsp(t_waffle *x, t_signal **sp)
{
    dsp_add(waffle_perform, 7, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
}
#endif

#ifdef MSP
void waffle_assist(t_waffle *x, void *b, long m, long a, char *s)
{
	switch(m) {
		case 1: // inlet
			switch(a) {
				case 0:
				sprintf(s, "(signal) Input");
				break;
				case 1:
				sprintf(s, "(signal) Crossover");
				break;
				case 2:
				sprintf(s, "(signal) Sync");
				break;
			}
		break;
		case 2: // outlet
			switch(a) {
				case 0:
				sprintf(s, "(signal) Low Output");
				break;
				case 1:
				sprintf(s, "(signal) High Output");
				break;
			}
		break;
	}

}
#endif /* MSP */

#ifdef MSP
void *waffle_new(void)
{
	t_waffle *x = (t_waffle *)newobject(waffle_class);
	dsp_setup((t_pxobject *)x, 3);
	outlet_new((t_object *)x, "signal");
	outlet_new((t_object *)x, "signal");
#endif /* MSP */
#ifdef PD
static void *waffle_new(void)
{
	t_waffle *x = (t_waffle *)pd_new(waffle_class);
	outlet_new(&x->l_obj, gensym("signal"));
	outlet_new(&x->l_obj, gensym("signal"));
    inlet_new (&x->l_obj, &x->l_obj.ob_pd, gensym ("signal"), gensym ("signal"));
    inlet_new (&x->l_obj, &x->l_obj.ob_pd, gensym ("signal"), gensym ("signal"));
#endif /* PD */
	post("waffle waffle waffle...");
	return (x);
}


#ifdef MSP
void main(void)
{
	setup(&waffle_class, waffle_new, (method)dsp_free, (short)sizeof(t_waffle), 0L, 0);
	addmess((method)waffle_dsp, "dsp", A_CANT, 0);
	addmess((method)waffle_assist, "assist", A_CANT, 0);
	dsp_initclass();
}
#endif /* MSP */

#ifdef PD
void waffle_tilde_setup(void)
{
	waffle_class = class_new(gensym("waffle~"), (t_newmethod)waffle_new, 0,
        sizeof(t_waffle), 0, 0);
    class_addmethod(waffle_class, nullfn, gensym("signal"), A_NULL);
    class_addmethod(waffle_class, (t_method)waffle_dsp, gensym("dsp"), A_NULL);
    class_sethelpsymbol(waffle_class, gensym("help-waffle~.pd"));
}
#endif /* PD */
