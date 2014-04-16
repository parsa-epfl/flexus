/*------------------------------------------------------------
 *  Copyright 1994 Digital Equipment Corporation and Steve Wilton
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-
 * free right and license under any changes, enhancements or extensions
 * made to the core functions of the software, including but not limited to
 * those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to Digital any
 * such changes, enhancements or extensions that they make and inform Digital
 * of noteworthy uses of this software.  Correspondence should be provided
 * to Digital at:
 *
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/

// The Flexus PowerTracker uses power computation routines taken from Wattch,
// which is available at http://www.eecs.harvard.edu/~dbrooks/wattch-form.html.
// Wattch was developed by David Brooks (dbrooks@eecs.harvard.edu) and Margaret
// Martonosi (mrm@Princeton.edu)

#include <cmath>
#include <cassert>
#include "def.hpp"
#include "WattchFunctions.hpp"

double logtwo(double x) {
  assert(x > 0);
  return static_cast<double>(log(x) / log(2.0));
}


/* returns gate capacitance in Farads */
/* gate width in um (length is Leff) */
/* poly wire length going to gate in lambda */
double gatecap(double width, double wirelength) {
  return width * Leff * Cgate + wirelength * Cpolywire * Leff;
}

/* returns gate capacitance in Farads */
/* gate width in um (length is Leff) */
/* poly wire length going to gate in lambda */
double gatecappass(double width, double wirelength) {
  return width * Leff * Cgatepass + wirelength * Cpolywire * Leff ;
}


/* Routine for calculating drain capacitances.  The draincap routine
 * folds transistors larger than 10um */
/* returns drain cap in Farads */
/* width in um */
/* whether n or p-channel (boolean) */
/* number of transistors in series that are on */
double draincap(double width, bool nchannel, int32_t stack) {
  double Cdiffside, Cdiffarea, Coverlap, cap;

  Cdiffside = (nchannel) ? Cndiffside : Cpdiffside;
  Cdiffarea = (nchannel) ? Cndiffarea : Cpdiffarea;
  Coverlap = (nchannel) ? (Cndiffovlp + Cnoxideovlp) : (Cpdiffovlp + Cpoxideovlp);

  /* calculate directly-connected (non-stacked) capacitance */
  /* then add in capacitance due to stacking */
  if (width >= 10) {
    cap = 3.0 * Leff * width / 2.0 * Cdiffarea + 6.0 * Leff * Cdiffside +
          width * Coverlap;
    cap += (double)(stack - 1) * (Leff * width * Cdiffarea +
                                  4.0 * Leff * Cdiffside + 2.0 * width * Coverlap);
  } else {
    cap = 3.0 * Leff * width * Cdiffarea + (6.0 * Leff + width) * Cdiffside +
          width * Coverlap;
    cap += (double)(stack - 1) * (Leff * width * Cdiffarea +
                                  2.0 * Leff * Cdiffside + 2.0 * width * Coverlap);
  }

  return cap;
}

/*----------------------------------------------------------------------*/

/* The following routines estimate the effective resistance of an
   on transistor as described in the tech report.  The first routine
   gives the "switching" resistance, and the second gives the
   "full-on" resistance */

/* returns resistance in ohms */
/* width in um */
/* whether n or p-channel (boolean) */
/* number of transistors in series */
double transresswitch(double width, bool nchannel, int32_t stack) {
  double restrans;
  restrans = (nchannel) ? (Rnchannelstatic) : (Rpchannelstatic);

  /* calculate resistance of stack - assume all but switching trans
     have 0.8X the resistance since they are on throughout switching */
  return (1.0 + ((stack - 1.0) * 0.8)) * restrans / width;
}


/* returns resistance in ohms */
/* width in um */
/* whether n or p-channel (boolean) */
/* number of transistors in series */
double transreson(double width, bool nchannel, int32_t stack) {
  double restrans;
  restrans = (nchannel) ? Rnchannelon : Rpchannelon;

  /* calculate resistance of stack.  Unlike transres, we don't
  	multiply the stacked transistors by 0.8 */
  return stack * restrans / width ;
}

/* This routine operates in reverse: given a resistance, it finds
 * the transistor width that would have this R.  It is used in the
 * data wordline to estimate the wordline driver size. */
/* returns width in um */
/* resistance in ohms */
/* whether N-channel or P-channel */
double restowidth(double res, bool nchannel) {
  double restrans;

  restrans = (nchannel) ? Rnchannelon : Rpchannelon;
  return(restrans / res);
}

/* input rise time */
/* time constant of gate */
/* threshold voltages */
/* whether INPUT rise or fall (boolean) */
double horowitz(double inputramptime, double tf, double vs1, double vs2, int32_t rise) {
  double a, b, td;

  a = inputramptime / tf;
  if (rise == RISE) {
    b = 0.5;
    td = tf * sqrt( log(vs1) * log(vs1) + 2 * a * b * (1.0 - vs1)) + tf * (log(vs1) - log(vs2));
  } else {
    b = 0.4;
    td = tf * sqrt( log(1.0 - vs1) * log(1.0 - vs1) + 2 * a * b * (vs1)) + tf * (log(1.0 - vs1) - log(1.0 - vs2));
  }
  return td;
}


/*
 * This part of the code contains routines for each section as
 * described in the tech report.  See the tech report for more details
 * and explanations */

/*----------------------------------------------------------------------*/

/* Decoder delay:  (see section 6.1 of tech report) */


double decoder_delay(int32_t C, int32_t B, int32_t A, int32_t Ndwl, int32_t Ndbl, int32_t Nspd, int32_t Ntwl, int32_t Ntbl, int32_t Ntspd,
                     double * Tdecdrive, double * Tdecoder1, double * Tdecoder2, double * outrisetime) {
  double Ceq, Req, Rwire, rows, tf, nextinputtime, vth;
  int32_t numstack;

  /* Calculate rise time.  Consider two inverters */

  Ceq = draincap(Wdecdrivep, PCH, 1) + draincap(Wdecdriven, NCH, 1) +
        gatecap(Wdecdrivep + Wdecdriven, 0.0);
  tf = Ceq * transreson(Wdecdriven, NCH, 1);
  nextinputtime = horowitz(0.0, tf, VTHINV100x60, VTHINV100x60, FALL) /
                  (VTHINV100x60);

  Ceq = draincap(Wdecdrivep, PCH, 1) + draincap(Wdecdriven, NCH, 1) +
        gatecap(Wdecdrivep + Wdecdriven, 0.0);
  tf = Ceq * transreson(Wdecdriven, NCH, 1);
  nextinputtime = horowitz(nextinputtime, tf, VTHINV100x60, VTHINV100x60,
                           RISE) /
                  (1.0 - VTHINV100x60);

  /* First stage: driving the decoders */

  rows = C / (8 * B * A * Ndbl * Nspd);
  Ceq = draincap(Wdecdrivep, PCH, 1) + draincap(Wdecdriven, NCH, 1) +
        4 * gatecap(Wdec3to8n + Wdec3to8p, 10.0) * (Ndwl * Ndbl) +
        Cwordmetal * 0.25 * 8 * B * A * Ndbl * Nspd;
  Rwire = Rwordmetal * 0.125 * 8 * B * A * Ndbl * Nspd;
  tf = (Rwire + transreson(Wdecdrivep, PCH, 1)) * Ceq;
  *Tdecdrive = horowitz(nextinputtime, tf, VTHINV100x60, VTHNAND60x90,
                        FALL);
  nextinputtime = *Tdecdrive / VTHNAND60x90;

  /* second stage: driving a bunch of nor gates with a nand */

  numstack = static_cast<int>(ceil((1.0 / 3.0) * logtwo( (double)((double)C / (double)(B * A * Ndbl * Nspd)))));
  if (numstack == 0) numstack = 1;
  if (numstack > 5) numstack = 5;
  Ceq = 3 * draincap(Wdec3to8p, PCH, 1) + draincap(Wdec3to8n, NCH, 3) +
        gatecap(WdecNORn + WdecNORp, ((numstack * 40) + 20.0)) * rows +
        Cbitmetal * rows * 8;

  Rwire = Rbitmetal * rows * 8 / 2;
  tf = Ceq * (Rwire + transreson(Wdec3to8n, NCH, 3));

  /* we only want to charge the output to the threshold of the
     nor gate.  But the threshold depends on the number of inputs
     to the nor.  */

  vth = 0;
  assert(numstack >= 1 && numstack <= 5);
  switch (numstack) {
    case 1:
      vth = VTHNOR12x4x1;
      break;
    case 2:
      vth = VTHNOR12x4x2;
      break;
    case 3:
      vth = VTHNOR12x4x3;
      break;
    case 4:
      vth = VTHNOR12x4x4;
      break;
    case 5:
      vth = VTHNOR12x4x4;
      break;
  }
  *Tdecoder1 = horowitz(nextinputtime, tf, VTHNAND60x90, vth, RISE);
  nextinputtime = *Tdecoder1 / (1.0 - vth);

  /* Final stage: driving an inverter with the nor */

  Req = transreson(WdecNORp, PCH, numstack);
  Ceq = (gatecap(Wdecinvn + Wdecinvp, 20.0) +
         numstack * draincap(WdecNORn, NCH, 1) +
         draincap(WdecNORp, PCH, numstack));
  tf = Req * Ceq;
  *Tdecoder2 = horowitz(nextinputtime, tf, vth, VSINV, FALL);
  *outrisetime = *Tdecoder2 / (VSINV);
  return(*Tdecdrive + *Tdecoder1 + *Tdecoder2);
}


