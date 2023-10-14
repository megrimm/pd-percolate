/********************************************/
/*  A scrubbing delay-line instrument      	*/
/*											*/
/*	yet another PeRColate hack				*/
/*											*/
/*	ported to PD by Olaf Matthes, 2002		*/
/*  										*/
/*  by dan trueman ('98 and on....	)		*/
/*											*/
/********************************************/

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
#define BUFLENGTH 132300. //3 secs
//#define BUFLENGTH 396900. //9 secs
#define MAXBUFLENGTH 3.*132300. 
//#define BUFLENGTH 22050
//#define BUFLENGTH 10000
#define BUFSIZE BUFLENGTH


/* ---------------------------- MSP --------------------------------- */
#ifdef MSP
void *scrub_class;

typedef struct _scrub
{
	//header
    t_pxobject x_obj;

    //DC blocker
    float *buf0, *buf1, *buf2;
    float where, rate;
    float where0;
    float where1;
    float where2;
    long recordPoint;
    float coeff[3];
    int whichBuf; //which buf is currently recording
    float overlap; //number of samps to overlap buffers
    float overlapUpdate;
    float recordenv; //ramp size for record buffers
    float attkTail;
    float overlapInv; 
    float recordenvInv;   
    float delaylen; 
    float delaylen0;
    float delaylen1;
    float delaylen2;
    float delaylenUpdate;
    float recordlen;
    short updatevals;
    //float buflen;
    
    int rate_connected;
    int power;
    
    float srate, one_over_srate;
    float srate_ms, one_over_srate_ms;
} t_scrub;

/****PROTOTYPES****/

//setup funcs
void *scrub_new(double val);
void scrub_dsp(t_scrub *x, t_signal **sp, short *count); 
t_int *scrub_perform(t_int *w);
void scrub_assist(t_scrub *x, void *b, long m, long a, char *s);
float getSamp0(t_scrub *x);
float getSamp1(t_scrub *x);
float getSamp2(t_scrub *x);
void scrub_float(t_scrub *x, double f);

void scrub_alloc(t_scrub *x);
void scrub_free(t_scrub *x);
float recordEnvelope(t_scrub *x, float sample);
float attackEnvelope(t_scrub *x, float sample);
float decayEnvelope(t_scrub *x, float sample);
void setoverlap(t_scrub *x, Symbol *s, short argc, Atom *argv);
void setramp(t_scrub *x, Symbol *s, short argc, Atom *argv);
void setdelay(t_scrub *x, Symbol *s, short argc, Atom *argv);
void setpower(t_scrub *x, Symbol *s, short argc, Atom *argv);
void zero(t_scrub *x, Symbol *s, short argc, Atom *argv);

/****FUNCTIONS****/

void setoverlap(t_scrub *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	float temp;
	x->updatevals = 1;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = (float)argv[i].a_w.w_long;
				if(temp < 10.) temp = 10.;
				x->overlapUpdate = x->srate_ms*temp;
				break;
			case A_FLOAT:
				temp = argv[i].a_w.w_float;
				if(temp < 10.) temp = 10.;
				x->overlapUpdate = x->srate_ms*temp;
				break;
		}
	}
}

void setpower(t_scrub *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	int temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = (int)argv[i].a_w.w_long;
				x->power = temp;
    			post("scrub: setting power: %d", temp);
				break;
			case A_FLOAT:
				temp = (int)argv[i].a_w.w_long;
				x->power = temp;
    			post("scrub: setting power: %d", temp);
				break;
		}
	}
}

void zero(t_scrub *x, Symbol *s, short argc, Atom *argv)
{
	long i;
	
	post ("scrub: zeroing delay lines");
	for(i=0; i<BUFSIZE; i++) {
    	x->buf0[i] = 0.;
    	x->buf1[i] = 0.;
    	x->buf2[i] = 0.;
    }

}

void setramp(t_scrub *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	float temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = (float)argv[i].a_w.w_long;
				if(temp < 10.) temp = 10.;
				x->recordenv = x->srate_ms*temp;
				if(x->recordenv > 0.5*x->delaylen) x->recordenv = 0.5*x->delaylen;
				x->recordenvInv = 1./x->recordenv;
				break;
			case A_FLOAT:
				temp = argv[i].a_w.w_float;
				if(temp < 10.) temp = 10.;
				x->recordenv = x->srate_ms*temp;
				if(x->recordenv > 0.5*x->delaylen) x->recordenv = 0.5*x->delaylen;
				x->recordenvInv = 1./x->recordenv;
				break;
		}
	}
}

