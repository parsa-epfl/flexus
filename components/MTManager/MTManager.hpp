#ifndef FLEXUS_MTManager_MTManager_HPP_INCLUDED
#define FLEXUS_MTManager_MTManager_HPP_INCLUDED

#include <core/target.hpp>
#include <core/types.hpp>

#include DBG_Control()

namespace nMTManager {

struct MTManager {
  static MTManager * get();

  virtual ~MTManager( ) {};
  virtual uint32_t  scheduleFAGThread( uint32_t  aCoreIndex ) = 0;
  virtual uint32_t  scheduleFThread( uint32_t  aCoreIndex ) = 0;
  virtual bool runThisEX( uint32_t  anIndex ) = 0;
  virtual bool runThisD( uint32_t  anIndex ) = 0;
};

} // namespace nMTManager

#endif //FLEXUS_MTManager_MTManager_HPP_INCLUDED