/*----------------------------------------------------------------------*/

/* Decoder delay in the tag array (see section 6.1 of tech report) */


double decoder_tag_delay(int32_t C, int32_t B, int32_t A, int32_t Ndwl, int32_t Ndbl, int32_t Nspd, int32_t Ntwl, int32_t Ntbl, int32_t Ntspd,
                         double * Tdecdrive, double * Tdecoder1, double * Tdecoder2, double * outrisetime) {
  double Ceq, Req, Rwire, rows, tf, nextinputtime, vth;
  int32_t numstack;


  /* Calculate rise time.  Consider two inverters */

  Ceq = draincap(Wdecdrivep, PCH, 1) + draincap(Wdecdriven, NCH, 1) +
        gatecap(Wdecdrivep + Wdecdriven, 0.0);
  tf = Ceq * transreson(Wdecdriven, NCH, 1);
  nextinputtime = horowitz(0.0, tf, VTHINV100x60, VTHINV100x60, FALL) /
                  (VTHINV100x60);

  Ceq = draincap(Wdecdrivep, PCH, 1) + draincap(Wdecdriven, NCH, 1) +
        gatecap(Wdecdrivep + Wdecdriven, 0.0);
  tf = Ceq * transreson(Wdecdriven, NCH, 1);
  nextinputtime = horowitz(nextinputtime, tf, VTHINV100x60, VTHINV100x60,
                           RISE) /
                  (1.0 - VTHINV100x60);

  /* First stage: driving the decoders */

  rows = C / (8 * B * A * Ntbl * Ntspd);
  Ceq = draincap(Wdecdrivep, PCH, 1) + draincap(Wdecdriven, NCH, 1) +
        4 * gatecap(Wdec3to8n + Wdec3to8p, 10.0) * (Ntwl * Ntbl) +
        Cwordmetal * 0.25 * 8 * B * A * Ntbl * Ntspd;
  Rwire = Rwordmetal * 0.125 * 8 * B * A * Ntbl * Ntspd;
  tf = (Rwire + transreson(Wdecdrivep, PCH, 1)) * Ceq;
  *Tdecdrive = horowitz(nextinputtime, tf, VTHINV100x60, VTHNAND60x90,
                        FALL);
  nextinputtime = *Tdecdrive / VTHNAND60x90;

  /* second stage: driving a bunch of nor gates with a nand */

  numstack = static_cast<int>(ceil((1.0 / 3.0) * logtwo( (double)((double)C / (double)(B * A * Ntbl * Ntspd)))));
  if (numstack == 0) numstack = 1;
  if (numstack > 5) numstack = 5;

  Ceq = 3 * draincap(Wdec3to8p, PCH, 1) + draincap(Wdec3to8n, NCH, 3) +
        gatecap(WdecNORn + WdecNORp, ((numstack * 40) + 20.0)) * rows +
        Cbitmetal * rows * 8;

  Rwire = Rbitmetal * rows * 8 / 2;
  tf = Ceq * (Rwire + transreson(Wdec3to8n, NCH, 3));

  /* we only want to charge the output to the threshold of the
     nor gate.  But the threshold depends on the number of inputs
     to the nor.  */

  vth = 0;
  assert(numstack >= 1 && numstack <= 6);
  switch (numstack) {
    case 1:
      vth = VTHNOR12x4x1;
      break;
    case 2:
      vth = VTHNOR12x4x2;
      break;
    case 3:
      vth = VTHNOR12x4x3;
      break;
    case 4:
      vth = VTHNOR12x4x4;
      break;
    case 5:
      vth = VTHNOR12x4x4;
      break;
    case 6:
      vth = VTHNOR12x4x4;
      break;
  }
  *Tdecoder1 = horowitz(nextinputtime, tf, VTHNAND60x90, vth, RISE);
  nextinputtime = *Tdecoder1 / (1.0 - vth);

  /* Final stage: driving an inverter with the nor */

  Req = transreson(WdecNORp, PCH, numstack);
  Ceq = (gatecap(Wdecinvn + Wdecinvp, 20.0) +
         numstack * draincap(WdecNORn, NCH, 1) +
         draincap(WdecNORp, PCH, numstack));
  tf = Req * Ceq;
  *Tdecoder2 = horowitz(nextinputtime, tf, vth, VSINV, FALL);
  *outrisetime = *Tdecoder2 / (VSINV);
  return(*Tdecdrive + *Tdecoder1 + *Tdecoder2);
}


/*----------------------------------------------------------------------*/

/* Data array wordline delay (see section 6.2 of tech report) */


double wordline_delay(int32_t B, int32_t A, int32_t Ndwl, int32_t Nspd, double inrisetime, double * outrisetime) {
  double Rpdrive;
  double desiredrisetime, psize, nsize;
  double tf, nextinputtime, Ceq, Rline, Cline;
  int32_t cols;
  double Tworddrivedel, Twordchargedel;

  cols = 8 * B * A * Nspd / Ndwl;

  /* Choose a transistor size that makes sense */
  /* Use a first-order approx */

  desiredrisetime = krise * log((double)(cols)) / 2.0;
  Cline = (gatecappass(Wmemcella, 0.0) +
           gatecappass(Wmemcella, 0.0) +
           Cwordmetal) * cols;
  Rpdrive = desiredrisetime / (Cline * log(VSINV) * -1.0);
  psize = restowidth(Rpdrive, PCH);
  if (psize > Wworddrivemax) {
    psize = Wworddrivemax;
  }

  /* Now that we have a reasonable psize, do the rest as before */
  /* If we keep the ratio the same as the tag wordline driver,
     the threshold voltage will be close to VSINV */

  nsize = psize * Wdecinvn / Wdecinvp;

  Ceq = draincap(Wdecinvn, NCH, 1) + draincap(Wdecinvp, PCH, 1) +
        gatecap(nsize + psize, 20.0);
  tf = transreson(Wdecinvn, NCH, 1) * Ceq;

  Tworddrivedel = horowitz(inrisetime, tf, VSINV, VSINV, RISE);
  nextinputtime = Tworddrivedel / (1.0 - VSINV);

  Cline = (gatecappass(Wmemcella, (BitWidth - 2 * Wmemcella) / 2.0) +
           gatecappass(Wmemcella, (BitWidth - 2 * Wmemcella) / 2.0) +
           Cwordmetal) * cols +
          draincap(nsize, NCH, 1) + draincap(psize, PCH, 1);
  Rline = Rwordmetal * cols / 2;
  tf = (transreson(psize, PCH, 1) + Rline) * Cline;
  Twordchargedel = horowitz(nextinputtime, tf, VSINV, VSINV, FALL);
  *outrisetime = Twordchargedel / VSINV;

  return(Tworddrivedel + Twordchargedel);
}

