#ifndef FLEXUS_DRIVE_REFERENCE_HPP_INCLUDED
#define FLEXUS_DRIVE_REFERENCE_HPP_INCLUDED

namespace Flexus {
namespace Core {

struct DriveBase
{
    // This method is called every cycle.
    virtual ~DriveBase() {};
    virtual void doCycle() = 0;
};

typedef DriveBase& DriveReference;

} // End namespace Core
namespace Wiring {

extern Flexus::Core::DriveReference theDrive;

} // End Namespace Wiring
} // namespace Flexus

#endif // FLEXUS_DRIVE_REFERENCE_HPP_INCLUDED