//delay length in ms (sorta)
void setdelay(t_scrub *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	float temp;
	x->updatevals = 1;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = (float)argv[i].a_w.w_long;
				if(temp < 10.) temp = 10.;
				x->delaylenUpdate = x->srate_ms*temp;
				//if(x->delaylen  > x->buflen) x->delaylen = x->buflen;
				if(x->delaylenUpdate > BUFSIZE) x->delaylenUpdate = BUFSIZE;
				break;
			case A_FLOAT:
				temp = argv[i].a_w.w_float;
				if(temp < 10.) temp = 10.;
				x->delaylenUpdate = x->srate_ms*temp;
				if(x->delaylenUpdate > BUFSIZE) x->delaylenUpdate = BUFSIZE;
				break;
		}
	}
}

float getSamp0(t_scrub *x)
{
	float alpha, om_alpha, output;
	long first;
	
	while(x->where0 < 0.) x->where0 += x->delaylen0;
	while(x->where0 >= x->delaylen0) x->where0 -= x->delaylen0;
	
	first = (long)x->where0;
	
	alpha = x->where0 - first;
	om_alpha = 1. - alpha;
	
	output = x->buf0[first++] * om_alpha;
	if(first <  x->delaylen0) {
		output += x->buf0[first] * alpha;
	}
	else {
		output += x->buf0[0] * alpha;
	}
	
	return output;
	
}

float getSamp2(t_scrub *x)
{
	float alpha, om_alpha, output;
	long first;

	while(x->where2 < 0.) x->where2 += x->delaylen2;
	while(x->where2 >= x->delaylen2) x->where2 -= x->delaylen2;
	
	first = (long)x->where2;
	
	alpha = x->where2 - first;
	om_alpha = 1. - alpha;
	
	output = x->buf2[first++] * om_alpha;
	if(first <  x->delaylen2) {
		output += x->buf2[first] * alpha;
	}
	else {
		output += x->buf2[0] * alpha;
	}
	
	return output;
	
}

float getSamp1(t_scrub *x)
{
	float alpha, om_alpha, output;
	long first;
	
	while(x->where1 < 0.) x->where1 += x->delaylen1;
	while(x->where1 >= x->delaylen1) x->where1 -= x->delaylen1;
	
	first = (long)x->where1;
	
	alpha = x->where1 - first;
	om_alpha = 1. - alpha;
	
	output = x->buf1[first++] * om_alpha;
	if(first <  x->delaylen1) {
		output += x->buf1[first] * alpha;
	}
	else {
		output += x->buf1[0] * alpha;
	}
	
	return output;
	
}

void scrub_assist(t_scrub *x, void *b, long m, long a, char *s)
{
	assist_string(3215,m,a,1,3,s);
}


//primary MSP funcs
void main(void)
{
    setup((struct messlist **)&scrub_class, (method)scrub_new, (method)scrub_free, (short)sizeof(t_scrub), 0L, A_DEFFLOAT, 0);
    addmess((method)scrub_dsp, "dsp", A_CANT, 0);
    addmess((method)scrub_assist,"assist",A_CANT,0);
    addmess((method)setoverlap, "overlap", A_GIMME, 0);
    addmess((method)setramp, "ramp", A_GIMME, 0);
    addmess((method)setdelay, "delay", A_GIMME, 0);
    addmess((method)setpower, "power", A_GIMME, 0);
    addmess((method)zero, "zero", A_GIMME, 0);
    addfloat((method)scrub_float);
    dsp_initclass();
    rescopy('STR#',3215);
}

void scrub_alloc(t_scrub *x){
	x->buf0 = t_getbytes(BUFLENGTH * sizeof(float));
	//x->buf0 = t_getbytes(x->buflen * sizeof(float));
	if (!x->buf0) {
		error("scrub: out of memory");
		return;
	}
	x->buf1 = t_getbytes(BUFLENGTH * sizeof(float));
	//x->buf1 = t_getbytes(x->buflen * sizeof(float));
	if (!x->buf1) {
		error("scrub: out of memory");
		return;
	}
	x->buf2 = t_getbytes(BUFLENGTH * sizeof(float));
	//x->buf2 = t_getbytes(x->buflen * sizeof(float));
	if (!x->buf2) {
		error("scrub: out of memory");
		return;
	}
}

