/*  Filename:    histogram.h
 *  Description: A generic class which does the dirty work of calculating
 *   histograms for discrete integer events, when given a list of bucket
 *   positions to fill.
 * 
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */

#ifndef _HISTOGRAM_H_
#define _HISTOGRAM_H_

#ifndef COUNTER
typedef long long COUNTER;
#endif


#ifndef COMMA
#define COMMA " " 
#endif

#include <iostream>
#include <math.h>

using namespace std;

#ifndef LLONG_MIN
#   define LLONG_MAX    9223372036854775807LL
#   define LLONG_MIN    (-LLONG_MAX - 1LL)
#endif

static const COUNTER HISTOGRAM_NEGATIVE_INFINITY = LLONG_MIN;
static const COUNTER HISTOGRAM_POSITIVE_INFINITY = LLONG_MAX;
static const COUNTER NO_BUCKET = -1;

class Histogram
{

 public:
  /* Initializes the Histogram, by passing a sorted (ascending) list of 
   *  minimum cutoff bucket values (the last value cooresponds to a maximum
   *  cutoff for the last bucket
   */
  Histogram ( const COUNTER * bucketList,
              const COUNTER   bucketCount, 
              const bool      dontInitialize = false );
  virtual ~Histogram();

  /* Bumps up the histogram count for the bucket which contains
   *  the datapoint
   */
  virtual bool increment ( const long dataPoint )
    { COUNTER c = dataPoint; return increment ( c ); }

  virtual bool increment ( const int dataPoint )
  { COUNTER c = dataPoint; return increment ( c ); }

  virtual bool increment ( const COUNTER dataPoint );

  /* Adds the specified value (positive or negative) to the 
   * bucket which contains the specified datapoint 
   */
  virtual bool add ( const long dataPoint, const long value )
    { return add ( (COUNTER)dataPoint, (COUNTER)value ); }

  virtual bool add ( const int dataPoint, const int value )
    { return add ( (COUNTER)dataPoint, (COUNTER)value ); }

  virtual bool add ( const COUNTER dataPoint, const COUNTER value );
 
  /* Gets the minimum value allowed in bucket number 'bucket' */
  virtual bool getBucketMinValue ( const COUNTER  bucket, 
				   COUNTER       &minValue );
  
  /* Gets the maximum value allowed in bucket number 'bucket' */
  virtual bool getBucketMaxValue ( const COUNTER  bucket, 
				   COUNTER       &maxValue );

  /* Zeros all bucket values */
  bool clearBuckets ( void );

  /* Retrieves the value for the specified bucket */
  bool getBucketValue ( const int bucket, COUNTER &value );
  
  bool getNumBuckets ( int &num );

  bool getAccumulation ( COUNTER & accum );

  bool getPointCount   ( COUNTER & ptCount, 
                         const bool onlyAboveZero = false );

  bool getMean         ( double & mean, 
                         const bool onlyAboveZero = false );

  /* Output all of the statistics in a simple CSV style output */
  bool outputCSVStats ( ostream & out );

  /* Output all of the statistics, but in a fractactional form where
   * the buckets sum to 1.0 
   */
  bool outputCSVFractionalStats ( ostream & out );

  bool outputCSVHeader ( ostream & out );
  
 public:
  static bool buildLinearBucketList ( const COUNTER interval, 
                                      const COUNTER low,
                                      const COUNTER high,
                                      COUNTER *& buckets,
                                      COUNTER  & bucketCount );
  
  static bool buildLogBucketList ( const double  base,
                                   const COUNTER low,
                                   const COUNTER high,
                                   COUNTER *& buckets,
                                   COUNTER & bucketCount );
  
 protected:

  virtual int  findBucketPosition ( const COUNTER dataPoint );
  virtual bool initializeBucketList ( const COUNTER * bucketList );

 protected:

  COUNTER
    *buckets,          /* The list of minimum bucket cutoffs */
    *bucketValues;     /* The list of values which lie between the cutoffs */
  
  COUNTER 
    bucketCount;

  COUNTER
    accumulation;      /* An accumulation of all values in the histogram */
};

class CategoryHistogram : public Histogram
{
  public:
  
  CategoryHistogram ( const COUNTER * bucketList, 
                      const COUNTER   bucketCount );
  virtual ~CategoryHistogram ( void );

 public:
  
 protected:
  
  virtual int findBucketPosition ( const COUNTER dataPoint );
  virtual bool initializeBucketList ( const COUNTER * bucketList );
};

extern const COUNTER HIST_BUCKETS_POWERS_OF_2[];
extern const int     HIST_BUCKETS_POWERS_OF_2_COUNT;

#endif /* _HISTOGRAM_H_ */
