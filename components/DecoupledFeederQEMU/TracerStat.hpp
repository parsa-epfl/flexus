
#ifndef FLEXUS_TRACER_STAT_HPP
#define FLEXUS_TRACER_STAT_HPP

#include "TracerCounter.hpp"

/**
 * Macro to define touch functions
 * This is definitely easier to replicate a function
 * that to over engineer one more complex
 * to achieve the same goal.
 */

#define DEFINE_TOUCH_FUNCTION(name, stat_field) \
void touch_##name(bool is_in_kernel) { \
    if (is_in_kernel) \
        os_counter.stat_field++; \
    else \
        user_counter.stat_field++; \
}
/**
 * Class that hold per core statitics for kernel and user
 * instruction.
 **/
class TracerStat
{

private:
    TracerCounter user_counter;
    TracerCounter os_counter;
public:
    TracerStat(std::size_t core_index):
        user_counter(boost::padded_string_cast<2, '0'>(core_index) + "-user-"),
        os_counter(boost::padded_string_cast<2, '0'>(core_index) + "-os-")
    {}

    void
    update_collector(void)
    {
        user_counter.update_collector();
        os_counter.update_collector();
    }

    // Use the macro to define touch functions for each statistic
    DEFINE_TOUCH_FUNCTION(feeder_io,        feeder_io)
    DEFINE_TOUCH_FUNCTION(feeder_rmw,       feeder_rmw)
    DEFINE_TOUCH_FUNCTION(feeder_fetch,     feeder_fetch)
    DEFINE_TOUCH_FUNCTION(feeder_prefetch,  feeder_prefetch)
    DEFINE_TOUCH_FUNCTION(feeder_load,      feeder_load)
    DEFINE_TOUCH_FUNCTION(feeder_store,     feeder_store)

    DEFINE_TOUCH_FUNCTION(itlb_hit,         itlb_hit)
    DEFINE_TOUCH_FUNCTION(itlb_miss,        itlb_miss)
    DEFINE_TOUCH_FUNCTION(itlb_request,     itlb_request)
    DEFINE_TOUCH_FUNCTION(itlb_access,      itlb_access)
    DEFINE_TOUCH_FUNCTION(dtlb_hit,         dtlb_hit)
    DEFINE_TOUCH_FUNCTION(dtlb_miss,        dtlb_miss)
    DEFINE_TOUCH_FUNCTION(dtlb_request,     dtlb_request)
    DEFINE_TOUCH_FUNCTION(dtlb_access,      dtlb_access)
};

#endif