void scrub_free(t_scrub *x)
{
	if (x->buf1)
		t_freebytes(x->buf1, BUFLENGTH * sizeof(float));
		//t_freebytes(x->buf1, x->buflen * sizeof(float));
	if (x->buf2)
		t_freebytes(x->buf2, BUFLENGTH * sizeof(float));
		//t_freebytes(x->buf2, x->buflen * sizeof(float));
	if (x->buf0)
		t_freebytes(x->buf0, BUFLENGTH * sizeof(float));
		//t_freebytes(x->buf0, x->buflen * sizeof(float));
	dsp_free((t_pxobject *)x);
}


void *scrub_new(double initial_coeff)
{
	int i;
	
	float rate;

    t_scrub *x = (t_scrub *)newobject(scrub_class);
       //zero out the struct, to be careful (takk to jkclayton)
    if (x) { 
        for(i=sizeof(t_pxobject);i<sizeof(t_scrub);i++)  
                ((char *)x)[i]=0; 
	} 
    dsp_setup((t_pxobject *)x,2);
    outlet_new((t_object *)x, "signal");
    
    x->srate = sys_getsr();
    x->one_over_srate = 1./x->srate;
    x->srate_ms = .001 * x->srate;
    x->one_over_srate_ms = 1. / x->srate_ms;
    
    scrub_alloc(x);

    x->rate = 1.;
    x->where = 0.;
    x->where0 = 0.;
    x->where1 = 0.;
    x->where2 = 0.;
    x->recordPoint = 0;
    x->whichBuf = 0;
    x->overlap = 4410.;
    x->overlapUpdate = 4410.;
    x->recordenv = 4410.;
    x->recordlen = BUFSIZE;
    x->delaylen = BUFSIZE;
    x->delaylen0 = BUFSIZE;
    x->delaylen1 = BUFSIZE;
    x->delaylen2 = BUFSIZE;
    x->delaylenUpdate = BUFSIZE;
    x->recordPoint = 0;

    x->attkTail = x->delaylen - x->overlap;    
    x->overlapInv = 1./x->overlap; 
    x->recordenvInv = 1./x->recordenv;
    x->rate_connected = 0;
    
    x->power = 1;
    x->updatevals = -1;
    
    x->coeff[0] = x->coeff[1] = x->coeff[2] = 0.;
    
    for(i=0; i<BUFSIZE; i++) {
    //for(i=0; i<x->buflen; i++) {
    	x->buf0[i] = 0.;
    	x->buf1[i] = 0.;
    	x->buf2[i] = 0.;
    }
    
    return (x);
}


void scrub_dsp(t_scrub *x, t_signal **sp, short *count)
{

	x->rate_connected 	= count[1];
	x->srate = sp[0]->s_sr;
    x->one_over_srate = 1./x->srate;
    x->srate_ms = .001 * x->srate;
    x->one_over_srate_ms = 1. / x->srate_ms;
	dsp_add(scrub_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);	
	
}

void scrub_float(t_scrub *x, double f)
{
	if (x->x_obj.z_in == 1) {
		x->rate = f;
		//post("scrub: setting rate =  %f", f);
	} 
}

float recordEnvelope(t_scrub *x, float sample)
{
	long done = x->recordPoint;
	//long tail = x->delaylen - x->recordPoint;
	long tail = x->recordlen - x->recordPoint;
	
	if(done < x->recordenv) sample *= done*x->recordenvInv;
	else if(tail < x->recordenv) sample *= tail*x->recordenvInv;
	
	return sample;
}

/*
float attackEnvelope(t_scrub *x, float sample)
//float decayEnvelope(t_scrub *x, float sample)
{
	//long tail = x->attkTail; 
	long tail = x->recordlen - x->overlap; 
	float coeff = (x->recordPoint - (float)tail)*x->overlapInv;

	if(x->recordPoint > tail) sample *= coeff;
	else sample = 0.;
	
	return sample;
}
*/
float attackEnvelope(t_scrub *x, float sample)
{
	if(x->recordPoint < x->overlap) sample *= x->recordPoint*x->overlapInv;
	else sample = 1.;
	
	return sample;
}

