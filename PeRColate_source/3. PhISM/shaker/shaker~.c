#ifdef MSP
#include "ext.h"
#include "z_dsp.h"
#endif
#ifdef PD
#include "m_pd.h"
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define TWOPI 6.283185307
#define ONE_OVER_RANDLIMIT 0.00006103516 // constant = 1. / 16384.0
#define ATTACK 0
#define DECAY 1
#define SUSTAIN 2
#define RELEASE 3
#define MAX_RANDOM 32768


void *shaker_class;

typedef struct _shaker
{
	//header
#ifdef MSP
    t_pxobject x_obj;
#endif
#ifdef PD
  t_object x_obj;
#endif    

    //user controlled vars
    long num_beans;	//number of beans	
    float res_freq;	//resonance
    float shake_damp; 	//damping
    float shake_speed;
    long shake_times;
    float shake_max;
    
    long num_beansSave;	//number of beans	
    float res_freqSave;	//resonance
    float shake_dampSave; 	//damping
    float shake_speedSave;
    long shake_timesSave;
    float shake_maxSave;
    
    //other vars
    long wait_time;
    long shake_num;
    float coll_damp;
    float shakeEnergy;
    float shakeVel;
    float noiseGain;
    float gain_norm;
        
    //signals connected? or controls...
    short num_beansConnected;
    short res_freqConnected;
    short shake_dampConnected;
    short shake_speedConnected;
    short shake_timesConnected;
    short shake_maxConnected;
    
    //biquad stuff
    float bq_inputs[2];
    float bq_lastOutput;
    float bq_zeroCoeffs[2], bq_poleCoeffs[2];
    float bq_gain;
    
    //ADSR stuff
    float e_target;
    float e_value;
    float e_rate;
    float e_attackRate;
    float e_decayRate;
    float e_sustainLevel;
    float e_releaseRate;
    float e_state;

    float srate, one_over_srate;
} t_shaker;

/****PROTOTYPES****/

//setup funcs
static void *shaker_new(double val);
static void shaker_dsp(t_shaker *x, t_signal **sp, short *count);
static void shaker_float(t_shaker *x, double f);
static void shaker_int(t_shaker *x, int f);
static void shaker_bang(t_shaker *x);
static t_int *shaker_perform(t_int *w);
static void shaker_assist(t_shaker *x, void *b, long m, long a, char *s);

//noise maker
static float noise_tick(void);

//biquad funcs
static void bq_setFreqAndReson(t_shaker *x, float freq, float reson);
static void bq_setEqualGainZeros(t_shaker *x);
static void bq_setGain(t_shaker *x, float gain);
static float bq_tick(t_shaker *x, float sample);

//ADSR funcs
static void e_keyOn(t_shaker *x);
static void e_keyOff(t_shaker *x);
static void e_setAttackRate(t_shaker *x, float rate);
static void e_setDecayRate(t_shaker *x, float rate);
static void e_setSustainLevel(t_shaker *x, float level);
static void e_setReleaseRate(t_shaker *x, float rate);
static void e_setAll(t_shaker *x, float attRate, float decRate, float susLevel, float relRate);
static float e_tick(t_shaker *x);

//shaker funcs
static void shake(t_shaker *x, float amplitude);
static void setFreq(t_shaker *x, float freq);
static void noteOn(t_shaker *x, float freq, float amp);
static void noteOff(t_shaker *x, float amplitude);
static long one_in(long one_in_howmany);

/****FUNCTIONS****/

//shaker funcs
static void shake(t_shaker *x, float amplitude)
{
	x->shake_max = 2. * amplitude;
	x->shake_speed = .0008 + (amplitude * .0004);
	x->shake_num = x->shake_times;
	e_setAll(x, x->shake_speed, x->shake_speed, 0., x->shake_speed);
	e_keyOn(x);
}

static void setFreq(t_shaker *x, float freq)
{
	bq_setFreqAndReson(x, freq, 0.96);
}

static void noteOn(t_shaker *x, float freq, float amp)
{
	bq_setFreqAndReson(x, freq, 0.96);
	shake(x, amp);
}

static void noteOff(t_shaker *x, float amplitude)
{
	x->shake_num = 0;
}

static long one_in(long one_in_howmany)
{
	if(rand() < one_in_howmany) return 1;
	else return 0;
}


//ADSR funcs
static void e_keyOn(t_shaker *x)
{
	x->e_target = 1.;
	x->e_rate = x->e_attackRate;
	x->e_state = ATTACK;
}

static void e_keyOff(t_shaker *x)
{
	x->e_target = 0.;
	x->e_rate = x->e_releaseRate;
	x->e_state = RELEASE;
}

