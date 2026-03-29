
#include "ValueTracker.hpp"

namespace nuArch {

ValueTracker** ValueTracker::theGlobalTracker = nullptr;
int ValueTracker::theNumTrackers              = 0;

} // namespace nuArchARM