float decayEnvelope(t_scrub *x, float sample)
{
	long tail = x->recordlen - x->recordPoint;
	
	if(tail < x->overlap) sample *= (float)tail*x->overlapInv;
	else sample = 1.;
	
	return sample;
}


t_int *scrub_perform(t_int *w)
{
	t_scrub *x = (t_scrub *)(w[1]);
        
	float *in = (float *)(w[2]); 
	float rate = x->rate_connected? *(float *)(w[3]) : x->rate;
	float output, input;
	float *out = (float *)(w[4]);
	long n = w[5];
	int moveon;
	
	if(x->power) {
		while(n--) {
		
			input = *in++;
			
			//x->where += rate;
			x->where0 += rate;
			x->where1 += rate;
			x->where2 += rate;

			if(x->recordPoint == 0) {
				x->whichBuf += 1;
				if(x->whichBuf >= 3) x->whichBuf = 0;
				
				x->recordlen = x->delaylenUpdate;
				if(x->recordenv > 0.5*x->recordlen) x->recordenv = 0.5*x->recordlen;
				x->recordenvInv = 1./x->recordenv;
				
				x->overlap = x->overlapUpdate;
				if(x->overlap > 0.5*x->recordlen) x->overlap = 0.5*x->recordlen;    
	    		x->overlapInv = 1./x->overlap; 
				
				//if(x->updatevals-- == 0) {	
					x->delaylen = x->delaylenUpdate;
					/*
					x->overlap = x->overlapUpdate;
					if(x->overlap > 0.5*x->delaylen) x->overlap = 0.5*x->delaylen;
	    			x->overlapInv = 1./x->overlap; 
	    			*/
				//}		
			}
			
			//calcCoeffs(x), put sample;		
			if(x->whichBuf == 0) {
				x->coeff[0] = 0.;
				x->coeff[1] = decayEnvelope(x, 1.); 
				x->coeff[2] = attackEnvelope(x, 1.);
				
				x->buf0[x->recordPoint] = recordEnvelope(x, input);
				if (++x->recordPoint >= x->recordlen) x->recordPoint = 0;
				
				x->delaylen0 = x->delaylen;
				x->where0 = x->where2;
				
				output = x->coeff[1]*getSamp1(x) + x->coeff[2]*getSamp2(x);
			}
			else if(x->whichBuf == 1) {
				x->coeff[2] = decayEnvelope(x, 1.); 
				x->coeff[1] = 0.;
				x->coeff[0] = attackEnvelope(x, 1.);
				
				x->buf1[x->recordPoint] = recordEnvelope(x, input);
				if (++x->recordPoint >= x->recordlen) x->recordPoint = 0;
				
				x->delaylen1 = x->delaylen;
				x->where1 = x->where0;
				
				output = x->coeff[0]*getSamp0(x) + x->coeff[2]*getSamp2(x);
			}
			else if(x->whichBuf == 2) {
				x->coeff[0] = decayEnvelope(x, 1.); 
				x->coeff[2] = 0.;
				x->coeff[1] = attackEnvelope(x, 1.);

				x->buf2[x->recordPoint] = recordEnvelope(x, input);
				if (++x->recordPoint >= x->recordlen) x->recordPoint = 0;
				
				x->delaylen2 = x->delaylen;
				x->where2 = x->where1;
				
				output = x->coeff[0]*getSamp0(x) + x->coeff[1]*getSamp1(x);
			}
			else output = 0.;
		    
		    *out++ = output;
		}
	}
	else while(n--) *out++ = 0.;
	return w + 6;
}	
#endif /* MSP */


/* ----------------------------------- PureData ------------------------------- */
#ifdef PD
static t_class *scrub_class;

typedef struct _scrub
{
	//header
    t_object x_obj;

    //DC blocker
    t_float *buf0, *buf1, *buf2;
    t_float where, rate;
    t_float where0;
    t_float where1;
    t_float where2;
    long recordPoint;
    t_float coeff[3];
    int whichBuf; //which buf is currently recording
    t_float overlap; //number of samps to overlap buffers
    t_float overlapUpdate;
    t_float recordenv; //ramp size for record buffers
    t_float attkTail;
    t_float overlapInv; 
    t_float recordenvInv;   
    t_float delaylen; 
    t_float delaylen0;
    t_float delaylen1;
    t_float delaylen2;
    t_float delaylenUpdate;
    t_float recordlen;
    short updatevals;
    //float buflen;
    
    int rate_connected;
    int power;
    
    t_float srate, one_over_srate;
    t_float srate_ms, one_over_srate_ms;
} t_scrub;

