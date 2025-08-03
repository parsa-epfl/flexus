#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>

namespace Flexus {
namespace SharedTypes {

std::ostream&
operator<<(std::ostream& anOstream, eSquashCause aCause)
{
    const char* causes[] = { "Invalid",   "Resynchronize", "BranchMispredict",
                             "Exception", "Interrupt",     "FailedSpeculation" };
    if (aCause > 5) {
        anOstream << "Invalid Squash Cause(" << static_cast<int>(aCause) << ")";
    } else {
        anOstream << causes[aCause];
    }
    return anOstream;
}

std::ostream&
operator<<(std::ostream& anOstream, AbstractInstruction const& anInstruction)
{
    anInstruction.describe(anOstream);
    return anOstream;
}

void
AbstractInstruction::describe(std::ostream& anOstream) const
{
}
bool
AbstractInstruction::haltDispatch() const
{
    return false;
}

} // namespace SharedTypes
} // namespace Flexus
