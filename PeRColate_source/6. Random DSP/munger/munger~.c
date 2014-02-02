/********************************************/
/*  A granular sampling instrument      	*/
/*											*/
/*	yet another PeRColate hack				*/
/*  										*/
/*  by dan trueman ('97 and on....	)		*/
/*  										*/
/*  ported to PD by Olaf Matthes, 2002		*/
/*											*/
/* 	ok, this is some butt-ugly code, what	*/
/*  can i say. always a work in progress	*/
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
#define ONE_OVER_HALFRAND 0.00006103516 	// constant = 1. / 16384.0
#define ONE_OVER_MAXRAND 0.000030517578 	// 1 / 32768
#define NUMVOICES 100						//max number of voices
#define MINSPEED .001 						//minimum speed through buffer on playback
#define ENVSIZE 32
#define ONE_OVER_ENVSIZE .0078125
#define MINSIZE 64							// twice ENVSIZE. minimum grainsize in samples
#ifdef MSP						
#define RAND01 (float)rand() * ONE_OVER_MAXRAND //random numbers 0-1
#define RAND11 ((float)rand() - 16384.) * ONE_OVER_HALFRAND //random numbers -1 to 1
#endif /* MSP */
#ifdef PD
#define RAND01 (t_float)rand() * ONE_OVER_MAXRAND //random numbers 0-1
#define RAND11 ((t_float)rand() - 16384.) * ONE_OVER_HALFRAND //random numbers -1 to 1
#endif /* PD */
#define WINLENGTH 1024
#define PITCHTABLESIZE 100 					//max number of transpositions for the "scale" message


#ifdef MSP
/* -------------------------------- MSP ------------------------------------- */
/****MAIN STRUCT****/

void *munger_class;

typedef struct _munger
{
	//header
    t_pxobject x_obj;
    
    //user controlled vars
    float grate;			//grain rate	
    float grate_var;		//grain rate variation; percentage of grain rate
    float glen; 			//grain length
    float glen_var; 		
    float gpitch; 		 
    float gpitch_var;
    float gpan_spread;		//how much to spread the grains around center
				
	float pitchTable[PITCHTABLESIZE]; 	//table of pitch values to draw from
	float twelfth; //1/12
	float semitone;
	short smoothPitch;
	
	float gain, randgain;
	float position; //playback position (0-1) (if ==-1, then random, which is default)
	
	float buflen;
	float maxsize, minsize;
	float twothirdBufsize, onethirdBufsize;
	float initbuflen;
	long maxvoices;

    //signals connected? or controls...
    short grate_connected;
    short grate_var_connected;
    short glen_connected;
    short glen_var_connected;
    short gpitch_connected;
    short gpitch_var_connected;
    short gpan_spread_connected;
    
    //window stuff
    short doHanning;
    float winTime[NUMVOICES], winRate[NUMVOICES];
    float winTable[WINLENGTH];
    float rampLength; //for simple linear ramp
    
    //voice parameters
    long gvoiceSize[NUMVOICES];			//sample size
    double gvoiceSpeed[NUMVOICES];		//1 = at pitch
    double gvoiceCurrent[NUMVOICES];	//current sample position
    int gvoiceDirection[NUMVOICES];		//1 = forward, -1 backwards
    int gvoiceOn[NUMVOICES];			//currently playing? boolean
    long gvoiceDone[NUMVOICES];			//how many samples already played from grain
    float gvoiceLPan[NUMVOICES];
    float gvoiceRPan[NUMVOICES];
    float gvoiceRamp[NUMVOICES];
    float gvoiceOneOverRamp[NUMVOICES];
    float gvoiceGain[NUMVOICES];
    int voices;
    float gimme;
    
    //sample buffer
    float *recordBuf;
    int recordOn; //boolean
    long recordCurrent;
    
    //other stuff
    long time; 
    int power;
    short ambi;

    float srate, one_over_srate;
    float srate_ms, one_over_srate_ms;
} t_munger;



/****PROTOTYPES****/

//setup funcs
//void *munger_new(double val);
void *munger_new(double maxdelay);
void munger_dsp(t_munger *x, t_signal **sp, short *count);
void munger_float(t_munger *x, double f);
t_int *munger_perform(t_int *w);
void munger_alloc(t_munger *x);
void munger_free(t_munger *x);
void munger_assist(t_munger *x, void *b, long m, long a, char *s);
void setpower(t_munger *x, Symbol *s, short argc, Atom *argv);

//sample buffer funcs
void recordSamp(t_munger *x, float sample);		//stick a sample in the buffer
float getSamp(t_munger *x, double where); 		//get sample, using linear interpolation

//grain funcs
int findVoice(t_munger *x);						//tries to find an available voice
float newSetup(t_munger *x, int whichVoice);	//creates a new start position for a new grain
float newSize(t_munger *x, int whichVoice);		//creates a size for a new grain
int newDirection(t_munger *x);
float envelope(t_munger *x, int whichone, float sample);

//window funcs
float win_tick(t_munger *x, int whichone);
void munger_setramp(t_munger *x, Symbol *s, short argc, Atom *argv);
void munger_sethanning(t_munger *x, Symbol *s, short argc, Atom *argv);

//scale funcs
void munger_scale(t_munger *x, Symbol *s, short argc, Atom *argv);
void munger_tempered(t_munger *x, Symbol *s, short argc, Atom *argv);
void munger_smooth(t_munger *x, Symbol *s, short argc, Atom *argv);

//buffersize change
void munger_bufsize(t_munger *x, Symbol *s, short argc, Atom *argv);

//buffersize change (ms)
void munger_bufsize_ms(t_munger *x, Symbol *s, short argc, Atom *argv);

//set maximum number of voices possible
void munger_maxvoices(t_munger *x, Symbol *s, short argc, Atom *argv);

//set number of voices to actually use
void munger_setvoices(t_munger *x, Symbol *s, short argc, Atom *argv);

//set min grain size
void munger_setminsize(t_munger *x, Symbol *s, short argc, Atom *argv);

//turn on/off backwards grains 
void munger_ambidirectional(t_munger *x, Symbol *s, short argc, Atom *argv);

//turn on/off recording
void munger_record(t_munger *x, Symbol *s, short argc, Atom *argv);

//set overall gain and rand gain range
void munger_gain(t_munger *x, Symbol *s, short argc, Atom *argv);
void munger_randgain(t_munger *x, Symbol *s, short argc, Atom *argv);

//fix position for start of grain playback
void munger_setposition(t_munger *x, Symbol *s, short argc, Atom *argv);





/****FUNCTIONS****/

//window stuff
float win_tick(t_munger *x, int whichone)
{
	long temp;
	float temp_time, alpha, output;
	
	x->winTime[whichone] += x->winRate[whichone];
	while (x->winTime[whichone] >= (float)WINLENGTH) x->winTime[whichone] -= (float)WINLENGTH;
	while (x->winTime[whichone] < 0.) x->winTime[whichone] += (float)WINLENGTH;
	
	temp_time = x->winTime[whichone];
	
	temp = (long)temp_time;
	alpha = temp_time - (float)temp;
	output = x->winTable[temp];
	output = output + (alpha * (x->winTable[temp+1] - output));
	return output;
}

//grain funcs
float envelope(t_munger *x, int whichone, float sample)
{
	long done = x->gvoiceDone[whichone];
	long tail = x->gvoiceSize[whichone] - x->gvoiceDone[whichone];
	
	/*
	if(x->doHanning) sample *= win_tick(x, whichone);
	else{
		if(done < x->gvoiceRamp[whichone]) sample *= (done*x->gvoiceOneOverRamp[whichone]);
		else if(tail < x->gvoiceRamp[whichone]) sample *= (tail*x->gvoiceOneOverRamp[whichone]);
	}
	*/
	if(done < x->gvoiceRamp[whichone]) sample *= (done*x->gvoiceOneOverRamp[whichone]);
	else if(tail < x->gvoiceRamp[whichone]) sample *= (tail*x->gvoiceOneOverRamp[whichone]);
	
	return sample;
}

