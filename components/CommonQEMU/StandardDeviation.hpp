#include <cmath>

class StandardDeviation
{
  private:
    uint64_t k;
    double Sum;
    double SumSq;
    double SigmaSqSum;

  public:
    StandardDeviation(void)
    {
        k          = 0UL;
        Sum        = 0.0;
        SumSq      = 0.0;
        SigmaSqSum = 0.0;

        return;
    }

    void Reset(void)
    {
        k          = 0UL;
        Sum        = 0.0;
        SumSq      = 0.0;
        SigmaSqSum = 0.0;

        return;
    }

    void NewSample(const double X)
    {
        const double Xsq = X * X;

        SigmaSqSum += SumSq + k * Xsq - 2 * X * Sum;

        k++;
        Sum += X;
        SumSq += Xsq;

        return;
    }

    double GetStandardDeviation(void) const
    {
        if (k == 0) return 0.0;

        DBG_Assert(SigmaSqSum >= 0.0);
        return std::sqrt(SigmaSqSum) / (double)k;
    }
};

inline double
safe_div(const uint64_t a, const uint64_t b)
{
    return (b == 0 ? 0.0 : (double)a / b);
}
