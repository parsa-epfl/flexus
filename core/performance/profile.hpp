#ifndef FLEXUS_PROFILE_HPP_INCLUDED
#define FLEXUS_PROFILE_HPP_INCLUDED

#include <boost/preprocessor/cat.hpp>
#include <string>
#include <vector>

namespace nProfile {

inline int64_t
rdtsc()
{
    return 0;
}

class Timer;
class ManualTimer;
class Profiler;

class ProfileManager
{
    std::vector<Profiler*> theProfilers;
    int64_t theStartTime;

  public:
    ProfileManager() { theStartTime = rdtsc(); }
    void addProfiler(Profiler* aProfiler) { theProfilers.push_back(aProfiler); }
    inline int64_t programTime() { return (rdtsc() - theStartTime) / 1000; }
    void report(std::ostream&);
    void reset();
    static ProfileManager* profileManager();
};

extern Profiler* theProfileTOS;

class Profiler
{
    std::string theFn;
    std::string theFile;
    int64_t theLine;
    int64_t theTimeIn;
    int64_t theTimeAccum;
    int64_t theTimeAccumChildren;
    friend class Timer;
    friend class ManualTimer;

  public:
    std::string const& name() const { return theFn; }
    std::string const& file() const { return theFile; }
    int64_t line() const { return theLine; }
    int64_t totalTime() const { return theTimeAccum / 1000; }
    int64_t selfTime() const { return (theTimeAccum - theTimeAccumChildren) / 1000; }

    Profiler(std::string const& aFn, std::string const& aFile, int64_t aLine)
      : theFn(aFn)
      , theFile(aFile)
      , theLine(aLine)
      , theTimeIn(0)
      , theTimeAccum(0)
      , theTimeAccumChildren(0)
    {
        ProfileManager::profileManager()->addProfiler(this);
    }

    void reset()
    {
        theTimeIn            = 0;
        theTimeAccum         = 0;
        theTimeAccumChildren = 0;
    }
};

class Timer
{
    Profiler& theProfiler;
    Profiler* theParent;

  public:
    inline Timer(Profiler& aProfiler)
      : theProfiler(aProfiler)
    {
        theParent             = theProfileTOS;
        theProfileTOS         = &theProfiler;
        theProfiler.theTimeIn = rdtsc();
    }
    inline ~Timer()
    {
        if (theProfiler.theTimeIn != 0) {
            int64_t delta = rdtsc() - theProfiler.theTimeIn;
            if (delta > 0) {
                theProfiler.theTimeAccum += delta;
                if (theParent) { theParent->theTimeAccumChildren += delta; }
            }
        }
        theProfileTOS         = theParent;
        theProfiler.theTimeIn = 0;
    }
};

class ManualTimer
{
    Profiler& theProfiler;

  public:
    inline ManualTimer(Profiler& aProfiler)
      : theProfiler(aProfiler)
    {
    }

    inline void start() { theProfiler.theTimeIn = rdtsc(); }

    inline void stop()
    {
        if (theProfiler.theTimeIn != 0) {
            int64_t delta = rdtsc() - theProfiler.theTimeIn;
            if (delta > 0) { theProfiler.theTimeAccum += delta; }
        }
        theProfiler.theTimeIn = 0;
    }
};

#undef PROFILING_ENABLED
#ifdef PROFILING_ENABLED

#define FLEXUS_PROFILE()                                                                                               \
    static nProfile::Profiler BOOST_PP_CAT(profiler, __LINE__)(__FUNCTION__, __FILE__, __LINE__);                      \
    nProfile::Timer BOOST_PP_CAT(timer, __LINE__)(BOOST_PP_CAT(profiler, __LINE__)) /**/

#define FLEXUS_PROFILE_N(name)                                                                                         \
    static nProfile::Profiler BOOST_PP_CAT(profiler, __LINE__)(name, __FILE__, __LINE__);                              \
    nProfile::Timer BOOST_PP_CAT(timer, __LINE__)(BOOST_PP_CAT(profiler, __LINE__)) /**/

#else

#define FLEXUS_PROFILE()       while (false)
#define FLEXUS_PROFILE_N(name) while (false)

#endif

} // namespace nProfile

#endif // FLEXUS_PROFILE_HPP_INCLUDED