//tries to find an available voice; return -1 if no voices available
int findVoice(t_munger *x)
{
	int i = 0, foundOne = -1;
	while(foundOne < 0 && i < x->voices ) {
		if (!x->gvoiceOn[i]) foundOne = i;
		i++;
	}
	return foundOne;
}

//creates a new (random) start position for a new grain, returns beginning start sample
//sets up size and direction
//max grain size is BUFLENGTH / 3, to avoid recording into grains while they are playing
float newSetup(t_munger *x, int whichVoice)
{
	float newPosition;
	
	x->gvoiceSize[whichVoice] 		= newSize(x, whichVoice);
	x->gvoiceDirection[whichVoice] 	= newDirection(x);
	x->gvoiceLPan[whichVoice] 		= ((float)rand() - 16384.) * ONE_OVER_MAXRAND * x->gpan_spread + 0.5;
	x->gvoiceRPan[whichVoice]		= 1. - x->gvoiceLPan[whichVoice];
	x->gvoiceOn[whichVoice] 		= 1;
	x->gvoiceDone[whichVoice]		= 0;
	x->gvoiceGain[whichVoice]		= x->gain + ((float)rand() - 16384.) * ONE_OVER_HALFRAND * x->randgain;
	
	if(x->gvoiceSize[whichVoice] < 2.*x->rampLength) {
		x->gvoiceRamp[whichVoice] = .5 * x->gvoiceSize[whichVoice];
		if(x->gvoiceRamp[whichVoice] <= 0.) x->gvoiceRamp[whichVoice] = 1.;
		x->gvoiceOneOverRamp[whichVoice] = 1./x->gvoiceRamp[whichVoice];
	}
	else {
		x->gvoiceRamp[whichVoice] = x->rampLength;
		if(x->gvoiceRamp[whichVoice] <= 0.) x->gvoiceRamp[whichVoice] = 1.;
		x->gvoiceOneOverRamp[whichVoice] = 1./x->gvoiceRamp[whichVoice];
	}
	
	
/*** set start point; tricky, cause of moving buffer, variable playback rates, backwards/forwards, etc.... ***/

	// 1. random positioning and moving buffer (default)
	if(x->position == -1. && x->recordOn == 1) { 
		if(x->gvoiceDirection[whichVoice] == 1) {//going forward			
			if(x->gvoiceSpeed[whichVoice] > 1.) 
				newPosition = x->recordCurrent - x->onethirdBufsize - (float)rand() * ONE_OVER_MAXRAND * x->onethirdBufsize;
			else
				newPosition = x->recordCurrent - (float)rand() * ONE_OVER_MAXRAND * x->onethirdBufsize;//was 2/3rds
		}
		
		else //going backwards
			newPosition = x->recordCurrent - (float)rand() * ONE_OVER_MAXRAND * x->onethirdBufsize;
	}
	
	// 2. fixed positioning and moving buffer	
	else if (x->position >= 0. && x->recordOn == 1) {
		if(x->gvoiceDirection[whichVoice] == 1) {//going forward			
			if(x->gvoiceSpeed[whichVoice] > 1.) 
				//newPosition = x->recordCurrent - x->onethirdBufsize - x->position * x->onethirdBufsize;
				//this will follow more closely...
				newPosition = x->recordCurrent - x->gvoiceSize[whichVoice]*x->gvoiceSpeed[whichVoice] - x->position * x->onethirdBufsize;
				
			else
				newPosition = x->recordCurrent - x->position * x->onethirdBufsize;//was 2/3rds
		}
		
		else //going backwards
			newPosition = x->recordCurrent - x->position * x->onethirdBufsize;
	}
	
	// 3. random positioning and fixed buffer	
	else if (x->position == -1. && x->recordOn == 0) {
		if(x->gvoiceDirection[whichVoice] == 1) {//going forward			
			newPosition = x->recordCurrent - x->onethirdBufsize - (float)rand() * ONE_OVER_MAXRAND * x->onethirdBufsize;
		}	
		else //going backwards
			newPosition = x->recordCurrent - (float)rand() * ONE_OVER_MAXRAND * x->onethirdBufsize;
	}
	
	// 4. fixed positioning and fixed buffer	
	else if (x->position >= 0. && x->recordOn == 0) {
		if(x->gvoiceDirection[whichVoice] == 1) {//going forward			
			newPosition = x->recordCurrent - x->onethirdBufsize - x->position * x->onethirdBufsize;
		}	
		else //going backwards
			newPosition = x->recordCurrent - x->position * x->onethirdBufsize;
	}
	
	return newPosition;
	
}	

//creates a size for a new grain
//actual number of samples PLAYED, regardless of pitch
//might be shorter for higher pitches and long grains, to avoid collisions with recordCurrent
//size given now in milliseconds!
float newSize(t_munger *x, int whichOne)
{
	float newsize, temp;
	int pitchChoice, pitchExponent;
	
	//set grain pitch
	if(x->smoothPitch == 1) x->gvoiceSpeed[whichOne] = x->gpitch + ((float)rand() - 16384.) * ONE_OVER_HALFRAND*x->gpitch_var;
	else {
		temp = (float)rand() * ONE_OVER_MAXRAND * x->gpitch_var * (float)PITCHTABLESIZE * .1;
		pitchChoice = (int) temp;
		if(pitchChoice > PITCHTABLESIZE) pitchChoice = PITCHTABLESIZE;
		if(pitchChoice < 0) pitchChoice = 0;
		pitchExponent = x->pitchTable[pitchChoice];
		x->gvoiceSpeed[whichOne] = x->gpitch * pow(x->semitone, pitchExponent);
	}
	
	if(x->gvoiceSpeed[whichOne] < MINSPEED) x->gvoiceSpeed[whichOne] = MINSPEED;
	newsize = x->srate_ms*(x->glen + ((float)rand() - 16384.) * ONE_OVER_HALFRAND*x->glen_var);
	if(newsize > x->maxsize) newsize = x->maxsize;
	if(newsize*x->gvoiceSpeed[whichOne] > x->maxsize) 
		newsize = x->maxsize/x->gvoiceSpeed[whichOne];
	if(newsize < x->minsize) newsize = x->minsize;
	return newsize;

}	
int newDirection(t_munger *x)
{
//-1 == always backwards
//0  == backwards and forwards (default)
//1  == only forwards
	int dir;
	if(x->ambi == 0) {
		dir = rand() - 16384;
		if (dir < 0) dir = -1;
		else dir = 1;
	} 
	else 
		if(x->ambi == -1) dir = -1;
	else
		dir = 1;

	return dir;
}


//buffer funcs
void recordSamp(t_munger *x, float sample)
{
	if(x->recordCurrent >= x->buflen) x->recordCurrent = 0;
	x->recordBuf[x->recordCurrent++] = sample;
}

float getSamp(t_munger *x, double where)
{
	double alpha, om_alpha, output;
	long first;
	
	while(where < 0.) where += x->buflen;
	while(where >= x->buflen) where -= x->buflen;
	
	first = (long)where;
	
	alpha = where - first;
	om_alpha = 1. - alpha;
	
	output = x->recordBuf[first++] * om_alpha;
	if(first <  x->buflen) {
		output += x->recordBuf[first] * alpha;
	}
	else {
		output += x->recordBuf[0] * alpha;
	}
	
	return (float)output;
	
}