/****FUNCTIONS****/

void setoverlap(t_scrub *x, t_floatarg temp)
{
	x->updatevals = 1;
	if(temp < 10.) temp = 10.;
	x->overlapUpdate = x->srate_ms*temp;
}

static void setpower(t_scrub *x, t_floatarg temp)
{
	x->power = temp;
    post("scrub: setting power: %d", x->power);
}

static void zero(t_scrub *x)
{
	long i;
	
	post ("scrub: zeroing delay lines");
	for(i=0; i<BUFSIZE; i++) {
    	x->buf0[i] = 0.;
    	x->buf1[i] = 0.;
    	x->buf2[i] = 0.;
    }

}

static void setramp(t_scrub *x, t_floatarg temp)
{
	if(temp < 10.) temp = 10.;
	x->recordenv = x->srate_ms*temp;
	if(x->recordenv > 0.5*x->delaylen) x->recordenv = 0.5*x->delaylen;
	x->recordenvInv = 1./x->recordenv;
}

//delay length in ms (sorta)
static void setdelay(t_scrub *x, t_floatarg temp)
{
	x->updatevals = 1;
	if(temp < 10.) temp = 10.;
	x->delaylenUpdate = x->srate_ms*temp;
	//if(x->delaylen  > x->buflen) x->delaylen = x->buflen;
	if(x->delaylenUpdate > BUFSIZE) x->delaylenUpdate = BUFSIZE;
}

static t_float getSamp0(t_scrub *x)
{
	t_float alpha, om_alpha, output;
	long first;
	
	while(x->where0 < 0.) x->where0 += x->delaylen0;
	while(x->where0 >= x->delaylen0) x->where0 -= x->delaylen0;
	
	first = (long)x->where0;
	
	alpha = x->where0 - first;
	om_alpha = 1. - alpha;
	
	output = x->buf0[first++] * om_alpha;
	if(first <  x->delaylen0) {
		output += x->buf0[first] * alpha;
	}
	else {
		output += x->buf0[0] * alpha;
	}
	
	return output;
	
}

static t_float getSamp2(t_scrub *x)
{
	t_float alpha, om_alpha, output;
	long first;

	while(x->where2 < 0.) x->where2 += x->delaylen2;
	while(x->where2 >= x->delaylen2) x->where2 -= x->delaylen2;
	
	first = (long)x->where2;
	
	alpha = x->where2 - first;
	om_alpha = 1. - alpha;
	
	output = x->buf2[first++] * om_alpha;
	if(first <  x->delaylen2) {
		output += x->buf2[first] * alpha;
	}
	else {
		output += x->buf2[0] * alpha;
	}
	
	return output;
	
}

static t_float getSamp1(t_scrub *x)
{
	t_float alpha, om_alpha, output;
	long first;
	
	while(x->where1 < 0.) x->where1 += x->delaylen1;
	while(x->where1 >= x->delaylen1) x->where1 -= x->delaylen1;
	
	first = (long)x->where1;
	
	alpha = x->where1 - first;
	om_alpha = 1. - alpha;
	
	output = x->buf1[first++] * om_alpha;
	if(first <  x->delaylen1) {
		output += x->buf1[first] * alpha;
	}
	else {
		output += x->buf1[0] * alpha;
	}
	
	return output;
	
}

static t_float recordEnvelope(t_scrub *x, t_float sample)
{
	long done = x->recordPoint;
	//long tail = x->delaylen - x->recordPoint;
	long tail = x->recordlen - x->recordPoint;
	
	if(done < x->recordenv) sample *= done*x->recordenvInv;
	else if(tail < x->recordenv) sample *= tail*x->recordenvInv;
	
	return sample;
}

/*
float attackEnvelope(t_scrub *x, float sample)
//float decayEnvelope(t_scrub *x, float sample)
{
	//long tail = x->attkTail; 
	long tail = x->recordlen - x->overlap; 
	float coeff = (x->recordPoint - (float)tail)*x->overlapInv;

	if(x->recordPoint > tail) sample *= coeff;
	else sample = 0.;
	
	return sample;
}
*/
static t_float attackEnvelope(t_scrub *x, t_float sample)
{
	if(x->recordPoint < x->overlap) sample *= x->recordPoint*x->overlapInv;
	else sample = 1.;
	
	return sample;
}

