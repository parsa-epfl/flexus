
#ifndef FLEXUS_DECODER_INSTRUCTIONCOMPONENTBUFFER_HPP_INCLUDED
#define FLEXUS_DECODER_INSTRUCTIONCOMPONENTBUFFER_HPP_INCLUDED

#include <core/stats.hpp>
#include <core/types.hpp>

static const size_t kICBSize = 2048;

namespace nDecoder {
extern Flexus::Stat::StatCounter theICBs;

struct UncountedComponent
{
    virtual ~UncountedComponent() {}
};

struct InstructionComponentBuffer
{
    size_t theComponentCount;
    std::vector<UncountedComponent*> theComponents;

    InstructionComponentBuffer()
      : theComponentCount(0)
    {
        theComponents.reserve(kICBSize);
    }

    ~InstructionComponentBuffer()
    {
        for (auto aComp : theComponents) {
            delete aComp;
        }
    }

    size_t addNewComponent(UncountedComponent* aComponent)
    {
        try {
            if (++theComponentCount >= theComponents.capacity()) { theComponents.reserve(theComponents.size() * 2); }
            theComponents.emplace_back(aComponent);
            return theComponentCount;
        } catch (std::exception& e) {
            DBG_Assert(false,
                       (<< "Could not add component " << aComponent << " to ICB. Number of components stored: "
                        << theComponentCount << ", vector capacity: " << theComponents.capacity()
                        << ", threw exception type: " << e.what()));
            return 0; // dodge compiler error
        }
    }
};

} // namespace nDecoder

#endif // FLEXUS_DECODER_INSTRUCTIONCOMPONENTBUFFER_HPP_INCLUDED