//primary MSP funcs
void main(void)
{
    setup((struct messlist **)&munger_class, (method)munger_new, (method)munger_free, (short)sizeof(t_munger), 0L, A_DEFFLOAT, 0);
    addmess((method)munger_dsp, "dsp", A_CANT, 0);
    addmess((method)munger_assist,"assist",A_CANT,0);
    addmess((method)munger_setramp, "ramptime", A_GIMME, 0);
    addmess((method)munger_sethanning, "hanning", A_GIMME, 0);
    addmess((method)munger_tempered, "tempered", A_GIMME, 0);
    addmess((method)munger_smooth, "smooth", A_GIMME, 0);
    addmess((method)munger_scale, "scale", A_GIMME, 0);
    addmess((method)munger_bufsize, "delaylength", A_GIMME, 0);
    addmess((method)munger_bufsize_ms, "delaylength_ms", A_GIMME, 0);
    addmess((method)munger_setvoices, "voices", A_GIMME, 0);
    addmess((method)munger_maxvoices, "maxvoices", A_GIMME, 0);
    addmess((method)munger_setminsize, "minsize", A_GIMME, 0);
    addmess((method)setpower, "power", A_GIMME, 0);
    addmess((method)munger_ambidirectional, "ambidirectional", A_GIMME, 0);
    addmess((method)munger_record, "record", A_GIMME, 0);
    addmess((method)munger_gain, "gain", A_GIMME, 0);
    addmess((method)munger_randgain, "rand_gain", A_GIMME, 0);
    addmess((method)munger_setposition, "position", A_GIMME, 0);
    addfloat((method)munger_float);
    dsp_initclass();
    rescopy('STR#',9810);
}

void munger_setramp(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	x->doHanning = 0;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				//post("munger: setting ramp to: %ld ms", argv[i].a_w.w_long);
				x->rampLength = x->srate_ms * (float)argv[i].a_w.w_long; 
				if(x->rampLength <= 0.) x->rampLength = 1.;
				break;
			case A_FLOAT:
				//post("munger: setting ramp to: %lf ms", argv[i].a_w.w_float);
				x->rampLength = x->srate_ms * argv[i].a_w.w_float;
				if(x->rampLength <= 0.) x->rampLength = 1.; 
				break;
		}
	}
}

void munger_scale(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	int i,j;
	
	//post("munger: loading scale from input list");
	x->smoothPitch = 0;
	
	for(i=0;i<PITCHTABLESIZE;i++) x->pitchTable[i] = 0.;
	if (argc > PITCHTABLESIZE) argc = PITCHTABLESIZE;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				x->pitchTable[i] = (float)argv[i].a_w.w_long; 
				break;
			case A_FLOAT:
				x->pitchTable[i] = argv[i].a_w.w_float;
				break;
		}
	}
	i=0;
	//wrap input list through all of pitchTable
	for (j=argc; j < PITCHTABLESIZE; j++) {
		x->pitchTable[j] = x->pitchTable[i++];
		if (i >= argc) i = 0;
	}
		
}

void munger_bufsize(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	float temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = x->srate * (float)argv[i].a_w.w_long;
				if(temp < 20.*(float)MINSIZE) temp = 20.*(float)MINSIZE;
				//if(temp > (float)BUFLENGTH) temp = (float)BUFLENGTH;
				if(temp > x->initbuflen) temp = x->initbuflen;
				//x->buflen = temp;
				//x->maxsize = x->buflen / 3.;
				x->maxsize = temp / 3.;
    			x->twothirdBufsize = x->maxsize * 2.;
    			x->onethirdBufsize = x->maxsize; 
    			//post("munger: setting delaylength to: %f seconds", (x->buflen/x->srate));
				break;
			case A_FLOAT:
				temp = x->srate * argv[i].a_w.w_float;
				if(temp < 20.*(float)MINSIZE) temp = 20.*(float)MINSIZE;
				//if(temp > (float)BUFLENGTH) temp = (float)BUFLENGTH;
				if(temp > x->initbuflen) temp = x->initbuflen;
				//x->buflen = temp;
				//x->maxsize = x->buflen / 3.;
				x->maxsize = temp / 3.;
    			x->twothirdBufsize = x->maxsize * 2.;
    			x->onethirdBufsize = x->maxsize; 
    			//post("munger: setting delaylength to: %f seconds", x->buflen/x->srate);
				break;
		}
	}
}

void munger_bufsize_ms(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	float temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = x->srate_ms * (float)argv[i].a_w.w_long;
				if(temp < 20.*(float)MINSIZE) temp = 20.*(float)MINSIZE;
				//if(temp > (float)BUFLENGTH) temp = (float)BUFLENGTH;
				if(temp > x->initbuflen) temp = x->initbuflen;
				//x->buflen = temp;
				//x->maxsize = x->buflen / 3.;
				x->maxsize = temp / 3.;
    			x->twothirdBufsize = x->maxsize * 2.;
    			x->onethirdBufsize = x->maxsize; 
    			//post("munger: setting delaylength to: %f seconds", (x->buflen/x->srate));
				break;
			case A_FLOAT:
				temp = x->srate_ms * argv[i].a_w.w_float;
				if(temp < 20.*(float)MINSIZE) temp = 20.*(float)MINSIZE;
				//if(temp > (float)BUFLENGTH) temp = (float)BUFLENGTH;
				if(temp > x->initbuflen) temp = x->initbuflen;
				//x->buflen = temp;
				//x->maxsize = x->buflen / 3.;
				x->maxsize = temp / 3.;
    			x->twothirdBufsize = x->maxsize * 2.;
    			x->onethirdBufsize = x->maxsize; 
    			//post("munger: setting delaylength to: %f seconds", x->buflen/x->srate);
				break;
		}
	}
}

void munger_setminsize(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	float temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = x->srate_ms * (float)argv[i].a_w.w_long;
				if(temp < (float)MINSIZE) temp = (float)MINSIZE;
				if(temp >= x->initbuflen) {
					temp = (float)MINSIZE;
					post("munger error: minsize too large!");
				}
				x->minsize = temp;
    			//post("munger: setting min grain size to: %f ms", (x->minsize/x->srate_ms));
				break;
			case A_FLOAT:
				temp = x->srate_ms * argv[i].a_w.w_float;
				if(temp < (float)MINSIZE) temp = (float)MINSIZE;
				if(temp >= x->initbuflen) {
					temp = (float)MINSIZE;
					post("munger error: minsize too large!");
				}
				x->minsize = temp;
    			//post("munger: setting min grain size to: %f ms", (x->minsize/x->srate_ms));
				break;
		}
	}
}


void munger_setvoices(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	int temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = argv[i].a_w.w_long;
				if(temp < 0) temp = 0;
				if(temp > x->maxvoices) temp = x->maxvoices;
				x->voices = temp;
    			//post("munger: setting max voices to: %d ", x->voices);
				break;
			case A_FLOAT:
				temp = (int)argv[i].a_w.w_float;
				if(temp < 0) temp = 0;
				if(temp > x->maxvoices) temp = x->maxvoices;
				x->voices = temp;
    			//post("munger: setting max voices to: %d ", x->voices);
				break;
		}
	}
}

void munger_maxvoices(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	int temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = argv[i].a_w.w_long;
				if(temp < 0) temp = 0;
				if(temp > NUMVOICES) temp = NUMVOICES;
				x->maxvoices = temp;
    			//post("munger: setting max voices to: %d ", x->voices);
				break;
			case A_FLOAT:
				temp = (int)argv[i].a_w.w_float;
				if(temp < 0) temp = 0;
				if(temp > NUMVOICES) temp = NUMVOICES;
				x->maxvoices = temp;
    			//post("munger: setting max voices to: %d ", x->voices);
				break;
		}
	}
}