/*----------------------------------------------------------------------*/

/* Tag array wordline delay (see section 6.3 of tech report) */


double wordline_tag_delay(int32_t C, int32_t A, int32_t Ntspd, int32_t Ntwl, double inrisetime, double * outrisetime) {
  double tf;
  double Cline, Rline, Ceq, nextinputtime;
  int32_t tagbits;
  double Tworddrivedel, Twordchargedel;

  /* number of tag bits */

  tagbits = ADDRESS_BITS + 2 - (int)logtwo((double)C) + (int)logtwo((double)A);

  /* first stage */

  Ceq = draincap(Wdecinvn, NCH, 1) + draincap(Wdecinvp, PCH, 1) +
        gatecap(Wdecinvn + Wdecinvp, 20.0);
  tf = transreson(Wdecinvn, NCH, 1) * Ceq;

  Tworddrivedel = horowitz(inrisetime, tf, VSINV, VSINV, RISE);
  nextinputtime = Tworddrivedel / (1.0 - VSINV);

  /* second stage */
  Cline = (gatecappass(Wmemcella, (BitWidth - 2 * Wmemcella) / 2.0) +
           gatecappass(Wmemcella, (BitWidth - 2 * Wmemcella) / 2.0) +
           Cwordmetal) * tagbits * A * Ntspd / Ntwl +
          draincap(Wdecinvn, NCH, 1) + draincap(Wdecinvp, PCH, 1);
  Rline = Rwordmetal * tagbits * A * Ntspd / (2 * Ntwl);
  tf = (transreson(Wdecinvp, PCH, 1) + Rline) * Cline;
  Twordchargedel = horowitz(nextinputtime, tf, VSINV, VSINV, FALL);
  *outrisetime = Twordchargedel / VSINV;
  return(Tworddrivedel + Twordchargedel);

}

/*----------------------------------------------------------------------*/

/* Data array bitline: (see section 6.4 in tech report) */


double bitline_delay(int32_t C, int32_t A, int32_t B, int32_t Ndwl, int32_t Ndbl, int32_t Nspd, double inrisetime, double * outrisetime, double baseVdd) {
  double Tbit, Cline, Ccolmux, Rlineb, r1, r2, c1, c2, a, b, c;
  double m, tstep;
  double Cbitrow;    /* bitline capacitance due to access transistor */
  int32_t rows, cols;

  Cbitrow = draincap(Wmemcella, NCH, 1) / 2.0; /* due to shared contact */
  rows = C / (B * A * Ndbl * Nspd);
  cols = 8 * B * A * Nspd / Ndwl;
  if (Ndbl * Nspd == 1) {
    Cline = rows * (Cbitrow + Cbitmetal) + 2 * draincap(Wbitpreequ, PCH, 1);
    Ccolmux = 2 * gatecap(WsenseQ1to4, 10.0);
    Rlineb = Rbitmetal * rows / 2.0;
    r1 = Rlineb;
  } else {
    Cline = rows * (Cbitrow + Cbitmetal) + 2 * draincap(Wbitpreequ, PCH, 1) +
            draincap(Wbitmuxn, NCH, 1);
    Ccolmux = Nspd * Ndbl * (draincap(Wbitmuxn, NCH, 1)) + 2 * gatecap(WsenseQ1to4, 10.0);
    Rlineb = Rbitmetal * rows / 2.0;
    r1 = Rlineb +
         transreson(Wbitmuxn, NCH, 1);
  }
  r2 = transreson(Wmemcella, NCH, 1) +
       transreson(Wmemcella * Wmemcellbscale, NCH, 1);
  c1 = Ccolmux;
  c2 = Cline;


  tstep = (r2 * c2 + (r1 + r2) * c1) * log((Vbitpre) / (Vbitpre - Vbitsense));

  /* take input rise time into account */

  m = baseVdd / inrisetime;
  if (tstep <= (0.5 * (baseVdd - Vt) / m)) {
    a = m;
    b = 2 * ((baseVdd * 0.5) - Vt);
    c = -2 * tstep * (baseVdd - Vt) + 1 / m * ((baseVdd * 0.5) - Vt) *
        ((baseVdd * 0.5) - Vt);
    Tbit = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
  } else {
    Tbit = tstep + (baseVdd + Vt) / (2 * m) - (baseVdd * 0.5) / m;
  }

  *outrisetime = Tbit / (log((Vbitpre - Vbitsense) / baseVdd));
  return(Tbit);
}




/*----------------------------------------------------------------------*/

/* Tag array bitline: (see section 6.4 in tech report) */


double bitline_tag_delay(int32_t C, int32_t A, int32_t B, int32_t Ntwl, int32_t Ntbl, int32_t Ntspd, double inrisetime, double * outrisetime, double baseVdd) {
  double Tbit, Cline, Ccolmux, Rlineb, r1, r2, c1, c2, a, b, c;
  double m, tstep;
  double Cbitrow;    /* bitline capacitance due to access transistor */
  int32_t rows, cols;

  Cbitrow = draincap(Wmemcella, NCH, 1) / 2.0; /* due to shared contact */
  rows = C / (B * A * Ntbl * Ntspd);
  cols = 8 * B * A * Ntspd / Ntwl;
  if (Ntbl * Ntspd == 1) {
    Cline = rows * (Cbitrow + Cbitmetal) + 2 * draincap(Wbitpreequ, PCH, 1);
    Ccolmux = 2 * gatecap(WsenseQ1to4, 10.0);
    Rlineb = Rbitmetal * rows / 2.0;
    r1 = Rlineb;
  } else {
    Cline = rows * (Cbitrow + Cbitmetal) + 2 * draincap(Wbitpreequ, PCH, 1) +
            draincap(Wbitmuxn, NCH, 1);
    Ccolmux = Ntspd * Ntbl * (draincap(Wbitmuxn, NCH, 1)) + 2 * gatecap(WsenseQ1to4, 10.0);
    Rlineb = Rbitmetal * rows / 2.0;
    r1 = Rlineb +
         transreson(Wbitmuxn, NCH, 1);
  }
  r2 = transreson(Wmemcella, NCH, 1) +
       transreson(Wmemcella * Wmemcellbscale, NCH, 1);

  c1 = Ccolmux;
  c2 = Cline;

  tstep = (r2 * c2 + (r1 + r2) * c1) * log((Vbitpre) / (Vbitpre - Vbitsense));

  /* take into account input rise time */

  m = baseVdd / inrisetime;
  if (tstep <= (0.5 * (baseVdd - Vt) / m)) {
    a = m;
    b = 2 * ((baseVdd * 0.5) - Vt);
    c = -2 * tstep * (baseVdd - Vt) + 1 / m * ((baseVdd * 0.5) - Vt) *
        ((baseVdd * 0.5) - Vt);
    Tbit = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
  } else {
    Tbit = tstep + (baseVdd + Vt) / (2 * m) - (baseVdd * 0.5) / m;
  }

  *outrisetime = Tbit / (log((Vbitpre - Vbitsense) / baseVdd));
  return(Tbit);
}



/* It is assumed the sense amps have a constant delay
   (see section 6.5) */

double sense_amp_delay(double inrisetime, double * outrisetime) {
  *outrisetime = tfalldata;
  return(tsensedata);
}

double sense_amp_tag_delay(double inrisetime, double * outrisetime) {
  *outrisetime = tfalltag;
  return(tsensetag);
}

/*----------------------------------------------------------------------*/

/* Comparator Delay (see section 6.6) */


