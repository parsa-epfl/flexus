#ifndef FLEXUS__CORE_TEST

#ifndef FLEXUS__LAYOUT_DRIVE_SECTION
#error "End of component drive section without preceding begin."
#endif // FLEXUS__LAYOUT_COMPONENT_WIRING_SECTION

#undef FLEXUS__LAYOUT_DRIVE_SECTION
#undef FLEXUS__LAYOUT_IN_SECTION

#define FLEXUS__LAYOUT_DRIVES_ORDERED

#endif // FLEXUS__CORE_TEST

> drive_list;

#undef DRIVE

using Flexus::Core::Drive;
using Flexus::Core::DriveReference;

// Create the core Drive component which takes care of everything else
Drive<drive_list> drive;
DriveReference theDrive = drive;

} // End namespace Flexus
} // End namespace Wiring