void setpower(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	int temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = (int)argv[i].a_w.w_long;
				x->power = temp;
    			post("munger: setting power: %d", temp);
				break;
			case A_FLOAT:
				temp = (int)argv[i].a_w.w_long;
				x->power = temp;
    			post("munger: setting power: %d", temp);
				break;
		}
	}
}

void munger_record(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	int temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = (int)argv[i].a_w.w_long;
				x->recordOn = temp;
    			post("munger: setting record: %d", temp);
				break;
			case A_FLOAT:
				temp = (int)argv[i].a_w.w_long;
				x->recordOn = temp;
    			post("munger: setting record: %d", temp);
				break;
		}
	}
}

void munger_ambidirectional(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	int temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = (int)argv[i].a_w.w_long;
				x->ambi = temp;
    			post("munger: setting ambidirectional: %d", temp);
				break;
			case A_FLOAT:
				temp = (int)argv[i].a_w.w_float;
				x->ambi = temp;
    			post("munger: setting ambidirectional: %d", temp);
				break;
		}
	}
}

void munger_gain(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	float temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = (float)argv[i].a_w.w_long;
    			post("munger: setting gain to: %f ", temp);
    			x->gain = temp;
				break;
			case A_FLOAT:
				temp = argv[i].a_w.w_float;
				post("munger: setting gain to: %f ", temp);
				x->gain = temp;
				break;
		}
	}
}

void munger_setposition(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	float temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = (float)argv[i].a_w.w_long;
				if (temp > 1.) temp = 1.;
				if (temp == -1.) 
    				post("munger: random positioning");
    			else if (temp < 0.) 
    				temp = 0.;
    			//post("munger: setting position to: %f ", temp);
    			x->position = temp;
				break;
			case A_FLOAT:
				temp = argv[i].a_w.w_float;
				if (temp > 1.) temp = 1.;
				if (temp == -1.) 
    				post("munger: random positioning");
    			else if (temp < 0.) 
    				temp = 0.;
    			//post("munger: setting position to: %f ", temp);
    			x->position = temp;
				break;
		}
	}
}

void munger_randgain(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	float temp;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = (float)argv[i].a_w.w_long;
    			post("munger: setting rand_gain to: %f ", temp);
    			x->randgain = temp;
				break;
			case A_FLOAT:
				temp = argv[i].a_w.w_float;
				post("munger: setting rand_gain to: %f ", temp);
				x->randgain = temp;
				break;
		}
	}
}

void munger_sethanning(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	post("munger: hanning window is busted");
	x->doHanning = 1;
}

void munger_tempered(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	int i;
	//post("munger: doing tempered scale");
	x->smoothPitch = 0;
	for(i=0; i<PITCHTABLESIZE-1; i += 2) {
		x->pitchTable[i] = 0.5*i;
		x->pitchTable[i+1] = -0.5*i;
	}
}
void munger_smooth(t_munger *x, Symbol *s, short argc, Atom *argv)
{
	//post("munger: doing smooth scale");
	x->smoothPitch = 1;
}

void munger_alloc(t_munger *x)
{
	//x->recordBuf = t_getbytes(BUFLENGTH * sizeof(float));
	x->recordBuf = t_getbytes(x->buflen * sizeof(float));
	if (!x->recordBuf) {
		error("munger: out of memory");
		return;
	}
}

void munger_free(t_munger *x)
{
	if (x->recordBuf)
		//t_freebytes(x->recordBuf, BUFLENGTH * sizeof(float));
		t_freebytes(x->recordBuf, x->initbuflen * sizeof(float));
	dsp_free((t_pxobject *)x);
}

void munger_assist(t_munger *x, void *b, long m, long a, char *s)
{
	assist_string(9810,m,a,1,9,s);
}

void munger_float(t_munger *x, double f)
{
	if (x->x_obj.z_in == 1) {
		x->grate = f;
	} else if (x->x_obj.z_in == 2) {
		x->grate_var = f;
	} else if (x->x_obj.z_in == 3) {
		x->glen = f;
	} else if (x->x_obj.z_in == 4) {
		x->glen_var = f;
	} else if (x->x_obj.z_in == 5) {
		x->gpitch = f;
	} else if (x->x_obj.z_in == 6) {
		x->gpitch_var = f;
	} else if (x->x_obj.z_in == 7) {
		x->gpan_spread = f;
	}
}

//hanning y = 0.5 + 0.5*cos(TWOPI * n/N + PI)

void *munger_new(double maxdelay)
{
	int i;
    
    t_munger *x = (t_munger *)newobject(munger_class);
    //zero out the struct, to be careful (takk to jkclayton)
    if (x) { 
        for(i=sizeof(t_pxobject);i<sizeof(t_munger);i++)  
                ((char *)x)[i]=0; 
	} 
	
	if (maxdelay < 100.) maxdelay = 3000.; //set maxdelay to 3000ms by default
	post("munger: maxdelay = %f milliseconds", maxdelay);
	
    dsp_setup((t_pxobject *)x,8);
    outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");
    			   
    x->srate = sys_getsr();
    x->one_over_srate = 1./x->srate;
    x->srate_ms = x->srate/1000.;
    x->one_over_srate_ms = 1./x->srate_ms;
    
    //x->buflen = (float)BUFLENGTH;
    if (x->recordBuf)
		t_freebytes(x->recordBuf, x->initbuflen * sizeof(float));
		
    x->initbuflen = (float)maxdelay * 44.1;
    x->buflen = x->initbuflen;
    x->maxsize = x->buflen / 3.;
    x->twothirdBufsize = x->maxsize * 2.;
    x->onethirdBufsize = x->maxsize;
    x->minsize = MINSIZE;
    x->voices = 10;
    x->gain = 1.;
    x->randgain = 0.;
    
    munger_alloc(x);
    
    x->twelfth = 1./12.;
    x->semitone = pow(2., 1./12.);
    x->smoothPitch = 1;
    
    x->grate = 1.;				
    x->grate_var = 0.;	
    x->glen = 1.; 				
    x->glen_var = 0.; 		
    x->gpitch = 1.; 		 
    x->gpitch_var = 0.;
    x->gpan_spread = 0.;
    x->time = 0;	
    x->position = -1.; //-1 default == random positioning
    
    x->gimme = 0.;	
    
    x->power = 1;
    x->ambi = 0;
    x->maxvoices = 20;
        
    for(i=0; i<NUMVOICES; i++) {
    	x->gvoiceSize[i] = 1000;			
    	x->gvoiceSpeed[i] = 1.;			
    	x->gvoiceCurrent[i] = 0.;		
    	x->gvoiceDirection[i] = 1;		
    	x->gvoiceOn[i] = 0;			
    	x->gvoiceDone[i] = 0;			
    	x->gvoiceRPan[i] = .5;
    	x->gvoiceLPan[i] = .5;
    	x->gvoiceGain[i] = 1.;
    }
    
    //init hanning window
    x->doHanning = 0;
    for(i=0; i<WINLENGTH; i++) 
    	x->winTable[i] = 0.5 + 0.5*cos(TWOPI * i/WINLENGTH + .5*TWOPI);
    	
    for(i=0; i<PITCHTABLESIZE; i++) {
		x->pitchTable[i] = 0.;
	}
    	
    x->rampLength = 256.;
    
    //sample buffer
    //for(i=0; i<BUFLENGTH; i++) x->recordBuf[i] = 0.;
    for(i=0; i<x->initbuflen; i++) x->recordBuf[i] = 0.;
    x->recordOn = 1; //boolean
	x->recordCurrent = 0;

    srand(0.54);
    //post("mungery away");
    
    return (x);
}