double compare_time(int32_t C, int32_t A, int32_t Ntbl, int32_t Ntspd, double inputtime, double * outputtime, double baseVdd) {
  double Req, Ceq, tf, st1del, st2del, st3del, nextinputtime, m;
  double c1, c2, r1, r2, tstep, a, b, c;
  double Tcomparatorni;
  int32_t cols, tagbits;

  /* First Inverter */

  Ceq = gatecap(Wcompinvn2 + Wcompinvp2, 10.0) +
        draincap(Wcompinvp1, PCH, 1) + draincap(Wcompinvn1, NCH, 1);
  Req = transreson(Wcompinvp1, PCH, 1);
  tf = Req * Ceq;
  st1del = horowitz(inputtime, tf, VTHCOMPINV, VTHCOMPINV, FALL);
  nextinputtime = st1del / VTHCOMPINV;

  /* Second Inverter */

  Ceq = gatecap(Wcompinvn3 + Wcompinvp3, 10.0) +
        draincap(Wcompinvp2, PCH, 1) + draincap(Wcompinvn2, NCH, 1);
  Req = transreson(Wcompinvn2, NCH, 1);
  tf = Req * Ceq;
  st2del = horowitz(inputtime, tf, VTHCOMPINV, VTHCOMPINV, RISE);
  nextinputtime = st1del / (1.0 - VTHCOMPINV);

  /* Third Inverter */

  Ceq = gatecap(Wevalinvn + Wevalinvp, 10.0) +
        draincap(Wcompinvp3, PCH, 1) + draincap(Wcompinvn3, NCH, 1);
  Req = transreson(Wcompinvp3, PCH, 1);
  tf = Req * Ceq;
  st3del = horowitz(nextinputtime, tf, VTHCOMPINV, VTHEVALINV, FALL);
  nextinputtime = st1del / (VTHEVALINV);

  /* Final Inverter (virtual ground driver) discharging compare part */

  tagbits = ADDRESS_BITS - (int)logtwo((double)C) + (int)logtwo((double)A);
  cols = tagbits * Ntbl * Ntspd;

  r1 = transreson(Wcompn, NCH, 2);
  r2 = transresswitch(Wevalinvn, NCH, 1);
  c2 = (tagbits) * (draincap(Wcompn, NCH, 1) + draincap(Wcompn, NCH, 2)) +
       draincap(Wevalinvp, PCH, 1) + draincap(Wevalinvn, NCH, 1);
  c1 = (tagbits) * (draincap(Wcompn, NCH, 1) + draincap(Wcompn, NCH, 2))
       + draincap(Wcompp, PCH, 1) + gatecap(Wmuxdrv12n + Wmuxdrv12p, 20.0) +
       cols * Cwordmetal;

  /* time to go to threshold of mux driver */

  tstep = (r2 * c2 + (r1 + r2) * c1) * log(1.0 / VTHMUXDRV1);

  /* take into account non-zero input rise time */

  m = baseVdd / nextinputtime;

  if ((tstep) <= (0.5 * (baseVdd - Vt) / m)) {
    a = m;
    b = 2 * ((baseVdd * VTHEVALINV) - Vt);
    c = -2 * (tstep) * (baseVdd - Vt) + 1 / m * ((baseVdd * VTHEVALINV) - Vt) * ((baseVdd * VTHEVALINV) - Vt);
    Tcomparatorni = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
  } else {
    Tcomparatorni = (tstep) + (baseVdd + Vt) / (2 * m) - (baseVdd * VTHEVALINV) / m;
  }
  *outputtime = Tcomparatorni / (1.0 - VTHMUXDRV1);

  return(Tcomparatorni + st1del + st2del + st3del);
}




/*----------------------------------------------------------------------*/

/* Delay of the multiplexor Driver (see section 6.7) */


double mux_driver_delay(int32_t C, int32_t B, int32_t A, int32_t Ndbl, int32_t Nspd, int32_t Ndwl, int32_t Ntbl, int32_t Ntspd, double inputtime, double * outputtime) {
  double Ceq, Req, tf, nextinputtime;
  double Tst1, Tst2, Tst3;

  /* first driver stage - Inverte "match" to produce "matchb" */
  /* the critical path is the DESELECTED case, so consider what
     happens when the address bit is true, but match goes low */

  Ceq = gatecap(WmuxdrvNORn + WmuxdrvNORp, 15.0) * (8 * B / BITOUT) +
        draincap(Wmuxdrv12n, NCH, 1) + draincap(Wmuxdrv12p, PCH, 1);
  Req = transreson(Wmuxdrv12p, PCH, 1);
  tf = Ceq * Req;
  Tst1 = horowitz(inputtime, tf, VTHMUXDRV1, VTHMUXDRV2, FALL);
  nextinputtime = Tst1 / VTHMUXDRV2;

  /* second driver stage - NOR "matchb" with address bits to produce sel */

  Ceq = gatecap(Wmuxdrv3n + Wmuxdrv3p, 15.0) + 2 * draincap(WmuxdrvNORn, NCH, 1) +
        draincap(WmuxdrvNORp, PCH, 2);
  Req = transreson(WmuxdrvNORn, NCH, 1);
  tf = Ceq * Req;
  Tst2 = horowitz(nextinputtime, tf, VTHMUXDRV2, VTHMUXDRV3, RISE);
  nextinputtime = Tst2 / (1 - VTHMUXDRV3);

  /* third driver stage - invert "select" to produce "select bar" */

  Ceq = BITOUT * gatecap(Woutdrvseln + Woutdrvselp + Woutdrvnorn + Woutdrvnorp, 20.0) +
        draincap(Wmuxdrv3p, PCH, 1) + draincap(Wmuxdrv3n, NCH, 1) +
        Cwordmetal * 8 * B * A * Nspd * Ndbl / 2.0;
  Req = (Rwordmetal * 8 * B * A * Nspd * Ndbl / 2) / 2 + transreson(Wmuxdrv3p, PCH, 1);
  tf = Ceq * Req;
  Tst3 = horowitz(nextinputtime, tf, VTHMUXDRV3, VTHOUTDRINV, FALL);
  *outputtime = Tst3 / (VTHOUTDRINV);

  return(Tst1 + Tst2 + Tst3);

}



/*----------------------------------------------------------------------*/

/* Valid driver (see section 6.9 of tech report)
   Note that this will only be called for a direct mapped cache */

double valid_driver_delay(int32_t C, int32_t A, int32_t Ntbl, int32_t Ntspd, double inputtime) {
  double Ceq, Tst1, tf;

  Ceq = draincap(Wmuxdrv12n, NCH, 1) + draincap(Wmuxdrv12p, PCH, 1) + Cout;
  tf = Ceq * transreson(Wmuxdrv12p, PCH, 1);
  Tst1 = horowitz(inputtime, tf, VTHMUXDRV1, 0.5, FALL);

  return(Tst1);
}


/*----------------------------------------------------------------------*/

/* Data output delay (data side) -- see section 6.8
   This is the time through the NAND/NOR gate and the final inverter
   assuming sel is already present */

