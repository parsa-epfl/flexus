#ifndef FLEXUS_POWER_DEFINITIONS_HPP_INCLUDED
#define FLEXUS_POWER_DEFINITIONS_HPP_INCLUDED

/*  The following are things you might want to change
 *  when compiling
 */

/*
 * The output can be in 'long' format, which shows everything, or
 * 'short' format, which is just what a program like 'grap' would
 * want to see
 */

#define LONG 1
#define SHORT 2

#define OUTPUTTYPE LONG

/* Do we want static AFs (STATIC_AF) or Dynamic AFs (DYNAMIC_AF) */
/* #define DYNAMIC_AF */
#define DYNAMIC_AF

/*
 * Address bits in a word, and number of output bits from the cache
 */

#define ADDRESS_BITS 64
#define BITOUT 64

/* limits on the various N parameters */

#define MAXN 8            /* Maximum for Ndwl,Ntwl,Ndbl,Ntbl */
#define MAXSUBARRAYS 8    /* Maximum subarrays for data and tag arrays */
#define MAXSPD 8          /* Maximum for Nspd, Ntspd */

/*===================================================================*/

/*
 * The following are things you probably wouldn't want to change.
 */

#define TRUE 1
#define FALSE 0
#define OK 1
#define ERROR 0
#define BIGNUM 1e30
#define DIVIDE(a,b) ((b)==0)? 0:(a)/(b)
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

/* Used to communicate with the horowitz model */

#define RISE 1
#define FALL 0
#define NCH  1
#define PCH  0

/*
 * The following scale factor can be used to scale between technologies.
 * To convert from 0.8um to 0.5um, make FUDGEFACTOR = 1.6
 */

#define FUDGEFACTOR 1.0

/*===================================================================*/

/*
 * Cache layout parameters and process parameters
 * Thanks to Glenn Reinman for the technology scaling factors
 */

#define GEN_POWER_FACTOR 1.31

// Butts and Sohi leakage model
#define K_D_FF     1.4
#define K_D_LATCH  2.0
#define K_MUX      1.9
#define K_RAM      1.2
#define K_CAM      1.7
#define K_STATIC   11
#define TURNOFF_FACTOR 0.10 // For the result bus since we don't have a device count

#define TECH_POINT22

#if defined(TECH_POINT22) // 22nm from PTM high-performance models
#define VDD 0.8

// CSCALE and RSCALE are from fitting a*exp(b*x) models to other Wattch technology points
// SSCALE is from a linear model
#define CSCALE  (304.1156656768918)
#define RSCALE  (272.2669559878673)
#define LSCALE  0.0275   /* length (feature) scaling factor */
#define ASCALE  (LSCALE*LSCALE) /* area scaling factor */
#define SSCALE   0.7491834  /* sense voltage scaling factor */

// Below are from PTM 22nm hi-K metal gate models with VDD of 0.8 V at 75C
#define VTSCALE  0.239424220183486 // Vth = 260.9724 mV (average of NFET and PFET)

#define GEN_POWER_SCALE (1/GEN_POWER_FACTOR)

// For computing clock network power, in millimeters
#define CORE_WIDTH  (0.0018714)
#define CORE_HEIGHT (0.00103245)
#define DIE_WIDTH   (4*CORE_WIDTH)
#define DIE_HEIGHT  (8*CORE_HEIGHT)

// Base leakage power at Vdd = 0.8V, 100C, no body bias, nominal process conditions
// Assume everything is made of inverters
// We have the total number of transistors, or 2x the number of inverters
// The leakage power computed in our HSPICE simulations is for 2 inverters, or 4 transistors, so we divide by 4 and multiply by N_DEVICES later
#define BASE_LEAKAGE_POWER_LOGIC (10.7798160052800e-009 / 4)
#define BASE_LEAKAGE_POWER_CACHE (10.7798160052800e-009 / (4*8)) // HSPICE simulations and logic assumed width of 8x nominal. Use minimum size devices for cache.
#define BASE_LEAKAGE_TEMP 373.15          // Baseline leakage is at 100C

#endif

/*
 * CMOS 0.8um model parameters
 *   - from Appendix II of Cacti tech report
 */

/* corresponds to 8um of m3 @ 225ff/um */
#define Cwordmetal    (1.8e-15 * (CSCALE * ASCALE))

/* corresponds to 16um of m2 @ 275ff/um */
#define Cbitmetal     (4.4e-15 * (CSCALE * ASCALE))

/* corresponds to 1um of m2 @ 275ff/um */
#define Cmetal        Cbitmetal/16

#define CM3metal        Cbitmetal/16
#define CM2metal        Cbitmetal/16

/* #define Cmetal 1.222e-15 */

/* fF/um2 at 1.5V */
#define Cndiffarea    0.137e-15  /* FIXME: ??? */

/* fF/um2 at 1.5V */
#define Cpdiffarea    0.343e-15  /* FIXME: ??? */

/* fF/um at 1.5V */
#define Cndiffside    0.275e-15  /* in general this does not scale */

/* fF/um at 1.5V */
#define Cpdiffside    0.275e-15  /* in general this does not scale */