void munger_dsp(t_munger *x, t_signal **sp, short *count)
{
	x->grate_connected 			= count[1];
	x->grate_var_connected		= count[2];
	x->glen_connected			= count[3];
	x->glen_var_connected		= count[4];
	x->gpitch_connected			= count[5];
	x->gpitch_var_connected		= count[6];
	x->gpan_spread_connected	= count[7];
	
	x->srate = sp[0]->s_sr;
    x->one_over_srate = 1./x->srate;

    x->srate_ms = x->srate * .001;
    x->one_over_srate_ms = 1000. * x->one_over_srate;

	dsp_add(munger_perform, 12, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, \
			sp[5]->s_vec, sp[6]->s_vec, sp[7]->s_vec, sp[8]->s_vec, sp[9]->s_vec, sp[0]->s_n);	
	
}

t_int *munger_perform(t_int *w)
{
	t_munger *x = (t_munger *)(w[1]);
	float *in = (float *)(w[2]);
	
	float grate 		= x->grate_connected? 		*(float *)(w[3]) : x->grate;
	float grate_var 	= x->grate_var_connected? 	*(float *)(w[4]) : x->grate_var;
	float glen 			= x->glen_connected? 		*(float *)(w[5]) : x->glen;
	float glen_var 		= x->glen_var_connected? 	*(float *)(w[6]) : x->glen_var;
	float gpitch 		= x->gpitch_connected? 		*(float *)(w[7]) : x->gpitch;
	float gpitch_var	= x->gpitch_var_connected? 	*(float *)(w[8]) : x->gpitch_var;
	float gpan_spread	= x->gpan_spread_connected? *(float *)(w[9]) : x->gpan_spread;
	
	float *outL = (float *)(w[10]);
	float *outR = (float *)(w[11]);
	long n = w[12];
	
	float gimme;
	float outsampL, outsampR, samp;
	int newvoice, i;
		
	//make sure vars are updated if signals are connected; stupid, lazy, boob.
	x->grate = grate;
	x->grate_var = grate_var;
	x->glen = glen;
	x->glen_var = glen_var;
	x->gpitch = gpitch;
	x->gpitch_var = gpitch_var;
	x->gpan_spread = gpan_spread;
		
	//grate = grate + RAND01 * grate_var;
	//grate = grate + ((float)rand() - 16384.) * ONE_OVER_HALFRAND * grate_var;
	//gimme = x->srate_ms * grate; //grate is actually time-distance between grains

	if(gpan_spread > 1.) gpan_spread = 1.;
	if(gpan_spread < 0.) gpan_spread = 0.;
	
	if(!x->power) {
		while(n--) {
			*outL++ = 0.;
			*outR++ = 0.;
		}
	}

	else {
		while(n--) {
			outsampL = outsampR = 0.;
			
			//record a sample
			if(x->recordOn) recordSamp(x, *in++);
			
			//find a voice if it's time (high resolution)
			if(x->time++ >= (long)x->gimme) {
				x->time = 0;
				newvoice = findVoice(x);
				if(newvoice >= 0) {
					x->gvoiceCurrent[newvoice] = newSetup(x, newvoice);
				}
				grate = grate + ((float)rand() - 16384.) * ONE_OVER_HALFRAND * grate_var;
				x->gimme = x->srate_ms * grate; //grate is actually time-distance between grains
			}
			
			//mix 'em, pan 'em
			for(i=0; i< x->maxvoices; i++) {
			//for(i=0; i<x->voices; i++) {
				if(x->gvoiceOn[i]) {
					//get a sample, envelope it
					samp = getSamp(x, x->gvoiceCurrent[i]);
					samp = envelope(x, i, samp) * x->gvoiceGain[i];
					
					//pan it, dumb linear
					outsampL += samp * x->gvoiceLPan[i];
					outsampR += samp * x->gvoiceRPan[i];
					
					//see if grain is done after jumping to next sample point
					x->gvoiceCurrent[i] += (double)x->gvoiceDirection[i] * x->gvoiceSpeed[i];
					if(++x->gvoiceDone[i] >= x->gvoiceSize[i]) x->gvoiceOn[i] = 0;
				}
			}
			*outL++ = outsampL;
			*outR++ = outsampR;
		}
	}
	return w + 13;	
}
#endif /* MSP */



/* ----------------------------------- Pure Data --------------------------------------- */
#ifdef PD
/****MAIN STRUCT****/

static t_class *munger_class;

typedef struct _munger
{
	//header
    t_object x_obj;
    
    //user controlled vars
    t_float grate;			//grain rate	
    t_float grate_var;		//grain rate variation; percentage of grain rate
    t_float glen; 			//grain length
    t_float glen_var; 		
    t_float gpitch; 		 
    t_float gpitch_var;
    t_float gpan_spread;		//how much to spread the grains around center
				
	t_float pitchTable[PITCHTABLESIZE]; 	//table of pitch values to draw from
	t_float twelfth; //1/12
	t_float semitone;
	short smoothPitch;
	
	t_float gain, randgain;
	t_float position; //playback position (0-1) (if ==-1, then random, which is default)
	
	t_float buflen;
	t_float maxsize, minsize;
	t_float twothirdBufsize, onethirdBufsize;
	t_float initbuflen;
	long maxvoices;

    //window stuff
    short doHanning;
    t_float winTime[NUMVOICES], winRate[NUMVOICES];
    t_float winTable[WINLENGTH];
    t_float rampLength; //for simple linear ramp
    
    //voice parameters
    long gvoiceSize[NUMVOICES];			//sample size
    double gvoiceSpeed[NUMVOICES];		//1 = at pitch
    double gvoiceCurrent[NUMVOICES];	//current sample position
    int gvoiceDirection[NUMVOICES];		//1 = forward, -1 backwards
    int gvoiceOn[NUMVOICES];			//currently playing? boolean
    long gvoiceDone[NUMVOICES];			//how many samples already played from grain
    t_float gvoiceLPan[NUMVOICES];
    t_float gvoiceRPan[NUMVOICES];
    t_float gvoiceRamp[NUMVOICES];
    t_float gvoiceOneOverRamp[NUMVOICES];
    t_float gvoiceGain[NUMVOICES];
    int voices;
    t_float gimme;
    
    //sample buffer
    t_float *recordBuf;
    int recordOn; //boolean
    long recordCurrent;
    
    //other stuff
    long time; 
    int power;
    short ambi;

    t_float srate, one_over_srate;
    t_float srate_ms, one_over_srate_ms;
} t_munger;



/****FUNCTIONS****/
//creates a size for a new grain
//actual number of samples PLAYED, regardless of pitch
//might be shorter for higher pitches and long grains, to avoid collisions with recordCurrent
//size given now in milliseconds!
static t_float newSize(t_munger *x, int whichOne)
{
	t_float newsize, temp;
	int pitchChoice, pitchExponent;
	
	//set grain pitch
	if(x->smoothPitch == 1) x->gvoiceSpeed[whichOne] = x->gpitch + ((t_float)rand() - 16384.) * ONE_OVER_HALFRAND*x->gpitch_var;
	else {
		temp = (t_float)rand() * ONE_OVER_MAXRAND * x->gpitch_var * (t_float)PITCHTABLESIZE * .1;
		pitchChoice = (int) temp;
		if(pitchChoice > PITCHTABLESIZE) pitchChoice = PITCHTABLESIZE;
		if(pitchChoice < 0) pitchChoice = 0;
		pitchExponent = x->pitchTable[pitchChoice];
		x->gvoiceSpeed[whichOne] = x->gpitch * pow(x->semitone, pitchExponent);
	}
	
	if(x->gvoiceSpeed[whichOne] < MINSPEED) x->gvoiceSpeed[whichOne] = MINSPEED;
	newsize = x->srate_ms*(x->glen + ((t_float)rand() - 16384.) * ONE_OVER_HALFRAND*x->glen_var);
	if(newsize > x->maxsize) newsize = x->maxsize;
	if(newsize*x->gvoiceSpeed[whichOne] > x->maxsize) 
		newsize = x->maxsize/x->gvoiceSpeed[whichOne];
	if(newsize < x->minsize) newsize = x->minsize;
	return newsize;
}	