static t_float decayEnvelope(t_scrub *x, t_float sample)
{
	long tail = x->recordlen - x->recordPoint;
	
	if(tail < x->overlap) sample *= (t_float)tail*x->overlapInv;
	else sample = 1.;
	
	return sample;
}

//primary MSP funcs

static t_int *scrub_perform(t_int *w)
{
	t_scrub *x = (t_scrub *)(w[1]);
        
	t_float *in = (t_float *)(w[2]); 
	t_float rate = x->rate;
	t_float output, input;
	t_float *out = (t_float *)(w[3]);
	long n = w[4];
	int moveon;
	
	if(x->power) {
		while(n--) {
		
			input = *in++;
			
			//x->where += rate;
			x->where0 += rate;
			x->where1 += rate;
			x->where2 += rate;

			if(x->recordPoint == 0) {
				x->whichBuf += 1;
				if(x->whichBuf >= 3) x->whichBuf = 0;
				
				x->recordlen = x->delaylenUpdate;
				if(x->recordenv > 0.5*x->recordlen) x->recordenv = 0.5*x->recordlen;
				x->recordenvInv = 1./x->recordenv;
				
				x->overlap = x->overlapUpdate;
				if(x->overlap > 0.5*x->recordlen) x->overlap = 0.5*x->recordlen;    
	    		x->overlapInv = 1./x->overlap; 
				
				//if(x->updatevals-- == 0) {	
					x->delaylen = x->delaylenUpdate;
					/*
					x->overlap = x->overlapUpdate;
					if(x->overlap > 0.5*x->delaylen) x->overlap = 0.5*x->delaylen;
	    			x->overlapInv = 1./x->overlap; 
	    			*/
				//}		
			}
			
			//calcCoeffs(x), put sample;		
			if(x->whichBuf == 0) {
				x->coeff[0] = 0.;
				x->coeff[1] = decayEnvelope(x, 1.); 
				x->coeff[2] = attackEnvelope(x, 1.);
				
				x->buf0[x->recordPoint] = recordEnvelope(x, input);
				if (++x->recordPoint >= x->recordlen) x->recordPoint = 0;
				
				x->delaylen0 = x->delaylen;
				x->where0 = x->where2;
				
				output = x->coeff[1]*getSamp1(x) + x->coeff[2]*getSamp2(x);
			}
			else if(x->whichBuf == 1) {
				x->coeff[2] = decayEnvelope(x, 1.); 
				x->coeff[1] = 0.;
				x->coeff[0] = attackEnvelope(x, 1.);
				
				x->buf1[x->recordPoint] = recordEnvelope(x, input);
				if (++x->recordPoint >= x->recordlen) x->recordPoint = 0;
				
				x->delaylen1 = x->delaylen;
				x->where1 = x->where0;
				
				output = x->coeff[0]*getSamp0(x) + x->coeff[2]*getSamp2(x);
			}
			else if(x->whichBuf == 2) {
				x->coeff[0] = decayEnvelope(x, 1.); 
				x->coeff[2] = 0.;
				x->coeff[1] = attackEnvelope(x, 1.);

				x->buf2[x->recordPoint] = recordEnvelope(x, input);
				if (++x->recordPoint >= x->recordlen) x->recordPoint = 0;
				
				x->delaylen2 = x->delaylen;
				x->where2 = x->where1;
				
				output = x->coeff[0]*getSamp0(x) + x->coeff[1]*getSamp1(x);
			}
			else output = 0.;
		    
		    *out++ = output;
		}
	}
	else while(n--) *out++ = 0.;
	return w + 5;
}

static void scrub_dsp(t_scrub *x, t_signal **sp)
{

	x->srate = sp[0]->s_sr;
    x->one_over_srate = 1./x->srate;
    x->srate_ms = .001 * x->srate;
    x->one_over_srate_ms = 1. / x->srate_ms;
	dsp_add(scrub_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);	
	
}