/* fF/um at 1.5V */
#define Cndiffovlp    0.138e-15  /* FIXME: by depth??? */

/* fF/um at 1.5V */
#define Cpdiffovlp    0.138e-15  /* FIXME: by depth??? */

/* fF/um assuming 25% Miller effect */
#define Cnoxideovlp   0.263e-15  /* FIXME: by depth??? */

/* fF/um assuming 25% Miller effect */
#define Cpoxideovlp   0.338e-15  /* FIXME: by depth??? */

/* um */
#define Leff          (0.8 * LSCALE)

/* fF/um2 */
#define Cgate         1.95e-15  /* FIXME: ??? */

/* fF/um2 */
#define Cgatepass     1.45e-15  /* FIXME: ??? */

/* note that the value of Cgatepass will be different depending on
   whether or not the source and drain are at different potentials or
   the same potential.  The two values were averaged */

/* fF/um */
#define Cpolywire (0.25e-15 * CSCALE * LSCALE)

/* ohms*um of channel width */
#define Rnchannelstatic (25800 * LSCALE)

/* ohms*um of channel width */
#define Rpchannelstatic (61200 * LSCALE)

#define Rnchannelon (9723 * LSCALE)

#define Rpchannelon (22400 * LSCALE)

/* corresponds to 16um of m2 @ 48mO/sq */
#define Rbitmetal (0.320 * (RSCALE * ASCALE))

/* corresponds to  8um of m3 @ 24mO/sq */
#define Rwordmetal (0.080 * (RSCALE * ASCALE))

/* other stuff (from tech report, appendix 1) */

#define Period          (1/frq)

#define krise  (0.4e-9 * LSCALE)
#define tsensedata (5.8e-10 * LSCALE)
#define tsensetag (2.6e-10 * LSCALE)
#define tfalldata (7e-10 * LSCALE)
#define tfalltag (7e-10 * LSCALE)
#define Vbitpre  (3.3 * SSCALE)
#define Vt  (1.09 * VTSCALE)
#define Vbitsense (0.10 * SSCALE)

#define Powerfactor (frq*vdd*vdd)

#define SensePowerfactor3 (frq)*(Vbitsense)*(Vbitsense)
#define SensePowerfactor2 (frq)*(Vbitpre-Vbitsense)*(Vbitpre-Vbitsense)
#define SensePowerfactor (frq)*(vdd/2)*(vdd/2)

#define AF    .5
#define POPCOUNT_AF  (23.9/64.0)

/* Threshold voltages (as a proportion of vdd)
   If you don't know them, set all values to 0.5 */

#define VSINV         0.456
#define VTHINV100x60  0.438   /* inverter with p=100,n=60 */
#define VTHNAND60x90  0.561   /* nand with p=60 and three n=90 */
#define VTHNOR12x4x1  0.503   /* nor with p=12, n=4, 1 input */
#define VTHNOR12x4x2  0.452   /* nor with p=12, n=4, 2 inputs */
#define VTHNOR12x4x3  0.417   /* nor with p=12, n=4, 3 inputs */
#define VTHNOR12x4x4  0.390   /* nor with p=12, n=4, 4 inputs */
#define VTHOUTDRINV    0.437
#define VTHOUTDRNOR   0.431
#define VTHOUTDRNAND  0.441
#define VTHOUTDRIVE   0.425
#define VTHCOMPINV    0.437
#define VTHMUXDRV1    0.437
#define VTHMUXDRV2    0.486
#define VTHMUXDRV3    0.437
#define VTHEVALINV    0.267
#define VTHSENSEEXTDRV  0.437

/* transistor widths in um (as described in tech report, appendix 1) */
#define Wdecdrivep (57.0 * LSCALE)
#define Wdecdriven (40.0 * LSCALE)
#define Wdec3to8n (14.4 * LSCALE)
#define Wdec3to8p (14.4 * LSCALE)
#define WdecNORn (5.4 * LSCALE)
#define WdecNORp (30.5 * LSCALE)
#define Wdecinvn (5.0 * LSCALE)
#define Wdecinvp (10.0  * LSCALE)

#define Wworddrivemax (100.0 * LSCALE)
#define Wmemcella (2.4 * LSCALE)
#define Wmemcellr (4.0 * LSCALE)
#define Wmemcellw (2.1 * LSCALE)
#define Wmemcellbscale 2  /* means 2x bigger than Wmemcella */
#define Wbitpreequ (10.0 * LSCALE)

#define Wbitmuxn (10.0 * LSCALE)
#define WsenseQ1to4 (4.0 * LSCALE)
#define Wcompinvp1 (10.0 * LSCALE)
#define Wcompinvn1 (6.0 * LSCALE)
#define Wcompinvp2 (20.0 * LSCALE)
#define Wcompinvn2 (12.0 * LSCALE)
#define Wcompinvp3 (40.0 * LSCALE)
#define Wcompinvn3 (24.0 * LSCALE)
#define Wevalinvp (20.0 * LSCALE)
#define Wevalinvn (80.0 * LSCALE)