static int newDirection(t_munger *x)
{
//-1 == always backwards
//0  == backwards and forwards (default)
//1  == only forwards
	int dir;
	if(x->ambi == 0) {
		dir = rand() - 16384;
		if (dir < 0) dir = -1;
		else dir = 1;
	} 
	else 
		if(x->ambi == -1) dir = -1;
	else
		dir = 1;

	return dir;
}

//window stuff
static t_float win_tick(t_munger *x, int whichone)
{
	long temp;
	t_float temp_time, alpha, output;
	
	x->winTime[whichone] += x->winRate[whichone];
	while (x->winTime[whichone] >= (t_float)WINLENGTH) x->winTime[whichone] -= (t_float)WINLENGTH;
	while (x->winTime[whichone] < 0.) x->winTime[whichone] += (t_float)WINLENGTH;
	
	temp_time = x->winTime[whichone];
	
	temp = (long)temp_time;
	alpha = temp_time - (t_float)temp;
	output = x->winTable[temp];
	output = output + (alpha * (x->winTable[temp+1] - output));
	return output;
}

//grain funcs
static t_float envelope(t_munger *x, int whichone, t_float sample)
{
	long done = x->gvoiceDone[whichone];
	long tail = x->gvoiceSize[whichone] - x->gvoiceDone[whichone];
	
	/*
	if(x->doHanning) sample *= win_tick(x, whichone);
	else{
		if(done < x->gvoiceRamp[whichone]) sample *= (done*x->gvoiceOneOverRamp[whichone]);
		else if(tail < x->gvoiceRamp[whichone]) sample *= (tail*x->gvoiceOneOverRamp[whichone]);
	}
	*/
	if(done < x->gvoiceRamp[whichone]) sample *= (done*x->gvoiceOneOverRamp[whichone]);
	else if(tail < x->gvoiceRamp[whichone]) sample *= (tail*x->gvoiceOneOverRamp[whichone]);
	
	return sample;
}

//tries to find an available voice; return -1 if no voices available
static int findVoice(t_munger *x)
{
	int i = 0, foundOne = -1;
	while(foundOne < 0 && i < x->voices ) {
		if (!x->gvoiceOn[i]) foundOne = i;
		i++;
	}
	return foundOne;
}

//creates a new (random) start position for a new grain, returns beginning start sample
//sets up size and direction
//max grain size is BUFLENGTH / 3, to avoid recording into grains while they are playing
static t_float newSetup(t_munger *x, int whichVoice)
{
	t_float newPosition = 0.0;
	
	x->gvoiceSize[whichVoice] 		= newSize(x, whichVoice);
	x->gvoiceDirection[whichVoice] 	= newDirection(x);
	x->gvoiceLPan[whichVoice] 		= ((t_float)rand() - 16384.) * ONE_OVER_MAXRAND * x->gpan_spread + 0.5;
	x->gvoiceRPan[whichVoice]		= 1. - x->gvoiceLPan[whichVoice];
	x->gvoiceOn[whichVoice] 		= 1;
	x->gvoiceDone[whichVoice]		= 0;
	x->gvoiceGain[whichVoice]		= x->gain + ((t_float)rand() - 16384.) * ONE_OVER_HALFRAND * x->randgain;
	
	if(x->gvoiceSize[whichVoice] < 2.*x->rampLength) {
		x->gvoiceRamp[whichVoice] = .5 * x->gvoiceSize[whichVoice];
		if(x->gvoiceRamp[whichVoice] <= 0.) x->gvoiceRamp[whichVoice] = 1.;
		x->gvoiceOneOverRamp[whichVoice] = 1./x->gvoiceRamp[whichVoice];
	}
	else {
		x->gvoiceRamp[whichVoice] = x->rampLength;
		if(x->gvoiceRamp[whichVoice] <= 0.) x->gvoiceRamp[whichVoice] = 1.;
		x->gvoiceOneOverRamp[whichVoice] = 1./x->gvoiceRamp[whichVoice];
	}
	
	
/*** set start point; tricky, cause of moving buffer, variable playback rates, backwards/forwards, etc.... ***/

	// 1. random positioning and moving buffer (default)
	if(x->position == -1. && x->recordOn == 1) { 
		if(x->gvoiceDirection[whichVoice] == 1) {//going forward			
			if(x->gvoiceSpeed[whichVoice] > 1.) 
				newPosition = x->recordCurrent - x->onethirdBufsize - (t_float)rand() * ONE_OVER_MAXRAND * x->onethirdBufsize;
			else
				newPosition = x->recordCurrent - (t_float)rand() * ONE_OVER_MAXRAND * x->onethirdBufsize;//was 2/3rds
		}
		
		else //going backwards
			newPosition = x->recordCurrent - (t_float)rand() * ONE_OVER_MAXRAND * x->onethirdBufsize;
	}
	
	// 2. fixed positioning and moving buffer	
	else if (x->position >= 0. && x->recordOn == 1) {
		if(x->gvoiceDirection[whichVoice] == 1) {//going forward			
			if(x->gvoiceSpeed[whichVoice] > 1.) 
				//newPosition = x->recordCurrent - x->onethirdBufsize - x->position * x->onethirdBufsize;
				//this will follow more closely...
				newPosition = x->recordCurrent - x->gvoiceSize[whichVoice]*x->gvoiceSpeed[whichVoice] - x->position * x->onethirdBufsize;
				
			else
				newPosition = x->recordCurrent - x->position * x->onethirdBufsize;//was 2/3rds
		}
		
		else //going backwards
			newPosition = x->recordCurrent - x->position * x->onethirdBufsize;
	}
	
	// 3. random positioning and fixed buffer	
	else if (x->position == -1. && x->recordOn == 0) {
		if(x->gvoiceDirection[whichVoice] == 1) {//going forward			
			newPosition = x->recordCurrent - x->onethirdBufsize - (t_float)rand() * ONE_OVER_MAXRAND * x->onethirdBufsize;
		}	
		else //going backwards
			newPosition = x->recordCurrent - (t_float)rand() * ONE_OVER_MAXRAND * x->onethirdBufsize;
	}
	
	// 4. fixed positioning and fixed buffer	
	else if (x->position >= 0. && x->recordOn == 0) {
		if(x->gvoiceDirection[whichVoice] == 1) {//going forward			
			newPosition = x->recordCurrent - x->onethirdBufsize - x->position * x->onethirdBufsize;
		}	
		else //going backwards
			newPosition = x->recordCurrent - x->position * x->onethirdBufsize;
	}
	
	return newPosition;
	
}	



//buffer funcs
void recordSamp(t_munger *x, float sample)
{
	if(x->recordCurrent >= x->buflen) x->recordCurrent = 0;
	x->recordBuf[x->recordCurrent++] = sample;
}

static t_float getSamp(t_munger *x, double where)
{
	double alpha, om_alpha, output;
	long first;
	
	while(where < 0.) where += x->buflen;
	while(where >= x->buflen) where -= x->buflen;
	
	first = (long)where;
	
	alpha = where - first;
	om_alpha = 1. - alpha;
	
	output = x->recordBuf[first++] * om_alpha;
	if(first <  x->buflen) {
		output += x->recordBuf[first] * alpha;
	}
	else {
		output += x->recordBuf[0] * alpha;
	}
	
	return (t_float)output;
	
}

