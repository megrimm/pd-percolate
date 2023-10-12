lib.name = pd-percolate


sdk := PeRColate_source/_source/stk.c

cflags = -I PeRColate_source/_source/ -I PeRColate_source/_headers/

# 1_Physical_Models
blotar~.class.sources := PeRColate_source/1_Physical_Models/blotar/blotar~.c $(sdk)
bowed~.class.sources := PeRColate_source/1_Physical_Models/bowed/bowed~.c $(sdk)
bowedbar~.class.sources := PeRColate_source/1_Physical_Models/bowedbar/bowedbar~.c $(sdk)
brass~.class.sources := PeRColate_source/1_Physical_Models/brass/brass~.c $(sdk)
clarinet~.class.sources := PeRColate_source/1_Physical_Models/clarinet/clar~.c $(sdk)
flute~.class.sources := PeRColate_source/1_Physical_Models/flute/flute~.c $(sdk)
mandolin~.class.sources := PeRColate_source/1_Physical_Models/mandolin/mandolin~.c $(sdk)
plucked~.class.sources := PeRColate_source/1_Physical_Models/plucked/plucked~.c $(sdk)

# 2_Modal_Synthesis
agogo~.class.sources := PeRColate_source/2_Modal_Synthesis/agogo/agogo~.c $(sdk)
marimba~.class.sources := PeRColate_source/2_Modal_Synthesis/marimba/marimba~.c $(sdk)
vibraphone~.class.sources := PeRColate_source/2_Modal_Synthesis/vibraphone/vibraphone~.c $(sdk) 

# 3_PhISM
bamboo~.class.sources := PeRColate_source/3_PhISM/bamboo/bamboo~.c 
cabasa~.class.sources := PeRColate_source/3_PhISM/cabasa/cabasa~.c
guiro~.class.sources := PeRColate_source/3_PhISM/guiro/guiro~.c
metashake~.class.sources := PeRColate_source/3_PhISM/meta-shaker/metashake~.c
sekere~.class.sources := PeRColate_source/3_PhISM/sekere/sekere~.c
shaker~.class.sources := PeRColate_source/3_PhISM/shaker/shaker~.c
sleigh~.class.sources := PeRColate_source/3_PhISM/sleighbells/sleigh~.c
tamb~.class.sources := PeRColate_source/3_PhISM/tamb/tamb~.c
wuter~.class.sources := PeRColate_source/3_PhISM/wuter/wuter~.c

# 4_MaxGens
gen5.class.sources := PeRColate_source/4_MaxGens/gen5/gen5.c
gen7.class.sources := PeRColate_source/4_MaxGens/gen7/gen7.c
gen9.class.sources := PeRColate_source/4_MaxGens/gen9/gen9.c
gen10.class.sources := PeRColate_source/4_MaxGens/gen10/gen10.c
gen17.class.sources := PeRColate_source/4_MaxGens/gen17/gen17.c
gen20.class.sources := PeRColate_source/4_MaxGens/gen20/gen20.c
gen24.class.sources := PeRColate_source/4_MaxGens/gen24/gen24.c
gen25.class.sources := PeRColate_source/4_MaxGens/gen25/gen25.c

# 5_SID
absmax~.class.sources := PeRColate_source/5_SID/absmax~/absmax~.c
absmin~.class.sources := PeRColate_source/5_SID/absmin~/absmin~.c
chase~.class.sources := PeRColate_source/5_SID/chase~/chase~.c
escalator~.class.sources := PeRColate_source/5_SID/escal~/escalator~.c
flip~.class.sources := PeRColate_source/5_SID/flip~/flip~.c
jitter~.class.sources := PeRColate_source/5_SID/jitter~/jitter~.c
klutz~.class.sources := PeRColate_source/5_SID/klutz~/klutz~.c 
random~.class.sources := PeRColate_source/5_SID/random~/random~.c
terrain~.class.sources := PeRColate_source/5_SID/terrain~/terrain~.c
waffle~.class.sources := PeRColate_source/5_SID/waffle~/waffle~.c
weave~.class.sources := PeRColate_source/5_SID/weave~/weave~.c

# 6_Random_DSP
dcblock~.class.sources := PeRColate_source/6_Random_DSP/dcblock/dcblock~.c
gQ~.class.sources := PeRColate_source/6_Random_DSP/gQ/gQ~.c
munger~.class.sources := PeRColate_source/6_Random_DSP/munger/munger~.c
scrub~.class.sources := PeRColate_source/6_Random_DSP/scrubber/scrub~.c

PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