double dataoutput_delay(int32_t C, int32_t B, int32_t A, int32_t Ndbl, int32_t Nspd, int32_t Ndwl, double inrisetime, double * outrisetime) {
  double Ceq, Rwire;
  double aspectRatio;     /* as height over width */
  double ramBlocks;       /* number of RAM blocks */
  double tf;
  double nordel, outdel, nextinputtime;
  double hstack, vstack;

  /* calculate some layout info */

  aspectRatio = (2.0 * C) / (8.0 * B * B * A * A * Ndbl * Ndbl * Nspd * Nspd);
  hstack = (aspectRatio > 1.0) ? aspectRatio : 1.0 / aspectRatio;
  ramBlocks = Ndwl * Ndbl;
  hstack = hstack * sqrt(ramBlocks / hstack);
  vstack = ramBlocks / hstack;

  /* Delay of NOR gate */

  Ceq = 2 * draincap(Woutdrvnorn, NCH, 1) + draincap(Woutdrvnorp, PCH, 2) +
        gatecap(Woutdrivern, 10.0);
  tf = Ceq * transreson(Woutdrvnorp, PCH, 2);
  nordel = horowitz(inrisetime, tf, VTHOUTDRNOR, VTHOUTDRIVE, FALL);
  nextinputtime = nordel / (VTHOUTDRIVE);

  /* Delay of final output driver */

  Ceq = (draincap(Woutdrivern, NCH, 1) + draincap(Woutdriverp, PCH, 1)) *
        ((8 * B * A) / BITOUT) +
        Cwordmetal * (8 * B * A * Nspd * (vstack)) + Cout;
  Rwire = Rwordmetal * (8 * B * A * Nspd * (vstack)) / 2;

  tf = Ceq * (transreson(Woutdriverp, PCH, 1) + Rwire);
  outdel = horowitz(nextinputtime, tf, VTHOUTDRIVE, 0.5, RISE);
  *outrisetime = outdel / 0.5;
  return(outdel + nordel);
}

/*----------------------------------------------------------------------*/

/* Sel inverter delay (part of the output driver)  see section 6.8 */

double selb_delay_tag_path(double inrisetime, double * outrisetime) {
  double Ceq, Tst1, tf;

  Ceq = draincap(Woutdrvseln, NCH, 1) + draincap(Woutdrvselp, PCH, 1) +
        gatecap(Woutdrvnandn + Woutdrvnandp, 10.0);
  tf = Ceq * transreson(Woutdrvseln, NCH, 1);
  Tst1 = horowitz(inrisetime, tf, VTHOUTDRINV, VTHOUTDRNAND, RISE);
  *outrisetime = Tst1 / (1.0 - VTHOUTDRNAND);

  return(Tst1);
}


/*----------------------------------------------------------------------*/

/* This routine calculates the extra time required after an access before
 * the next access can occur [ie.  it returns (cycle time-access time)].
 */

double precharge_delay(double worddata) {
  double Ceq, tf, pretime;

  /* as discussed in the tech report, the delay is the delay of
     4 inverter delays (each with fanout of 4) plus the delay of
     the wordline */

  Ceq = draincap(Wdecinvn, NCH, 1) + draincap(Wdecinvp, PCH, 1) +
        4 * gatecap(Wdecinvn + Wdecinvp, 0.0);
  tf = Ceq * transreson(Wdecinvn, NCH, 1);
  pretime = 4 * horowitz(0.0, tf, 0.5, 0.5, RISE) + worddata;

  return(pretime);
}



/*======================================================================*/


/* returns TRUE if the parameters make up a valid organization */
/* Layout concerns drive any restrictions you might add here */

int32_t organizational_parameters_valid(int32_t rows, int32_t cols, int32_t Ndwl, int32_t Ndbl, int32_t Nspd, int32_t Ntwl, int32_t Ntbl, int32_t Ntspd) {
  /* don't want more than 8 subarrays for each of data/tag */

  if (Ndwl * Ndbl > MAXSUBARRAYS) return(false);
  if (Ntwl * Ntbl > MAXSUBARRAYS) return(false);

  /* add more constraints here as necessary */

  return(true);
}


/*----------------------------------------------------------------------*/


void calculate_time(cacti_result_type * result, cacti_parameter_type * parameters, double baseVdd) {
  int32_t Ndwl, Ndbl, Nspd, Ntwl, Ntbl, Ntspd, rows, columns;
  double access_time;
  double before_mux, after_mux;
  double decoder_data_driver, decoder_data_3to8, decoder_data_inv;
  double decoder_data, decoder_tag, wordline_data, wordline_tag;
  double decoder_tag_driver, decoder_tag_3to8, decoder_tag_inv;
  double bitline_data, bitline_tag, sense_amp_data, sense_amp_tag;
  double compare_tag, mux_driver, data_output, selb;
  double time_till_compare, time_till_select, valid_driver;
  double cycle_time, precharge_del;
  double outrisetime, inrisetime;

  rows = parameters->number_of_sets;
  columns = 8 * parameters->block_size * parameters->associativity;

  /* go through possible Ndbl,Ndwl and find the smallest */
  /* Because of area considerations, I don't think it makes sense
     to break either dimension up larger than MAXN */

  result->cycle_time = BIGNUM;
  result->access_time = BIGNUM;
  for (Nspd = 1; Nspd <= MAXSPD; Nspd = Nspd * 2) {
    for (Ndwl = 1; Ndwl <= MAXN; Ndwl = Ndwl * 2) {
      for (Ndbl = 1; Ndbl <= MAXN; Ndbl = Ndbl * 2) {
        for (Ntspd = 1; Ntspd <= MAXSPD; Ntspd = Ntspd * 2) {
          for (Ntwl = 1; Ntwl <= 1; Ntwl = Ntwl * 2) {
            for (Ntbl = 1; Ntbl <= MAXN; Ntbl = Ntbl * 2) {

              if (organizational_parameters_valid
                  (rows, columns, Ndwl, Ndbl, Nspd, Ntwl, Ntbl, Ntspd)) {


                /* Calculate data side of cache */


                decoder_data = decoder_delay(parameters->cache_size, parameters->block_size,
                                             parameters->associativity, Ndwl, Ndbl, Nspd, Ntwl, Ntbl, Ntspd,
                                             &decoder_data_driver, &decoder_data_3to8,
                                             &decoder_data_inv, &outrisetime);

                inrisetime = outrisetime;
                wordline_data = wordline_delay(parameters->block_size,
                                               parameters->associativity, Ndwl, Nspd,
                                               inrisetime, &outrisetime);
                inrisetime = outrisetime;
                bitline_data = bitline_delay(parameters->cache_size, parameters->associativity,
                                             parameters->block_size, Ndwl, Ndbl, Nspd,
                                             inrisetime, &outrisetime, baseVdd);
                inrisetime = outrisetime;
                sense_amp_data = sense_amp_delay(inrisetime, &outrisetime);
                inrisetime = outrisetime;
                data_output = dataoutput_delay(parameters->cache_size, parameters->block_size,
                                               parameters->associativity, Ndbl, Nspd, Ndwl,
                                               inrisetime, &outrisetime);
                inrisetime = outrisetime;


                /* if the associativity is 1, the data output can come right
                   after the sense amp.   Otherwise, it has to wait until
                   the data access has been done. */

                if (parameters->associativity == 1) {
                  before_mux = decoder_data + wordline_data + bitline_data +
                               sense_amp_data + data_output;
                  after_mux = 0;
                } else {
                  before_mux = decoder_data + wordline_data + bitline_data +
                               sense_amp_data;
                  after_mux = data_output;
                }


                /*
                 * Now worry about the tag side.
                 */


                decoder_tag = decoder_tag_delay(parameters->cache_size,
                                                parameters->block_size, parameters->associativity,
                                                Ndwl, Ndbl, Nspd, Ntwl, Ntbl, Ntspd,
                                                &decoder_tag_driver, &decoder_tag_3to8,
                                                &decoder_tag_inv, &outrisetime);
                inrisetime = outrisetime;

                wordline_tag = wordline_tag_delay(parameters->cache_size,
                                                  parameters->associativity, Ntspd, Ntwl,
                                                  inrisetime, &outrisetime);
                inrisetime = outrisetime;

                bitline_tag = bitline_tag_delay(parameters->cache_size, parameters->associativity,
                                                parameters->block_size, Ntwl, Ntbl, Ntspd,
                                                inrisetime, &outrisetime, baseVdd);
                inrisetime = outrisetime;

                sense_amp_tag = sense_amp_tag_delay(inrisetime, &outrisetime);
                inrisetime = outrisetime;

                compare_tag = compare_time(parameters->cache_size, parameters->associativity,
                                           Ntbl, Ntspd,
                                           inrisetime, &outrisetime, baseVdd);
                inrisetime = outrisetime;

                if (parameters->associativity == 1) {
                  mux_driver = 0;
                  valid_driver = valid_driver_delay(parameters->cache_size,
                                                    parameters->associativity, Ntbl, Ntspd, inrisetime);
                  time_till_compare = decoder_tag + wordline_tag + bitline_tag +
                                      sense_amp_tag;

                  time_till_select = time_till_compare + compare_tag + valid_driver;


                  /*
                  		* From the above info, calculate the total access time
                  		*/

                  access_time = MAX(before_mux + after_mux, time_till_select);

                  selb = 0;
                } else {
                  mux_driver = mux_driver_delay(parameters->cache_size, parameters->block_size,
                                                parameters->associativity, Ndbl, Nspd, Ndwl, Ntbl, Ntspd,
                                                inrisetime, &outrisetime);

                  selb = selb_delay_tag_path(inrisetime, &outrisetime);

                  valid_driver = 0;

                  time_till_compare = decoder_tag + wordline_tag + bitline_tag +
                                      sense_amp_tag;

                  time_till_select = time_till_compare + compare_tag + mux_driver
                                     + selb;

                  access_time = MAX(before_mux, time_till_select) + after_mux;
                }

                /*
                 * Calcuate the cycle time
                 */

                precharge_del = precharge_delay(wordline_data);

                cycle_time = access_time + precharge_del;

                /*
                 * The parameters are for a 0.8um process.  A quick way to
                       * scale the results to another process is to divide all
                       * the results by FUDGEFACTOR.  Normally, FUDGEFACTOR is 1.
                 */

                if (result->cycle_time + 1e-11 * (result->best_Ndwl + result->best_Ndbl + result->best_Nspd + result->best_Ntwl + result->best_Ntbl + result->best_Ntspd) > cycle_time / FUDGEFACTOR + 1e-11 * (Ndwl + Ndbl + Nspd + Ntwl + Ntbl + Ntspd)) {
                  result->cycle_time = cycle_time / FUDGEFACTOR;
                  result->access_time = access_time / FUDGEFACTOR;
                  result->best_Ndwl = Ndwl;
                  result->best_Ndbl = Ndbl;
                  result->best_Nspd = Nspd;
                  result->best_Ntwl = Ntwl;
                  result->best_Ntbl = Ntbl;
                  result->best_Ntspd = Ntspd;
                  result->decoder_delay_data = decoder_data / FUDGEFACTOR;
                  result->decoder_delay_tag = decoder_tag / FUDGEFACTOR;
                  result->dec_tag_driver = decoder_tag_driver / FUDGEFACTOR;
                  result->dec_tag_3to8 = decoder_tag_3to8 / FUDGEFACTOR;
                  result->dec_tag_inv = decoder_tag_inv / FUDGEFACTOR;
                  result->dec_data_driver = decoder_data_driver / FUDGEFACTOR;
                  result->dec_data_3to8 = decoder_data_3to8 / FUDGEFACTOR;
                  result->dec_data_inv = decoder_data_inv / FUDGEFACTOR;
                  result->wordline_delay_data = wordline_data / FUDGEFACTOR;
                  result->wordline_delay_tag = wordline_tag / FUDGEFACTOR;
                  result->bitline_delay_data = bitline_data / FUDGEFACTOR;
                  result->bitline_delay_tag = bitline_tag / FUDGEFACTOR;
                  result->sense_amp_delay_data = sense_amp_data / FUDGEFACTOR;
                  result->sense_amp_delay_tag = sense_amp_tag / FUDGEFACTOR;
                  result->compare_part_delay = compare_tag / FUDGEFACTOR;
                  result->drive_mux_delay = mux_driver / FUDGEFACTOR;
                  result->selb_delay = selb / FUDGEFACTOR;
                  result->drive_valid_delay = valid_driver / FUDGEFACTOR;
                  result->data_output_delay = data_output / FUDGEFACTOR;
                  result->precharge_delay = precharge_del / FUDGEFACTOR;
                }
              }
            }
          }
        }
      }
    }
  }

}

