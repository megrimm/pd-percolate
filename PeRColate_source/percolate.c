/******************************************/
/*  PeRColate library for Pure Data       */
/*                                        */
/*  written for Max/MSP by Dan Trueman    */
/*  and R. Luke Dubois                    */
/*										  */
/*  ported to PD by Olaf Matthes, 2002	  */
/*                                        */
/******************************************/

#ifndef VERSION
#define VERSION "0.09"
#endif

#include <m_pd.h>


#ifndef __DATE__ 
#define __DATE__ "without using a gnu compiler"
#endif

typedef struct _percolate
{
     t_object x_obj;
} t_percolate;

static t_class* percolate_class;

// tilde objects
void plucked_tilde_setup(void);
void mandolin_tilde_setup(void);
void flute_tilde_setup(void);
void blotar_tilde_setup(void);
void bowed_tilde_setup(void);
void bowedbar_tilde_setup(void);
void clarinet_tilde_setup(void);
void brass_tilde_setup(void);
void agogo_tilde_setup(void);
void marimba_tilde_setup(void);
void vibraphone_tilde_setup(void);
void bamboo_tilde_setup(void);
void cabasa_tilde_setup(void);
void guiro_tilde_setup(void);
void metashake_tilde_setup(void);
void sekere_tilde_setup(void);
void shaker_tilde_setup(void);
void sleigh_tilde_setup(void);
void tamb_tilde_setup(void);
void wuter_tilde_setup(void);
void gen5_setup(void);
void gen7_setup(void);
void gen9_setup(void);
void gen10_setup(void);
void gen17_setup(void);
void gen24_setup(void);
void gen25_setup(void);
void escalator_tilde_setup(void);
void flip_tilde_setup(void);
void jitter_tilde_setup(void);
void klutz_tilde_setup(void);
void random_tilde_setup(void);
void chase_tilde_setup(void);
void terrain_tilde_setup(void);
void waffle_tilde_setup(void);
void weave_tilde_setup(void);
void dcblock_tilde_setup(void);
void gQ_tilde_setup(void);
void munger_tilde_setup(void);
void scrub_tilde_setup(void);
void absmin_tilde_setup(void);
void absmax_tilde_setup(void);

static void* percolate_new(t_symbol* s) {
    t_percolate *x = (t_percolate *)pd_new(percolate_class);
    return (x);
}

void percolate_setup(void) 
{
    percolate_class = class_new(gensym("PeRColate"), (t_newmethod)percolate_new, 0,
    	sizeof(t_percolate), 0,0);

	 /* physical modeling */
     plucked_tilde_setup();     
	 mandolin_tilde_setup();
	 flute_tilde_setup();
     blotar_tilde_setup();
     bowed_tilde_setup();
     bowedbar_tilde_setup();
     clarinet_tilde_setup();
     brass_tilde_setup();
	 /* modal synthesis */
     agogo_tilde_setup();
     marimba_tilde_setup();
     vibraphone_tilde_setup();
	 /* PhISM */
	 bamboo_tilde_setup();
     cabasa_tilde_setup();
     guiro_tilde_setup();
	 metashake_tilde_setup();
	 sekere_tilde_setup();
	 shaker_tilde_setup();
     sleigh_tilde_setup();
	 tamb_tilde_setup();
	 wuter_tilde_setup();
	 /* MaxGens */
     gen5_setup();
     gen7_setup();
     gen9_setup();
     gen10_setup();
     gen17_setup();
     gen24_setup();
     gen25_setup();
	 /* SID */
     absmax_tilde_setup();
     absmin_tilde_setup();
     escalator_tilde_setup();
     flip_tilde_setup();
     jitter_tilde_setup();
     klutz_tilde_setup();
     random_tilde_setup();
     chase_tilde_setup();
	 terrain_tilde_setup();
	 waffle_tilde_setup();
     weave_tilde_setup();
	 /* Random DSP */
     dcblock_tilde_setup();
     gQ_tilde_setup();
     munger_tilde_setup();
     scrub_tilde_setup();

     post("PeRColate: written for Max/MSP by Dan Trueman and R. Luke DuBois");
     post("PeRColate: ported to PD by Olaf Matthes <olaf.matthes@gmx.de>");
     post("PeRColate: adapted to Linux by Maurizio Umberto Puxeddu <umbpux@tin.it>");
	 post("PeRColate: help files ported by Martin Dupras <martin.dupras@uwe.ac.uk>");
     post("PeRColate: version: "VERSION);
     post("PeRColate: compiled: "__DATE__", "__TIME__);
     post("PeRColate: home: http://www.akustische-kunst.org/puredata/percolate/");
	 post("PeRColate: Physical Modeling: plucked~ mandolin~ flute~ bowed~ bowedbar~ ");
	 post("PeRColate:                    clarinet~ brass~");
	 post("PeRColate: Modal Synthesis:   agogo~ marimba~ vibraphone~");
	 post("PeRColate: PhISM:             bamboo~ cabasa~ guiro~ metashake~ sekere~");
	 post("PeRColate:                    shaker~ sleigh~ tamb~ wuter~");
	 post("PeRColate: MaxGens:           gen5 gen7 gen9 gen10 gen17 gen24 gen25");
	 post("PeRColate: SID:               absmax~ absmin~ escalator~ flip~ jitter~ ");
	 post("PeRColate:                    klutz~ random~ chase~ terrain~ waffle~ weave~");
	 post("PeRColate: Random DSP:        dcblock~ gQ~ munger~ scrub~");
}
