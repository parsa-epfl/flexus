#ifndef FLEXUS_DRIVE_REFERENCE_HPP_INCLUDED
#define FLEXUS_DRIVE_REFERENCE_HPP_INCLUDED

#include <cstdint>
namespace Flexus {
namespace Core {

struct DriveBase
{
    // This method is called every cycle.
    virtual ~DriveBase() {};
    virtual uint32_t doCycle() = 0;
};

typedef DriveBase& DriveReference;

} // End namespace Core
namespace Wiring {

extern Flexus::Core::DriveReference theDrive;

} // End Namespace Wiring
} // namespace Flexus

#endif // FLEXUS_DRIVE_REFERENCE_HPP_INCLUDED