#define Wcompn  (20.0 * LSCALE)
#define Wcompp  (30.0 * LSCALE)
#define Wcomppreequ     (40.0 * LSCALE)
#define Wmuxdrv12n (30.0 * LSCALE)
#define Wmuxdrv12p (50.0 * LSCALE)
#define WmuxdrvNANDn    (20.0 * LSCALE)
#define WmuxdrvNANDp    (80.0 * LSCALE)
#define WmuxdrvNORn (60.0 * LSCALE)
#define WmuxdrvNORp (80.0 * LSCALE)
#define Wmuxdrv3n (200.0 * LSCALE)
#define Wmuxdrv3p (480.0 * LSCALE)
#define Woutdrvseln (12.0 * LSCALE)
#define Woutdrvselp (20.0 * LSCALE)
#define Woutdrvnandn (24.0 * LSCALE)
#define Woutdrvnandp (10.0 * LSCALE)
#define Woutdrvnorn (6.0 * LSCALE)
#define Woutdrvnorp (40.0 * LSCALE)
#define Woutdrivern (48.0 * LSCALE)
#define Woutdriverp (80.0 * LSCALE)

#define Wcompcellpd2    (2.4 * LSCALE)
#define Wcompdrivern    (400.0 * LSCALE)
#define Wcompdriverp    (800.0 * LSCALE)
#define Wcomparen2      (40.0 * LSCALE)
#define Wcomparen1      (20.0 * LSCALE)
#define Wmatchpchg      (10.0 * LSCALE)
#define Wmatchinvn      (10.0 * LSCALE)
#define Wmatchinvp      (20.0 * LSCALE)
#define Wmatchnandn     (20.0 * LSCALE)
#define Wmatchnandp     (10.0 * LSCALE)
#define Wmatchnorn     (20.0 * LSCALE)
#define Wmatchnorp     (10.0 * LSCALE)

#define WSelORn         (10.0 * LSCALE)
#define WSelORprequ     (40.0 * LSCALE)
#define WSelPn          (10.0 * LSCALE)
#define WSelPp          (15.0 * LSCALE)
#define WSelEnn         (5.0 * LSCALE)
#define WSelEnp         (10.0 * LSCALE)

#define Wsenseextdrv1p  (40.0*LSCALE)
#define Wsenseextdrv1n  (24.0*LSCALE)
#define Wsenseextdrv2p  (200.0*LSCALE)
#define Wsenseextdrv2n  (120.0*LSCALE)

/* bit width of RAM cell in um */
#define BitWidth (16.0 * LSCALE)

/* bit height of RAM cell in um */
#define BitHeight (16.0 * LSCALE)

#define Cout  (0.5e-12 * LSCALE)

/* Sizing of cells and spacings */
#define RatCellHeight    (40.0 * LSCALE)
#define RatCellWidth     (70.0 * LSCALE)
#define RatShiftRegWidth (120.0 * LSCALE)
#define RatNumShift      4
#define BitlineSpacing   (6.0 * LSCALE)
#define WordlineSpacing  (6.0 * LSCALE)

#define RegCellHeight    (16.0 * LSCALE)
#define RegCellWidth     (8.0  * LSCALE)

#define CamCellHeight    (40.0 * LSCALE)
#define CamCellWidth     (25.0 * LSCALE)
#define MatchlineSpacing (6.0 * LSCALE)
#define TaglineSpacing   (6.0 * LSCALE)

/*===================================================================*/

/* ALU POWER NUMBERS for .18um 733frq */
/* normalize to cap from W */
#define NORMALIZE_SCALE (1.0/(733.0e6*1.45*1.45))
/* normalize .18um cap to other gen's cap, then xPowerfactor */
#define POWER_SCALE    (GEN_POWER_SCALE * NORMALIZE_SCALE * Powerfactor)
#define I_ADD          ((.37 - .091) * POWER_SCALE)
#define I_ADD32        (((.37 - .091)/2)*POWER_SCALE)
#define I_MULT16       ((.31-.095)*POWER_SCALE)
#define I_SHIFT        ((.21-.089)*POWER_SCALE)
#define I_LOGIC        ((.04-.015)*POWER_SCALE)
#define F_ADD          ((1.307-.452)*POWER_SCALE)
#define F_MULT         ((1.307-.452)*POWER_SCALE)

#define I_ADD_CLOCK    (.091*POWER_SCALE)
#define I_MULT_CLOCK   (.095*POWER_SCALE)
#define I_SHIFT_CLOCK  (.089*POWER_SCALE)
#define I_LOGIC_CLOCK  (.015*POWER_SCALE)
#define F_ADD_CLOCK    (.452*POWER_SCALE)
#define F_MULT_CLOCK   (.452*POWER_SCALE)

#define SensePowerfactor (frq)*(vdd/2)*(vdd/2)
#define Sense2Powerfactor (frq)*(2*.3+.1*vdd)
#define Powerfactor (frq*vdd*vdd)
#define LowSwingPowerfactor (frq)*.2*.2
/* set scale for crossover (vdd->gnd) currents */
#define crossover_scaling 1.2
/* set non-ideal turnoff percentage */
#define turnoff_factor 0.10

#define MSCALE (LSCALE * .624 / .2250)

#define opcode_length 8
#define inst_length 32

#endif