static void e_setAttackRate(t_shaker *x, float rate)
{
	if(rate < 0.) rate = -rate;
	x->e_attackRate = rate * 22050./x->srate;
}

static void e_setDecayRate(t_shaker *x, float rate)
{
	if(rate < 0.) rate = -rate;
	x->e_decayRate = rate;
}

static void e_setSustainLevel(t_shaker *x, float level)
{
	if(level < 0.) level = 0.;
	x->e_sustainLevel = level;
}

static void e_setReleaseRate(t_shaker *x, float rate)
{
	if(rate < 0.) rate = -rate;
	x->e_releaseRate = rate * 22050./x->srate;
}

static void e_setAll(t_shaker *x, float attRate, float decRate, float susLevel, float relRate)
{
	 e_setAttackRate(x, attRate);
	 e_setDecayRate(x,  decRate);
	 e_setSustainLevel(x,  susLevel);
	 e_setReleaseRate(x,  relRate);
}
	
static float e_tick(t_shaker *x)
{
	if(x->e_state == ATTACK) {
		x->e_value += x->e_rate;
		if(x->e_value > x->e_target) {
			x->e_value = x->e_target;
			x->e_rate = x->e_decayRate;
			x->e_target = x->e_sustainLevel;
			x->e_state = DECAY;
		}
	}
	else if(x->e_state == DECAY) {
		x->e_value -= x->e_decayRate;
		if(x->e_value <= x->e_sustainLevel) {
			x->e_value = x->e_sustainLevel;
			x->e_rate = 0.;
			x->e_state = SUSTAIN;	
		}
	}
	else if(x->e_state == RELEASE) {
		x->e_value -= x->e_releaseRate;
		if(x->e_value <= 0.) {
			x->e_value = 0.;
			x->e_state = 4;
		}
	}
	return x->e_value;
}

static void bq_setFreqAndReson(t_shaker *x, float freq, float reson)
{
	x->bq_poleCoeffs[1] = -(reson*reson);
	x->bq_poleCoeffs[0] = 2. * reson * cos(TWOPI * freq / x->srate);
}

static void bq_setEqualGainZeros(t_shaker *x)
{
	x->bq_zeroCoeffs[1] = -1.;
	x->bq_zeroCoeffs[0] = 0.;
}

static void bq_setGain(t_shaker *x, float gain)
{
	x->bq_gain = gain;
}

static float bq_tick(t_shaker *x, float sample)
{
	float temp;
	float output;
	
	temp = sample * x->bq_gain;
	temp += x->bq_inputs[0] * x->bq_poleCoeffs[0];
	temp += x->bq_inputs[1] * x->bq_poleCoeffs[1];
	
	output = temp;
	output += x->bq_inputs[0] * x->bq_zeroCoeffs[0];
	output += x->bq_inputs[1] * x->bq_zeroCoeffs[1];
	x->bq_inputs[1] = x->bq_inputs[0];
	x->bq_inputs[0] = temp;
	
	x->bq_lastOutput = output;
	
	return output;
}


//noise maker
static float noise_tick(void) 
{
	float output;
	output = (float)rand() - 16384.;
	output *= ONE_OVER_RANDLIMIT;
	return output;
}

#ifdef MSP
//primary MSP funcs
void main(void)
{
    setup((struct messlist **)&shaker_class, (method)shaker_new, (method)dsp_free, (short)sizeof(t_shaker), 0L, A_DEFFLOAT, 0);
    addmess((method)shaker_dsp, "dsp", A_CANT, 0);
    addmess((method)shaker_assist,"assist",A_CANT,0);
    addfloat((method)shaker_float);
    addint((method)shaker_int);
    addbang((method)shaker_bang);
    dsp_initclass();
    rescopy('STR#',9803);
}

static void shaker_assist(t_shaker *x, void *b, long m, long a, char *s)
{
	assist_string(9803,m,a,1,6,s);
}

static void shaker_float(t_shaker *x, double f)
{
	if (x->x_obj.z_in == 0) {
		x->num_beans = (long)f;
	} else if (x->x_obj.z_in == 1) {
		x->res_freq = f;
	} else if (x->x_obj.z_in == 2) {
		//x->shake_damp = .98 + f*0.02;
		x->shake_damp = f;
	} else if (x->x_obj.z_in == 3) {
		//x->shake_speed = f * 0.002;
		x->shake_speed = f;
	} else if (x->x_obj.z_in == 4) {
		//x->shake_max = f * 2.;
		x->shake_max = f;
	}
}

static void shaker_int(t_shaker *x, int f)
{
	shaker_float(x, (double)f);
}
#endif

#ifdef PD
static void shaker_res_freq(t_shaker *x, double f);
static void shaker_shake_dump(t_shaker *x, double f);
static void shaker_shake_speed(t_shaker *x, double f);
static void shaker_shake_max(t_shaker *x, double f);

