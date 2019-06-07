// -*- c++ -*-

//aledaglis: taken from https://github.com/leverich/mutilate

// 1. implement "fixed" generator
// 2. implement discrete generator
// 3. implement combine generator?

#ifndef GENERATOR_H
#define GENERATOR_H

#define MAX(a,b) ((a) > (b) ? (a) : (b))

#include <string>
#include <vector>
#include <utility>

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "StatsContainer.hpp"


// Generator syntax:
//
// \d+ == fixed
// n[ormal]:mean,sd
// e[xponential]:lambda
// p[areto]:scale,shape
// g[ev]:loc,scale,shape
// fb_value, fb_key, fb_rate

class Generator {
public:
  Generator() {}
  //  Generator(const Generator &g) = delete;
  //  virtual Generator& operator=(const Generator &g) = delete;
  virtual ~Generator() {}

  virtual double generate(double U = -1.0) = 0;
  virtual void set_lambda(double lambda) {
    fprintf(stderr,"set_lambda() not implemented");
    exit(-1);
  }
  virtual void resetContainer() { }
protected:
  std::string type;
};

class Fixed : public Generator {
public:
  Fixed(double _value = 1.0) : value(_value) { fprintf(stderr,"Fixed(%f)", value); }
  virtual double generate(double U = -1.0) { return value; }
  virtual void set_lambda(double lambda) {
    if (lambda > 0.0) value = 1.0 / lambda;
    else value = 0.0;
  }

private:
  double value;
};

class Uniform : public Generator {
public:
  Uniform(double _scale) : scale(_scale) { fprintf(stderr,"Uniform(%f)", scale); }

  virtual double generate(double U = -1.0) {
    if (U < 0.0) U = drand48();
    return scale * U;
  }

  virtual void set_lambda(double lambda) {
    if (lambda > 0.0) scale = 2.0 / lambda;
    else scale = 0.0;
  }

private:
  double scale;
};

class Normal : public Generator {
public:
  Normal(double _mean = 1.0, double _sd = 1.0) : mean(_mean), sd(_sd) {
    fprintf(stderr,"Normal(mean=%f, sd=%f)", mean, sd);
  }

  virtual double generate(double U = -1.0) {
    if (U < 0.0) U = drand48();
    double V = U; // drand48();
    double N = sqrt(-2 * log(U)) * cos(2 * M_PI * V);
    return mean + sd * N;
  }

  virtual void set_lambda(double lambda) {
    if (lambda > 0.0) mean = 1.0 / lambda;
    else mean = 0.0;
  }

private:
  double mean, sd;
};

class Exponential : public Generator {
public:
  Exponential(double _lambda = 1.0) : lambda(_lambda) {
    fprintf(stderr,"Exponential(lambda=%f)", lambda);
  }

  virtual double generate(double U = -1.0) {
    if (lambda <= 0.0) return 0.0;
    if (U < 0.0) U = drand48();
    return -log(U) / lambda;
  }

  virtual void set_lambda(double lambda) { this->lambda = lambda; }

private:
  double lambda;
};

class GPareto : public Generator {
public:
  GPareto(double _loc = 0.0, double _scale = 1.0, double _shape = 1.0) :
    loc(_loc), scale(_scale), shape(_shape) {
    assert(shape != 0.0);
    fprintf(stderr,"GPareto(loc=%f, scale=%f, shape=%f)", loc, scale, shape);
  }

  virtual double generate(double U = -1.0) {
    if (U < 0.0) U = drand48();
    return loc + scale * (pow(U, -shape) - 1) / shape;
  }

  virtual void set_lambda(double lambda) {
    if (lambda <= 0.0) scale = 0.0;
    else scale = (1 - shape) / lambda - (1 - shape) * loc;
  }

private:
  double loc /* mu */;
  double scale /* sigma */, shape /* k */;
};

class GEV : public Generator {
public:
  GEV(double _loc = 0.0, double _scale = 1.0, double _shape = 1.0) :
    e(1.0), loc(_loc), scale(_scale), shape(_shape) {
    assert(shape != 0.0);
    fprintf(stderr,"GEV(loc=%f, scale=%f, shape=%f)", loc, scale, shape);
  }

  virtual double generate(double U = -1.0) {
    return loc + scale * (pow(e.generate(U), -shape) - 1) / shape;
  }

private:
  Exponential e;
  double loc /* mu */, scale /* sigma */, shape /* k */;
};

class Discrete : public Generator {
public:
  ~Discrete() { delete def; }
  Discrete(Generator* _def = NULL) : def(_def) {
    if (def == NULL) def = new Fixed(0.0);
  }

  virtual double generate(double U = -1.0) {
    double Uc = U;
    if (pv.size() > 0 && U < 0.0) U = drand48();

    double sum = 0;

    std::vector< std::pair<double,double> >::iterator p;

    for (p = pv.begin(); p < pv.end(); p++) {
      sum += p->first;
      if (U < sum) return p->second;
    }

    return def->generate(Uc);
  }

  void add(double p, double v) {
    pv.push_back(std::pair<double,double>(p, v));
  }

private:
  Generator *def;
  std::vector< std::pair<double,double> > pv;
};

class DistrFromFile : public Generator {
    private:
        StatsContainer<double> distrContainer;
        size_t nextSTime;

    public:
        DistrFromFile(std::string fname, uint64_t numElements) : distrContainer(), nextSTime(0)  
    {
        distrContainer.readFromFile(fname,numElements);
    }

        virtual double generate( double U = -1.0 ) {
            double ret = distrContainer.get(nextSTime++);
            if ( nextSTime >= distrContainer.size() ) resetContainer();
            return ret;
        }

        virtual void resetContainer() {
            nextSTime = 0;
        }
};

Generator* createGenerator(std::string str,bool fromFile, std::string fName,uint64_t numElements);
Generator* createFacebookKey();
Generator* createFacebookValue();
Generator* createFacebookIA();

#endif // GENERATOR_H
