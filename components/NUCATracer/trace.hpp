#ifndef TRACE_INCLUDED
#define TRACE_INCLUDED

#include "common.hpp"

namespace trace {

void initialize(std::string const & aPath, const bool aReadOnlyFlag);
void flushFiles();
void addL2Access(TraceData const & aRecord);
void addOCAccess(TraceData const & aRecord);

int32_t getL2Access(TraceData * aRecord);
int32_t getOCAccess(TraceData * aRecord);
}

#endif //TRACE_INCLUDED