void shaker_tilde_setup(void)
{
  shaker_class = class_new(gensym("shaker~"), (t_newmethod)shaker_new, 0, (short)sizeof(t_shaker), 0L, A_DEFFLOAT, 0);
  class_addmethod(shaker_class, (t_method)shaker_dsp, gensym("dsp"), A_FLOAT, 0);
  class_addfloat(shaker_class, (t_method)shaker_float);
  class_addmethod(shaker_class, (t_method)shaker_res_freq, gensym("res_freq"), A_FLOAT, 0);
  class_addmethod(shaker_class, (t_method)shaker_shake_dump, gensym("shake_dump"), A_FLOAT, 0);
  class_addmethod(shaker_class, (t_method)shaker_shake_speed, gensym("shake_speed"), A_FLOAT, 0);
  class_addmethod(shaker_class, (t_method)shaker_shake_max, gensym("shake_max"), A_FLOAT, 0);
  class_addbang(shaker_class, (t_method)shaker_bang);
}

static void shaker_float(t_shaker *x, double f)
{
  x->num_beans = (long)f;
}
static void shaker_res_freq(t_shaker *x, double f)
{
  x->res_freq = f;
}
static void shaker_shake_dump(t_shaker *x, double f)
{
  //x->shake_damp = .98 + f*0.02;
  x->shake_damp = f;
}
static void shaker_shake_speed(t_shaker *x, double f)
{
  //x->shake_speed = f * 0.002;
  x->shake_speed = f;
}

static void shaker_shake_max(t_shaker *x, double f)
{
  //x->shake_max = f * 2.;
  x->shake_max = f;
}
#endif

static void shaker_bang(t_shaker *x)
{
	post("shaker: zeroing delay lines");
	x->bq_inputs[0] = 0.;
	x->bq_inputs[1] = 0.;
    x->bq_lastOutput = 0.;
}