//primary MSP funcs
static t_int *munger_perform(t_int *w)
{
	t_munger *x = (t_munger *)(w[1]);
	t_float *in = (t_float *)(w[2]);
	
	t_float grate 		= x->grate;
	t_float grate_var 	= x->grate_var;
	t_float glen 			= x->glen;
	t_float glen_var 		= x->glen_var;
	t_float gpitch 		= x->gpitch;
	t_float gpitch_var	= x->gpitch_var;
	t_float gpan_spread	= x->gpan_spread;
	
	t_float *outL = (t_float *)(w[3]);
	t_float *outR = (t_float *)(w[4]);
	long n = w[5];
	
	t_float gimme;
	t_float outsampL, outsampR, samp;
	int newvoice, i;
		
	//make sure vars are updated if signals are connected; stupid, lazy, boob.
	x->grate = grate;
	x->grate_var = grate_var;
	x->glen = glen;
	x->glen_var = glen_var;
	x->gpitch = gpitch;
	x->gpitch_var = gpitch_var;
	x->gpan_spread = gpan_spread;
		
	//grate = grate + RAND01 * grate_var;
	//grate = grate + ((t_float)rand() - 16384.) * ONE_OVER_HALFRAND * grate_var;
	//gimme = x->srate_ms * grate; //grate is actually time-distance between grains

	if(gpan_spread > 1.) gpan_spread = 1.;
	if(gpan_spread < 0.) gpan_spread = 0.;
	
	if(!x->power) {
		while(n--) {
			*outL++ = 0.;
			*outR++ = 0.;
		}
	}

	else {
		while(n--) {
			outsampL = outsampR = 0.;
			
			//record a sample
			if(x->recordOn) recordSamp(x, *in++);
			
			//find a voice if it's time (high resolution)
			if(x->time++ >= (long)x->gimme) {
				x->time = 0;
				newvoice = findVoice(x);
				if(newvoice >= 0) {
					x->gvoiceCurrent[newvoice] = newSetup(x, newvoice);
				}
				grate = grate + ((t_float)rand() - 16384.) * ONE_OVER_HALFRAND * grate_var;
				x->gimme = x->srate_ms * grate; //grate is actually time-distance between grains
			}
			
			//mix 'em, pan 'em
			for(i=0; i< x->maxvoices; i++) {
			//for(i=0; i<x->voices; i++) {
				if(x->gvoiceOn[i]) {
					//get a sample, envelope it
					samp = getSamp(x, x->gvoiceCurrent[i]);
					samp = envelope(x, i, samp) * x->gvoiceGain[i];
					
					//pan it, dumb linear
					outsampL += samp * x->gvoiceLPan[i];
					outsampR += samp * x->gvoiceRPan[i];
					
					//see if grain is done after jumping to next sample point
					x->gvoiceCurrent[i] += (double)x->gvoiceDirection[i] * x->gvoiceSpeed[i];
					if(++x->gvoiceDone[i] >= x->gvoiceSize[i]) x->gvoiceOn[i] = 0;
				}
			}
			*outL++ = outsampL;
			*outR++ = outsampR;
		}
	}
	return w + 6;	
}

static void munger_dsp(t_munger *x, t_signal **sp)
{
	x->srate = sp[0]->s_sr;
    x->one_over_srate = 1./x->srate;

    x->srate_ms = x->srate * .001;
    x->one_over_srate_ms = 1000. * x->one_over_srate;

	dsp_add(munger_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);	
	
}

static void munger_setramp(t_munger *x, t_floatarg temp)
{
	x->doHanning = 0;
	post("munger: setting ramp to: %ld ms", temp);
	x->rampLength = x->srate_ms * (t_float)temp; 
	if(x->rampLength <= 0.) x->rampLength = 1.;
}

static void munger_scale(t_munger *x, t_floatarg temp)
{
	int i,j;
	
	//post("munger: loading scale from input list");
	x->smoothPitch = 0;
	
	for(i=0;i<PITCHTABLESIZE;i++) x->pitchTable[i] = 0.;
/*	if (argc > PITCHTABLESIZE) argc = PITCHTABLESIZE;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				x->pitchTable[i] = (float)argv[i].a_w.w_long; 
				break;
			case A_FLOAT:
				x->pitchTable[i] = argv[i].a_w.w_float;
				break;
		}
	} */

	for(i=0;i<PITCHTABLESIZE;i++) x->pitchTable[i] = temp;

	i=0;
	//wrap input list through all of pitchTable
/*	for (j=argc; j < PITCHTABLESIZE; j++) {
		x->pitchTable[j] = x->pitchTable[i++];
		if (i >= argc) i = 0;
	} */		
}

static void munger_bufsize(t_munger *x, t_floatarg temp)
{
	temp *= x->srate;
	if(temp < 20.*(t_float)MINSIZE) temp = 20.*(t_float)MINSIZE;
	//if(temp > (t_float)BUFLENGTH) temp = (t_float)BUFLENGTH;
	if(temp > x->initbuflen) temp = x->initbuflen;
	//x->buflen = temp;
	//x->maxsize = x->buflen / 3.;
	x->maxsize = temp / 3.;
    x->twothirdBufsize = x->maxsize * 2.;
    x->onethirdBufsize = x->maxsize; 
    post("munger: setting delaylength to: %f seconds", (x->buflen/x->srate));
}

static void munger_bufsize_ms(t_munger *x, t_floatarg temp)
{
	temp *= x->srate_ms;
	if(temp < 20.*(t_float)MINSIZE) temp = 20.*(t_float)MINSIZE;
	//if(temp > (float)BUFLENGTH) temp = (t_float)BUFLENGTH;
	if(temp > x->initbuflen) temp = x->initbuflen;
	//x->buflen = temp;
	//x->maxsize = x->buflen / 3.;
	x->maxsize = temp / 3.;
    x->twothirdBufsize = x->maxsize * 2.;
    x->onethirdBufsize = x->maxsize; 
    post("munger: setting delaylength to: %f seconds", (x->buflen/x->srate));
}

static void munger_setminsize(t_munger *x, t_floatarg temp)
{
	if(temp < (t_float)MINSIZE) temp = (t_float)MINSIZE;
	if(temp >= x->initbuflen) {
		temp = (t_float)MINSIZE;
		post("munger error: minsize too large!");
	}
	x->minsize = temp;
    post("munger: setting min grain size to: %f ms", (x->minsize/x->srate_ms));
}


static void munger_setvoices(t_munger *x, t_floatarg temp)
{
	if(temp < 0) temp = 0;
	if(temp > x->maxvoices) temp = x->maxvoices;
	x->voices = temp;
    //post("munger: setting max voices to: %d ", x->voices);
}

static void munger_maxvoices(t_munger *x, t_floatarg temp)
{
	if(temp < 0) temp = 0;
	if(temp > NUMVOICES) temp = NUMVOICES;
	x->maxvoices = temp;
    //post("munger: setting max voices to: %d ", x->voices);
}


static void setpower(t_munger *x, t_floatarg temp)
{
	x->power = temp;
    post("munger: setting power: %d", temp);
}

static void munger_record(t_munger *x, t_floatarg temp)
{
	x->recordOn = temp;
    post("munger: setting record: %d", temp);
}

static void munger_ambidirectional(t_munger *x, t_floatarg temp)
{
	x->ambi = temp;
    post("munger: setting ambidirectional: %d", temp);
}

static void munger_gain(t_munger *x, t_floatarg temp)
{
    post("munger: setting gain to: %f ", temp);
    x->gain = temp;
}