int32_t pow2(int32_t x) {
  return static_cast<int>(pow(2.0, static_cast<double>(x)));
}

double logfour(double x) {
  assert(x > 0);
  return log(x) / log(4.0);
}

/* safer pop count to validate the fast algorithm */
int32_t pop_count_slow(quad_t bits) {
  int32_t count = 0;
  quad_t tmpbits = bits;

  while (tmpbits) {
    if (tmpbits & 1) {
      ++count;
    }
    tmpbits >>= 1;
  }
  return count;
}

/* fast pop count */
int32_t pop_count(quad_t bits) {
#define T uint64_t
#define ONES ((T)(-1))
#define TWO(k) ((T)1 << (k))
#define CYCL(k) (ONES/(1 + (TWO(TWO(k)))))
#define BSUM(x,k) ((x)+=(x) >> TWO(k), (x) &= CYCL(k))

  quad_t x = bits;
  x = (x & CYCL(0)) + ((x >> TWO(0)) & CYCL(0));
  x = (x & CYCL(1)) + ((x >> TWO(1)) & CYCL(1));
  BSUM(x, 2);
  BSUM(x, 3);
  BSUM(x, 4);
  BSUM(x, 5);
  return static_cast<int>(x);
}

double compute_af(counter_t num_pop_count_cycle, counter_t total_pop_count_cycle, int32_t pop_width) {
  double avg_pop_count;
  double af, af_b;

  avg_pop_count = num_pop_count_cycle ? static_cast<double>(total_pop_count_cycle) / static_cast<double>(num_pop_count_cycle) : 0;
  af = avg_pop_count / static_cast<double>(pop_width);

  af_b = 1.0 - af;

  return af_b;
}

int32_t squarify(int32_t rows, int32_t cols) {
  int32_t scale_factor = 1;

  if (rows == cols) {
    return 1;
  }

  while (rows > cols) {
    rows /= 2;
    cols *= 2;

    if (rows / 2 <= cols) {
      return pow2(scale_factor);
    }
    ++scale_factor;
  }

  return 1;
}

double squarify_new(int32_t rows, int32_t cols) {
  double scale_factor = 0.0;

  if (rows == cols) {
    return pow(2.0, scale_factor);
  }

  while (rows > cols) {
    rows /= 2;
    cols *= 2;
    if (rows <= cols) {
      return pow(2.0, scale_factor);
    }
    ++scale_factor;
  }

  while (cols > rows) {
    rows *= 2;
    cols /= 2;
    if (cols <= rows) {
      return pow(2.0, scale_factor);
    }
    --scale_factor;
  }
  return 1;
}

double driver_size(double driving_cap, double desiredrisetime) {
  double nsize, psize;
  double Rpdrive;

  Rpdrive = desiredrisetime / (driving_cap * log(VSINV) * -1.0);
  psize = restowidth(Rpdrive, PCH);
  nsize = restowidth(Rpdrive, NCH);
  if (psize > Wworddrivemax) {
    psize = Wworddrivemax;
  }
  if (psize < 4.0 * LSCALE) {
    psize = 4.0 * LSCALE;
  }

  return psize;
}

