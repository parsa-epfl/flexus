#ifndef FLEXUS_WATCH_FUNCTIONS_HPP_INCLUDED
#define FLEXUS_WATCH_FUNCTIONS_HPP_INCLUDED

// Functions and whatnot from Cacti 4.1
// Cacti header reproduced below:
/*------------------------------------------------------------
 *                              CACTI 4.0
 *         Copyright 2005 Hewlett-Packard Development Corporation
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein, and
 * hereby grant back to Hewlett-Packard Company and its affiliated companies ("HP")
 * a non-exclusive, unrestricted, royalty-free right and license under any changes,
 * enhancements or extensions  made to the core functions of the software, including
 * but not limited to those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to HP any such changes,
 * enhancements or extensions that they make and inform HP of noteworthy uses of
 * this software.  Correspondence should be provided to HP at:
 *
 *                       Director of Intellectual Property Licensing
 *                       Office of Strategy and Technology
 *                       Hewlett-Packard Company
 *                       1501 Page Mill Road
 *                       Palo Alto, California  94304
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND HP DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL HP
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

// Structs from cacti
typedef struct {
  int32_t cache_size;
  int32_t number_of_sets;
  int32_t associativity;
  int32_t block_size;
} cacti_parameter_type;

typedef struct {
  double access_time, cycle_time;
  int32_t best_Ndwl, best_Ndbl;
  int32_t best_Nspd;
  int32_t best_Ntwl, best_Ntbl;
  int32_t best_Ntspd;
  double decoder_delay_data, decoder_delay_tag;
  double dec_data_driver, dec_data_3to8, dec_data_inv;
  double dec_tag_driver, dec_tag_3to8, dec_tag_inv;
  double wordline_delay_data, wordline_delay_tag;
  double bitline_delay_data, bitline_delay_tag;
  double sense_amp_delay_data, sense_amp_delay_tag;
  double compare_part_delay;
  double drive_mux_delay;
  double selb_delay;
  double data_output_delay;
  double drive_valid_delay;
  double precharge_delay;
} cacti_result_type;

double logtwo(double x);
double gatecap(double width, double wirelength);
double gatecappass(double width, double wirelength);
double draincap(double width, bool nchannel, int32_t stack);
double transresswitch(double width, bool nchannel, int32_t stack);
double transreson(double width, bool nchannel, int32_t stack);
double restowidth(double res, bool nchannel);
double horowitz(double inputramptime, double tf, double vs1, double vs2, int32_t rise);
double decoder_delay(int32_t C, int32_t B, int32_t A, int32_t Ndwl, int32_t Ndbl, int32_t Nspd, int32_t Ntwl, int32_t Ntbl, int32_t Ntspd, double * Tdecdrive, double * Tdecoder1, double * Tdecoder2, double * outrisetime);
double decoder_tag_delay(int32_t C, int32_t B, int32_t A, int32_t Ndwl, int32_t Ndbl, int32_t Nspd, int32_t Ntwl, int32_t Ntbl, int32_t Ntspd, double * Tdecdrive, double * Tdecoder1, double * Tdecoder2, double * outrisetime);
double wordline_delay(int32_t B, int32_t A, int32_t Ndwl, int32_t Nspd, double inrisetime, double * outrisetime);
double wordline_tag_delay(int32_t C, int32_t A, int32_t Ntspd, int32_t Ntwl, double inrisetime, double * outrisetime);
double bitline_delay(int32_t C, int32_t A, int32_t B, int32_t Ndwl, int32_t Ndbl, int32_t Nspd, double inrisetime, double * outrisetime, double baseVdd);
double bitline_tag_delay(int32_t C, int32_t A, int32_t B, int32_t Ntwl, int32_t Ntbl, int32_t Ntspd, double inrisetime, double * outrisetime, double baseVdd);
double sense_amp_delay(double inrisetime, double * outrisetime);
double sense_amp_tag_delay(double inrisetime, double * outrisetime);
double compare_time(int32_t C, int32_t A, int32_t Ntbl, int32_t Ntspd, double inputtime, double * outputtime, double baseVdd);
double mux_driver_delay(int32_t C, int32_t B, int32_t A, int32_t Ndbl, int32_t Nspd, int32_t Ndwl, int32_t Ntbl, int32_t Ntspd, double inputtime, double * outputtime);
double valid_driver_delay(int32_t C, int32_t A, int32_t Ntbl, int32_t Ntspd, double inputtime);
double dataoutput_delay(int32_t C, int32_t B, int32_t A, int32_t Ndbl, int32_t Nspd, int32_t Ndwl, double inrisetime, double * outrisetime);
double selb_delay_tag_path(double inrisetime, double * outrisetime);
double precharge_delay(double worddata);
int32_t organizational_parameters_valid(int32_t rows, int32_t cols, int32_t Ndwl, int32_t Ndbl, int32_t Nspd, int32_t Ntwl, int32_t Ntbl, int32_t Ntspd);
void calculate_time(cacti_result_type * result, cacti_parameter_type * parameters, double baseVdd);

// Functions and whatnot from Wattch's power.h
#ifndef FLEXUS
#include <sys/types.h>
#else
typedef int64_t int32_t quad_t;
#endif
typedef uint64_t int32_t counter_t;

int32_t pow2(int32_t x);
double logfour(double x);
int32_t pop_count_slow(quad_t bits);
int32_t pop_count(quad_t bits);
double compute_af(counter_t num_pop_count_cycle, counter_t total_pop_count_cycle, int32_t pop_width);
int32_t squarify(int32_t rows, int32_t cols);
double squarify_new(int32_t rows, int32_t cols);
double driver_size(double driving_cap, double desiredrisetime);
double array_decoder_power(int32_t rows, int32_t cols, double predeclength, int32_t rports, int32_t wports, double frq, double vdd);
double simple_array_decoder_power(int32_t rows, int32_t cols, int32_t rports, int32_t wports, double frq, double vdd);
double array_wordline_power(int32_t rows, int32_t cols, double wordlinelength, int32_t rports, int32_t wports, double frq, double vdd);
double simple_array_wordline_power(int32_t rows, int32_t cols, int32_t rports, int32_t wports, double frq, double vdd);
double array_bitline_power(int32_t rows, int32_t cols, double bitlinelength, int32_t rports, int32_t wports, int32_t cache, double frq, double vdd, double & clockCap);
double simple_array_bitline_power(int32_t rows, int32_t cols, int32_t rports, int32_t wports, int32_t cache, double frq, double vdd, double & clockCap);
double senseamp_power(int32_t cols, double vdd);
double compare_cap(int32_t compare_bits);
double dcl_compare_power(int32_t compare_bits, int32_t decodeWidth, double frq, double vdd);
double simple_array_power(int32_t rows, int32_t cols, int32_t rports, int32_t wports, int32_t cache, double frq, double vdd, double & clockCap);
double cam_tagdrive(int32_t rows, int32_t cols, int32_t rports, int32_t wports, double frq, double vdd);
double cam_tagmatch(int32_t rows, int32_t cols, int32_t rports, int32_t wports, double frq, double vdd, double & clockCap, int32_t issueWidth);
double cam_array(int32_t rows, int32_t cols, int32_t rports, int32_t wports, double frq, double vdd, double & clockCap, int32_t issueWidth);
double selection_power(int32_t win_entries, double frq, double vdd, int32_t issueWidth);
double compute_resultbus_power(double frq, double vdd, int32_t numPhysicalRegisters, int32_t numFus, int32_t issueWidth, int32_t dataPathWidth);

#endif