static void munger_setposition(t_munger *x, t_floatarg temp)
{
	if (temp > 1.) temp = 1.;
	if (temp == -1.) 
    	post("munger: random positioning");
    else if (temp < 0.) 
    	temp = 0.;
    //post("munger: setting position to: %f ", temp);
    x->position = temp;
}

static void munger_randgain(t_munger *x, t_floatarg temp)
{
    post("munger: setting rand_gain to: %f ", temp);
    x->randgain = temp;
}

static void munger_sethanning(t_munger *x)
{
	post("munger: hanning window is busted");
	x->doHanning = 1;
}

static void munger_tempered(t_munger *x)
{
	int i;
	//post("munger: doing tempered scale");
	x->smoothPitch = 0;
	for(i=0; i<PITCHTABLESIZE-1; i += 2) {
		x->pitchTable[i] = 0.5*i;
		x->pitchTable[i+1] = -0.5*i;
	}
}
static void munger_smooth(t_munger *x)
{
	//post("munger: doing smooth scale");
	x->smoothPitch = 1;
}

static void munger_alloc(t_munger *x)
{
	//x->recordBuf = t_getbytes(BUFLENGTH * sizeof(float));
	x->recordBuf = t_getbytes(x->buflen * sizeof(t_float));
	if (!x->recordBuf) {
		error("munger: out of memory");
		return;
	}
}

static void munger_free(t_munger *x)
{
	if (x->recordBuf)
		//t_freebytes(x->recordBuf, BUFLENGTH * sizeof(float));
		t_freebytes(x->recordBuf, x->initbuflen * sizeof(t_float));
}



static void munger_float(t_munger *x, t_floatarg f)
{
	x->grate = f;

}
static void munger_grate_var(t_munger *x, t_floatarg f)
{
	x->grate_var = f;
}
static void munger_glen(t_munger *x, t_floatarg f)
{
	x->glen = f;
}
static void munger_glen_var(t_munger *x, t_floatarg f)
{
	x->glen_var = f;
}
static void munger_gpitch(t_munger *x, t_floatarg f)
{
	x->gpitch = f;
}
static void munger_gpitch_var(t_munger *x, t_floatarg f)
{
	x->gpitch_var = f;
}
static void munger_gpan_spread(t_munger *x, t_floatarg f)
{
	x->gpan_spread = f;
}


//hanning y = 0.5 + 0.5*cos(TWOPI * n/N + PI)

static void *munger_new(t_floatarg maxdelay)
{
	unsigned int i;
    
    t_munger *x = (t_munger *)pd_new(munger_class);
    //zero out the struct, to be careful (takk to jkclayton)
    if (x) { 
        for(i=sizeof(t_object);i<sizeof(t_munger);i++)  
                ((char *)x)[i]=0; 
	} 
	
	if (maxdelay < 100.) maxdelay = 3000.; //set maxdelay to 3000ms by default
	post("munger: maxdelay = %f milliseconds", maxdelay);
	
	outlet_new(&x->x_obj, gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
    			   
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("grate_var"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("glen"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("glen_var"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("gpitch"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("gpitch_var"));
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("gpan_spread"));

    x->srate = sys_getsr();
    x->one_over_srate = 1./x->srate;
    x->srate_ms = x->srate/1000.;
    x->one_over_srate_ms = 1./x->srate_ms;
    
    //x->buflen = (float)BUFLENGTH;
    if (x->recordBuf)
		t_freebytes(x->recordBuf, x->initbuflen * sizeof(t_float));
		
    x->initbuflen = (t_float)maxdelay * 44.1;
    x->buflen = x->initbuflen;
    x->maxsize = x->buflen / 3.;
    x->twothirdBufsize = x->maxsize * 2.;
    x->onethirdBufsize = x->maxsize;
    x->minsize = MINSIZE;
    x->voices = 10;
    x->gain = 1.;
    x->randgain = 0.;
    
    munger_alloc(x);
    
    x->twelfth = 1./12.;
    x->semitone = pow(2., 1./12.);
    x->smoothPitch = 1;
    
    x->grate = 1.;				
    x->grate_var = 0.;	
    x->glen = 1.; 				
    x->glen_var = 0.; 		
    x->gpitch = 1.; 		 
    x->gpitch_var = 0.;
    x->gpan_spread = 0.;
    x->time = 0;	
    x->position = -1.; //-1 default == random positioning
    
    x->gimme = 0.;	
    
    x->power = 1;
    x->ambi = 0;
    x->maxvoices = 20;
        
    for(i=0; i<NUMVOICES; i++) {
    	x->gvoiceSize[i] = 1000;			
    	x->gvoiceSpeed[i] = 1.;			
    	x->gvoiceCurrent[i] = 0.;		
    	x->gvoiceDirection[i] = 1;		
    	x->gvoiceOn[i] = 0;			
    	x->gvoiceDone[i] = 0;			
    	x->gvoiceRPan[i] = .5;
    	x->gvoiceLPan[i] = .5;
    	x->gvoiceGain[i] = 1.;
    }
    
    //init hanning window
    x->doHanning = 0;
    for(i=0; i<WINLENGTH; i++) 
    	x->winTable[i] = 0.5 + 0.5*cos(TWOPI * i/WINLENGTH + .5*TWOPI);
    	
    for(i=0; i<PITCHTABLESIZE; i++) {
		x->pitchTable[i] = 0.;
	}
    	
    x->rampLength = 256.;
    
    //sample buffer
    //for(i=0; i<BUFLENGTH; i++) x->recordBuf[i] = 0.;
    for(i=0; i<x->initbuflen; i++) x->recordBuf[i] = 0.;
    x->recordOn = 1; //boolean
	x->recordCurrent = 0;

    srand(0.54);
    //post("mungery away");
    
    return (x);
}


void munger_tilde_setup(void)
{
	munger_class = class_new(gensym("munger~"), (t_newmethod)munger_new, (t_method)munger_free,
        sizeof(t_munger), 0, A_DEFFLOAT, 0);
    class_addmethod(munger_class, nullfn, gensym("signal"), A_NULL);
    class_addmethod(munger_class, (t_method)munger_dsp, gensym("dsp"), A_NULL);
	class_addfloat(munger_class, (t_method)munger_float);
    class_addmethod(munger_class, (t_method)munger_setramp, gensym("ramptime"), A_NULL);
    class_addmethod(munger_class, (t_method)munger_sethanning, gensym("hanning"), A_NULL);
    class_addmethod(munger_class, (t_method)munger_tempered, gensym("tempered"), A_NULL);
    class_addmethod(munger_class, (t_method)munger_smooth, gensym("smooth"), A_NULL);
    class_addmethod(munger_class, (t_method)munger_scale, gensym("scale"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_bufsize, gensym("delaylength"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_bufsize_ms, gensym("delaylength_ms"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_setvoices, gensym("voices"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_maxvoices, gensym("maxvoices"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_setminsize, gensym("minsize"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)setpower, gensym("power"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_ambidirectional, gensym("ambidirectional"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_record, gensym("record"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_gain, gensym("gain"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_randgain, gensym("rand_gain"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_setposition, gensym("position"), A_FLOAT, A_NULL);

    class_addmethod(munger_class, (t_method)munger_grate_var, gensym("grate_var"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_glen, gensym("glen"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_glen_var, gensym("glen_var"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_gpitch, gensym("gpitch"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_gpitch_var, gensym("gpitch_var"), A_FLOAT, A_NULL);
    class_addmethod(munger_class, (t_method)munger_gpan_spread, gensym("gpan_spread"), A_FLOAT, A_NULL);
    class_sethelpsymbol(munger_class, gensym("help-munger~.pd"));
}
#endif /* PD */