double array_decoder_power(int32_t rows, int32_t cols, double predeclength, int32_t rports, int32_t wports, double frq, double vdd) {
  double Ctotal = 0;
  double Ceq = 0;
  int32_t numstack;
  int32_t decode_bits;
  int32_t ports;
  double rowsb;

  /* read and write ports are the same here */
  ports = rports + wports;

  rowsb = static_cast<double>(rows);

  /* number of input bits to be decoded */
  decode_bits = static_cast<int>(ceil((logtwo(rowsb))));

  /* First stage: driving the decoders */

  /* This is the capacitance for driving one bit (and its complement).
   -There are #rowsb 3->8 decoders contributing gatecap.
   - 2.0 factor from 2 identical sets of drivers in parallel */

  Ceq = 2.0 * (draincap(Wdecdrivep, PCH, 1) + draincap(Wdecdriven, NCH, 1)) + gatecap(Wdec3to8n + Wdec3to8p, 10.0) * rowsb;

  /* There are ports * #decode_bits total */
  Ctotal += ports * decode_bits * Ceq;

  /* second stage: driving a bunch of nor gates with a nand
   numstack is the size of the nor gates -- ie. a 7-128 decoder has
   3-input NAND followed by 3-input NOR*/

  numstack = static_cast<int>(ceil((1.0 / 3.0) * logtwo(rows)));

  if (numstack <= 0) {
    numstack = 1;
  } else if (numstack > 5) {
    numstack = 5;
  }

  /* There are #rowsb NOR gates being driven*/
  Ceq = (3.0 * draincap(Wdec3to8p, PCH, 1) + draincap(Wdec3to8n, NCH, 3) + gatecap(WdecNORn + WdecNORp, ((numstack * 40) + 20.0))) * rowsb;

  Ctotal += ports * Ceq;

  /* Final stage: driving an inverter with the nor
   (inverter preceding wordline driver) -- wordline driver is in the next section*/

  Ceq = (gatecap(Wdecinvn + Wdecinvp, 20.0) + numstack * draincap(WdecNORn, NCH, 1) + draincap(WdecNORp, PCH, numstack));

  Ctotal += ports * Ceq;

  /* assume Activity Factor == .3*/
  return 0.3 * Ctotal * Powerfactor;
}

double simple_array_decoder_power(int32_t rows, int32_t cols, int32_t rports, int32_t wports, double frq, double vdd) {
  return(array_decoder_power(rows, cols, 0.0, rports, wports, frq, vdd));
}


double array_wordline_power(int32_t rows, int32_t cols, double wordlinelength, int32_t rports, int32_t wports, double frq, double vdd) {
  double Ctotal = 0;
  double Ceq = 0;
  double Cline = 0;
  double Cliner, Clinew = 0;
  double desiredrisetime, psize, nsize;
  int32_t ports;
  double colsb;

  ports = rports + wports;

  colsb = static_cast<double>(cols);

  /* Calculate size of wordline drivers assuming rise time == Period / 8
  - estimate cap on line
  - compute min resistance to achieve this with RC
  - compute width needed to achieve this resistance */
  desiredrisetime = Period / 16;
  Cline = (gatecappass(Wmemcellr, 1.0)) * colsb + wordlinelength * CM3metal;
  psize = driver_size(Cline, desiredrisetime);

  /* how do we want to do p-n ratioing? -- here we just assume the same ratio
  from an inverter pair*/
  nsize = psize * Wdecinvn / Wdecinvp;

  Ceq = draincap(Wdecinvn, NCH, 1) + draincap(Wdecinvp, PCH, 1) +	gatecap(nsize + psize, 20.0);

  Ctotal += ports * Ceq;

  /* Compute caps of read wordline and write wordlines
  - wordline driver caps, given computed width from above
  - read wordlines have 1 nmos access tx, size ~4
  - write wordlines have 2 nmos access tx, size ~2
  - metal line cap
  */

  Cliner = (gatecappass(Wmemcellr, (BitWidth - 2 * Wmemcellr) / 2.0)) * colsb +
           wordlinelength * CM3metal +
           2.0 * (draincap(nsize, NCH, 1) + draincap(psize, PCH, 1));
  Clinew = (2.0 * gatecappass(Wmemcellw, (BitWidth - 2 * Wmemcellw) / 2.0)) * colsb +
           wordlinelength * CM3metal +
           2.0 * (draincap(nsize, NCH, 1) + draincap(psize, PCH, 1));
  Ctotal += rports * Cliner + wports * Clinew;

  /* AF == 1 assuming a different wordline is charged each cycle, but only
  1 wordline (per port) is actually used */
  return Ctotal * Powerfactor;
}

double simple_array_wordline_power(int32_t rows, int32_t cols, int32_t rports, int32_t wports, double frq, double vdd) {
  double wordlinelength;
  int32_t ports = rports + wports;
  wordlinelength = cols * (RegCellWidth + 2 * ports * BitlineSpacing);
  return array_wordline_power(rows, cols, wordlinelength, rports, wports, frq, vdd);
}

double array_bitline_power(int32_t rows, int32_t cols, double bitlinelength, int32_t rports, int32_t wports, int32_t cache, double frq, double vdd, double & clockCap) {
  double Ctotal = 0;
  double Ccolmux = 0;
  double Cbitrowr = 0;
  double Cbitroww = 0;
  double Cprerow = 0;
  double Cwritebitdrive = 0;
  double Cpregate = 0;
  double Cliner = 0;
  double Clinew = 0;
  int32_t ports;
  double rowsb;
  double colsb;

  double desiredrisetime, Cline, psize, nsize;

  ports = rports + wports;

  rowsb = static_cast<double>(rows);
  colsb = static_cast<double>(cols);

  /* Draincaps of access tx's */

  Cbitrowr = draincap(Wmemcellr, NCH, 1);
  Cbitroww = draincap(Wmemcellw, NCH, 1);

  /* Cprerow -- precharge cap on the bitline
  -simple scheme to estimate size of pre-charge tx's in a similar fashion
  to wordline driver size estimation.
  -FIXME: it would be better to use precharge/keeper pairs, i've omitted this
  from this version because it couldn't autosize as easily.
  */

  desiredrisetime = Period / 8;

  Cline = rowsb * Cbitrowr + CM2metal * bitlinelength;
  psize = driver_size(Cline, desiredrisetime);

  /* compensate for not having an nmos pre-charging */
  psize = psize + psize * Wdecinvn / Wdecinvp;

  Cprerow = draincap(psize, PCH, 1);

  /* Cpregate -- cap due to gatecap of precharge transistors -- tack this
  onto bitline cap, again this could have a keeper */
  Cpregate = 4.0 * gatecap(psize, 10.0);
  //global_clockcap+=rports*cols*2.0*Cpregate;
  clockCap += rports * cols * 2.0 * Cpregate;

  /* Cwritebitdrive -- write bitline drivers are used instead of the precharge
  stuff for write bitlines
  - 2 inverter drivers within each driver pair */
  Cline = rowsb * Cbitroww + CM2metal * bitlinelength;

  psize = driver_size(Cline, desiredrisetime);
  nsize = psize * Wdecinvn / Wdecinvp;

  Cwritebitdrive = 2.0 * (draincap(psize, PCH, 1) + draincap(nsize, NCH, 1));

  /*
  reg files (cache==0)
  => single ended bitlines (1 bitline/col)
  => AFs from pop_count
  caches (cache ==1)
  => double-ended bitlines (2 bitlines/col)
  => AFs = .5 (since one of the two bitlines is always charging/discharging)
  */

  if (!cache) {
    /* compute the total line cap for read/write bitlines */
    Cliner = rowsb * Cbitrowr + CM2metal * bitlinelength + Cprerow;
    Clinew = rowsb * Cbitroww + CM2metal * bitlinelength + Cwritebitdrive;

    /* Bitline inverters at the end of the bitlines (replaced w/ sense amps
    in cache styles) */
    Ccolmux = gatecap(MSCALE * (29.9 + 7.8), 0.0) + gatecap(MSCALE * (47.0 + 12.0), 0.0);
    Ctotal += (1.0 - POPCOUNT_AF) * rports * cols * (Cliner + Ccolmux + 2.0 * Cpregate);
    Ctotal += .3 * wports * cols * (Clinew + Cwritebitdrive);
  } else {
    Cliner = rowsb * Cbitrowr + CM2metal * bitlinelength + Cprerow + draincap(Wbitmuxn, NCH, 1);
    Clinew = rowsb * Cbitroww + CM2metal * bitlinelength + Cwritebitdrive;
    Ccolmux = (draincap(Wbitmuxn, NCH, 1)) + 2.0 * gatecap(WsenseQ1to4, 10.0);
    Ctotal += .5 * rports * 2.0 * cols * (Cliner + Ccolmux + 2.0 * Cpregate);
    Ctotal += .5 * wports * 2.0 * cols * (Clinew + Cwritebitdrive);
  }


  if (!cache) {
    return Ctotal * Powerfactor ;
  } else {
    return Ctotal * SensePowerfactor * .4;
  }
}