void scrub_alloc(t_scrub *x){
	x->buf0 = t_getbytes(BUFLENGTH * sizeof(t_float));
	//x->buf0 = t_getbytes(x->buflen * sizeof(float));
	if (!x->buf0) {
		perror("scrub: out of memory");
		return;
	}
	x->buf1 = t_getbytes(BUFLENGTH * sizeof(t_float));
	//x->buf1 = t_getbytes(x->buflen * sizeof(float));
	if (!x->buf1) {
		perror("scrub: out of memory");
		return;
	}
	x->buf2 = t_getbytes(BUFLENGTH * sizeof(t_float));
	//x->buf2 = t_getbytes(x->buflen * sizeof(float));
	if (!x->buf2) {
		perror("scrub: out of memory");
		return;
	}
}


static void scrub_float(t_scrub *x, t_floatarg f)
{
	x->rate = f;
	// post("scrub: setting rate =  %f", f);
}

static void scrub_free(t_scrub *x)
{
	if (x->buf1)
		t_freebytes(x->buf1, BUFLENGTH * sizeof(t_float));
		//t_freebytes(x->buf1, x->buflen * sizeof(float));
	if (x->buf2)
		t_freebytes(x->buf2, BUFLENGTH * sizeof(t_float));
		//t_freebytes(x->buf2, x->buflen * sizeof(float));
	if (x->buf0)
		t_freebytes(x->buf0, BUFLENGTH * sizeof(t_float));
		//t_freebytes(x->buf0, x->buflen * sizeof(float));
}


static void *scrub_new(void)
{
	unsigned int i;
	
	float rate;

    t_scrub *x = (t_scrub *)pd_new(scrub_class);
       //zero out the struct, to be careful (takk to jkclayton)
    if (x) { 
        for(i=sizeof(t_object);i<sizeof(t_scrub);i++)  
                ((char *)x)[i]=0; 
	} 
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("scrub"));
	outlet_new(&x->x_obj, gensym("signal"));
    			   
    x->srate = sys_getsr();
    x->one_over_srate = 1./x->srate;
    x->srate_ms = .001 * x->srate;
    x->one_over_srate_ms = 1. / x->srate_ms;
    
    scrub_alloc(x);

    x->rate = 1.;
    x->where = 0.;
    x->where0 = 0.;
    x->where1 = 0.;
    x->where2 = 0.;
    x->recordPoint = 0;
    x->whichBuf = 0;
    x->overlap = 4410.;
    x->overlapUpdate = 4410.;
    x->recordenv = 4410.;
    x->recordlen = BUFSIZE;
    x->delaylen = BUFSIZE;
    x->delaylen0 = BUFSIZE;
    x->delaylen1 = BUFSIZE;
    x->delaylen2 = BUFSIZE;
    x->delaylenUpdate = BUFSIZE;
    x->recordPoint = 0;

    x->attkTail = x->delaylen - x->overlap;    
    x->overlapInv = 1./x->overlap; 
    x->recordenvInv = 1./x->recordenv;
    x->rate_connected = 0;
    
    x->power = 1;
    x->updatevals = -1;
    
    x->coeff[0] = x->coeff[1] = x->coeff[2] = 0.;
    
    for(i=0; i<BUFSIZE; i++) {
    //for(i=0; i<x->buflen; i++) {
    	x->buf0[i] = 0.;
    	x->buf1[i] = 0.;
    	x->buf2[i] = 0.;
    }
    
    return (x);
}

	
void scrub_tilde_setup(void)
{
	scrub_class = class_new(gensym("scrub~"), (t_newmethod)scrub_new, (t_method)scrub_free,
        sizeof(t_scrub), 0, 0);
    class_addmethod(scrub_class, nullfn, gensym("signal"), A_NULL);
    class_addmethod(scrub_class, (t_method)scrub_dsp, gensym("dsp"), A_NULL);
	class_addfloat(scrub_class, (t_method)scrub_float);
	class_addmethod(scrub_class, (t_method)scrub_float, gensym("scrub"), A_FLOAT, A_NULL);
    class_addmethod(scrub_class, (t_method)setoverlap, gensym("overlap"), A_FLOAT, A_NULL);
    class_addmethod(scrub_class, (t_method)setramp, gensym("ramp"), A_FLOAT, A_NULL);
    class_addmethod(scrub_class, (t_method)setdelay, gensym("delay"), A_FLOAT, A_NULL);
    class_addmethod(scrub_class, (t_method)setpower, gensym("power"), A_FLOAT, A_NULL);
    class_addmethod(scrub_class, (t_method)zero, gensym("zero"), A_NULL);
    class_sethelpsymbol(scrub_class, gensym("help-scrub~.pd"));
}
#endif /* PD */
