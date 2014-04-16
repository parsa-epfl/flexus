#ifndef COORDINATOR_INCLUDED
#define COORDINATOR_INCLUDED

class tCoordinator : public TraceCoordinator {

public:
  tCoordinator(std::string const & aPath, const bool aReadOnlyFlag);
  virtual ~tCoordinator();

  virtual void finalize( void );
  virtual void processTrace( void );

  //Fine grained interface - to increase efficiency
  virtual void accessL2(tTime aTime, tEventType aType, tCoreId aNode, tThreadId aThread, tAddress anAddress, tVAddress aPC, tFillType aFillType, tFillLevel aFillLevel, bool anOS);
  virtual void accessOC(tTime aTime, tEventType aType, tCoreId aNode, tThreadId aThread, tAddress anAddress, tVAddress aPC, tFillType aFillType, tFillLevel aFillLevel, bool anOS);

  virtual int32_t getL2Access(TraceData * aRecord);
  virtual int32_t getOCAccess(TraceData * aRecord);
};

#endif //COORDINATOR_INCLUDED