double simple_array_bitline_power(int32_t rows, int32_t cols, int32_t rports, int32_t wports, int32_t cache, double frq, double vdd, double & clockCap) {
  double bitlinelength;
  int32_t ports = rports + wports;

  bitlinelength = rows * (RegCellHeight + ports * WordlineSpacing);

  return array_bitline_power(rows, cols, bitlinelength, rports, wports, cache, frq, vdd, clockCap);
}


double senseamp_power(int32_t cols, double vdd) {
  return(static_cast<double>(cols) * vdd / 8 * .5e-3);
}

double compare_cap(int32_t compare_bits) {
  double c1, c2;
  /* bottom part of comparator */
  c2 = (compare_bits) * (draincap(Wcompn, NCH, 1) + draincap(Wcompn, NCH, 2)) +
       draincap(Wevalinvp, PCH, 1) + draincap(Wevalinvn, NCH, 1);

  /* top part of comparator */
  c1 = (compare_bits) * (draincap(Wcompn, NCH, 1) + draincap(Wcompn, NCH, 2) +
                         draincap(Wcomppreequ, NCH, 1)) +
       gatecap(WdecNORn, 1.0) +
       gatecap(WdecNORp, 3.0);

  return c1 + c2;
}

double dcl_compare_power(int32_t compare_bits, int32_t decodeWidth, double frq, double vdd) {
  double Ctotal;
  int32_t num_comparators;

  num_comparators = (decodeWidth - 1) * (decodeWidth);
  Ctotal = num_comparators * compare_cap(compare_bits);

  return Ctotal * Powerfactor * AF;
}

double simple_array_power(int32_t rows, int32_t cols, int32_t rports, int32_t wports, int32_t cache, double frq, double vdd, double & clockCap) {
  if (!cache) {
    return( simple_array_decoder_power(rows, cols, rports, wports, frq, vdd) +
            simple_array_wordline_power(rows, cols, rports, wports, frq, vdd) +
            simple_array_bitline_power(rows, cols, rports, wports, cache, frq, vdd, clockCap));
  } else {
    return( simple_array_decoder_power(rows, cols, rports, wports, frq, vdd) +
            simple_array_wordline_power(rows, cols, rports, wports, frq, vdd) +
            simple_array_bitline_power(rows, cols, rports, wports, cache, frq, vdd, clockCap) +
            senseamp_power(cols, vdd));
  }
}


double cam_tagdrive(int32_t rows, int32_t cols, int32_t rports, int32_t wports, double frq, double vdd) {
  double Ctotal, Ctlcap, Cblcap, Cwlcap;
  double taglinelength;
  double wordlinelength;
  double nsize, psize;
  int32_t ports;

  Ctotal = 0;

  ports = rports + wports;

  taglinelength = rows * (CamCellHeight + ports * MatchlineSpacing);
  wordlinelength = cols * (CamCellWidth + ports * TaglineSpacing);

  /* Compute tagline cap */
  Ctlcap = Cmetal * taglinelength +
           rows * gatecappass(Wcomparen2, 2.0) +
           draincap(Wcompdrivern, NCH, 1) + draincap(Wcompdriverp, PCH, 1);

  /* Compute bitline cap (for writing new tags) */
  Cblcap = Cmetal * taglinelength + rows * draincap(Wmemcellr, NCH, 2);

  /* autosize wordline driver */
  psize = driver_size(Cmetal * wordlinelength + 2 * cols * gatecap(Wmemcellr, 2.0), Period / 8);
  nsize = psize * Wdecinvn / Wdecinvp;

  /* Compute wordline cap (for writing new tags) */
  Cwlcap = Cmetal * wordlinelength + draincap(nsize, NCH, 1) + draincap(psize, PCH, 1) + 2 * cols * gatecap(Wmemcellr, 2.0);
  Ctotal += (rports * cols * 2 * Ctlcap) + (wports * ((cols * 2 * Cblcap) + (rows * Cwlcap)));

  return Ctotal * Powerfactor * AF;
}

double cam_tagmatch(int32_t rows, int32_t cols, int32_t rports, int32_t wports, double frq, double vdd, double & clockCap, int32_t issueWidth) {
  double Ctotal, Cmlcap;
  double matchlinelength;
  int32_t ports;

  Ctotal = 0;

  ports = rports + wports;

  matchlinelength = cols * (CamCellWidth + ports * TaglineSpacing);

  Cmlcap = 2 * cols * draincap(Wcomparen1, NCH, 2) +
           Cmetal * matchlinelength + draincap(Wmatchpchg, NCH, 1) +
           gatecap(Wmatchinvn + Wmatchinvp, 10.0) +
           gatecap(Wmatchnandn + Wmatchnandp, 10.0);

  Ctotal += rports * rows * Cmlcap;

  clockCap += rports * rows * gatecap(Wmatchpchg, 5.0);

  /* noring the nanded match lines */
  if (issueWidth >= 8) {
    Ctotal += 2 * gatecap(Wmatchnorn + Wmatchnorp, 10.0);
  }

  return Ctotal * Powerfactor * AF;
}


double cam_array(int32_t rows, int32_t cols, int32_t rports, int32_t wports, double frq, double vdd, double & clockCap, int32_t issueWidth) {
  return(cam_tagdrive(rows, cols, rports, wports, frq, vdd) + cam_tagmatch(rows, cols, rports, wports, frq, vdd, clockCap, issueWidth));
}

double selection_power(int32_t win_entries, double frq, double vdd, int32_t issueWidth) {
  double Ctotal, Cor, Cpencode;
  int32_t num_arbiter = 1;

  Ctotal = 0;

  while (win_entries > 4) {
    win_entries = (int)ceil((double)win_entries / 4.0);
    num_arbiter += win_entries;
  }

  Cor = 4 * draincap(WSelORn, NCH, 1) + draincap(WSelORprequ, PCH, 1);

  Cpencode = draincap(WSelPn, NCH, 1) + draincap(WSelPp, PCH, 1) +
             2 * draincap(WSelPn, NCH, 1) + draincap(WSelPp, PCH, 2) +
             3 * draincap(WSelPn, NCH, 1) + draincap(WSelPp, PCH, 3) +
             4 * draincap(WSelPn, NCH, 1) + draincap(WSelPp, PCH, 4) +
             4 * gatecap(WSelEnn + WSelEnp, 20.0) +
             4 * draincap(WSelEnn, NCH, 1) + 4 * draincap(WSelEnp, PCH, 1);

  Ctotal += issueWidth * num_arbiter * (Cor + Cpencode);

  return Ctotal * Powerfactor * AF ;
}

double compute_resultbus_power(double frq, double vdd, int32_t numPhysicalRegisters, int32_t numFus, int32_t issueWidth, int32_t dataPathWidth) {
  double Ctotal, Cline;
  double regfile_height;

  /* compute size of result bus tags */
  int32_t npreg_width = static_cast<int>(ceil(logtwo(static_cast<double>(numPhysicalRegisters))));

  Ctotal = 0;

  regfile_height = numPhysicalRegisters * (RegCellHeight + WordlineSpacing * 3 * issueWidth);

  /* assume num alu's == ialu	(FIXME: generate a more detailed result bus network model*/
  Cline = Cmetal * (regfile_height + .5 * numFus  * 3200.0 * LSCALE);

  /* or use result bus length measured from 21264 die photo */
  /*	Cline = Cmetal * 3.3*1000;*/

  // Power is for a single result bus access
  Ctotal += 2.0 * (dataPathWidth + npreg_width) * Cline;

  return Ctotal * Powerfactor * AF;
}