#ifdef MSP
static void *shaker_new(double initial_coeff)
{
	int i;

    t_shaker *x = (t_shaker *)newobject(shaker_class);
    //zero out the struct, to be careful (takk to jkclayton)
    if (x) { 
        for(i=sizeof(t_pxobject);i<sizeof(t_shaker);i++)  
                ((char *)x)[i]=0; 
	} 
    dsp_setup((t_pxobject *)x,5);
    outlet_new((t_object *)x, "signal");
    
    x->srate = sys_getsr();
    x->one_over_srate = 1./x->srate;
    
    //clear stuff
 	x->bq_inputs[0] = x->bq_inputs[1] = x->bq_lastOutput = 0.;
 	
 	//init stuff
 	x->res_freq = 3200.;
 	bq_setFreqAndReson(x, x->res_freq, 0.96);
 	bq_setEqualGainZeros(x);
 	bq_setGain(x, 1.);
 	x->shake_speed = 0.001;
 	e_setAll(x, x->shake_speed, x->shake_speed, 0., x->shake_speed);
 	x->shakeEnergy = 0.;
 	x->noiseGain = 0.;
 	x->coll_damp = 0.95;
 	x->shake_damp = 0.999;
 	x->num_beans = 8;
 	x->wait_time = MAX_RANDOM / x->num_beans;
 	x->gain_norm = 0.0005;
 	x->shake_times = x->shake_num = 100;
 	
 	x->num_beansSave = -1;	
    x->res_freqSave = -1.;	
    x->shake_dampSave = -1.; 	
    x->shake_speedSave = -1.;
    x->shake_timesSave = -1;
    x->shake_maxSave = -1.;
 	
    srand(0.54);
    
    return (x);
}
#endif
#ifdef PD
static void *shaker_new(double initial_coeff)
{
	unsigned int i;

    t_shaker *x = (t_shaker *)pd_new(shaker_class);
    //zero out the struct, to be careful (takk to jkclayton)
    if (x) { 
        for(i=sizeof(t_object);i<sizeof(t_shaker);i++)  
                ((char *)x)[i]=0; 
	} 
    outlet_new((t_object *)x, gensym("signal"));
    
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("res_freq"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("shake_dump"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("shake_speed"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("shake_max"));

    x->srate = sys_getsr();
    x->one_over_srate = 1./x->srate;
    
    //clear stuff
 	x->bq_inputs[0] = x->bq_inputs[1] = x->bq_lastOutput = 0.;
 	
 	//init stuff
 	x->res_freq = 3200.;
 	bq_setFreqAndReson(x, x->res_freq, 0.96);
 	bq_setEqualGainZeros(x);
 	bq_setGain(x, 1.);
 	x->shake_speed = 0.001;
 	e_setAll(x, x->shake_speed, x->shake_speed, 0., x->shake_speed);
 	x->shakeEnergy = 0.;
 	x->noiseGain = 0.;
 	x->coll_damp = 0.95;
 	x->shake_damp = 0.999;
 	x->num_beans = 8;
 	x->wait_time = MAX_RANDOM / x->num_beans;
 	x->gain_norm = 0.0005;
 	x->shake_times = x->shake_num = 100;
 	
 	x->num_beansSave = -1;	
    x->res_freqSave = -1.;	
    x->shake_dampSave = -1.; 	
    x->shake_speedSave = -1.;
    x->shake_timesSave = -1;
    x->shake_maxSave = -1.;
 	
    srand(0.54);
    
    return (x);
}
#endif

static void shaker_dsp(t_shaker *x, t_signal **sp, short *count)
{
#ifdef MSP
	x->num_beansConnected = count[0];
	x->res_freqConnected = count[1];
	x->shake_dampConnected = count[2];
	x->shake_speedConnected = count[3];
	x->shake_maxConnected = count[4];

	x->srate = sp[0]->s_sr;
	x->one_over_srate = 1./x->srate;
	
	dsp_add(shaker_perform, 8, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, 
		    sp[4]->s_vec, sp[5]->s_vec, sp[0]->s_n);	
#endif
#ifdef PD	
	x->srate = sp[0]->s_sr;
	x->one_over_srate = 1./x->srate;
	
	dsp_add(shaker_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);	
#endif
}

static t_int *shaker_perform(t_int *w)
{
	t_shaker *x = (t_shaker *)(w[1]);
#ifdef MSP
	float num_beans		= x->num_beansConnected		? 	*(float *)(w[2]) : x->num_beans;
	float res_freq 		= x->res_freqConnected		? 	*(float *)(w[3]) : x->res_freq;
	float shake_damp 	= x->shake_dampConnected	? 	*(float *)(w[4]) : x->shake_damp;
	float shake_speed	= x->shake_speedConnected	? 	*(float *)(w[5]) : x->shake_speed;
	float shake_max 	= x->shake_maxConnected		? 	*(float *)(w[6]) : x->shake_max;
	
	float *out = (float *)(w[7]);
	long n = w[8];
#endif
#ifdef PD
	float num_beans		= x->num_beans;
	float res_freq 		= x->res_freq;
	float shake_damp 	= x->shake_damp;
	float shake_speed	= x->shake_speed;
	float shake_max 	= x->shake_max;
	
	float *out = (float *)(w[2]);
	long n = w[3];
#endif
	float temp;
	int state = x->e_state;
	long shake_num = x->shake_num;
	float gain_norm = x->gain_norm;
	long wait_time = x->wait_time;
	float lastOutput;
	float coll_damp = x->coll_damp;
	
	if(num_beans != x->num_beansSave) {
		num_beans = 129 - num_beans;
		x->num_beans = num_beans;
		x->num_beansSave = num_beans;
		wait_time = x->wait_time = MAX_RANDOM / num_beans;
	}
	
	if(res_freq != x->res_freqSave) {
		x->res_freqSave = x->res_freq = res_freq;
		bq_setFreqAndReson(x, res_freq, 0.96);
	}
	
	if(shake_damp != x->shake_dampSave) {
		x->shake_dampSave = x->shake_damp = (.98 + 0.02*shake_damp);
	}
	
	if(shake_speed != x->shake_speedSave) {
		x->shake_speedSave = x->shake_speed = shake_speed * .002;
		e_setAll(x, shake_speed, shake_speed, 0., shake_speed);
	}
	
	if(shake_max != x->shake_maxSave) {
		x->shake_maxSave = x->shake_max = 2.*shake_max;
		temp = (shake_max * 0.5 - 0.5) * 0.0002;
		temp += shake_speed;
		e_setAll(x, temp, temp, 0., temp);
		e_keyOn(x);
	}	

	while(n--) {
		temp = e_tick(x) * shake_max;
		if(shake_num > 0) {
			if(state == SUSTAIN) {
				if(shake_num < 64) {
					shake_num -= 1;
					x->shake_num -= 1;
				}
			}
		}
		if(temp > x->shakeEnergy) x->shakeEnergy = temp;
		x->shakeEnergy *= shake_damp;
		
		if(one_in(wait_time) == 1) {
			x->noiseGain += gain_norm * x->shakeEnergy * num_beans;
		}
		
		lastOutput = x->noiseGain * noise_tick();
		x->noiseGain *= coll_damp;
		lastOutput = bq_tick(x, lastOutput);
		
		*out++ = lastOutput;
	}
#ifdef MSP
	return w + 9;
#endif
#ifdef PD
	return w + 4;
#endif
}	

