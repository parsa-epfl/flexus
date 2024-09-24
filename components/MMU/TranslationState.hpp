#ifndef FLEXUS_SLICES__TRANS_STATE_HPP_INCLUDED
#define FLEXUS_SLICES__TRANS_STATE_HPP_INCLUDED
#include "TTResolvers.hpp"

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <memory> // for shared_ptr

namespace nMMU {

class TTResolver;

struct TranslationState : public boost::counted_base
{
    // Stuff that carries MMU-internal state.
    uint8_t ELRegime;
    uint8_t requiredTableLookups;
    uint8_t currentLookupLevel;
    bool isBR0;
    uint32_t granuleSize;
    std::shared_ptr<TTResolver> TTAddressResolver;
    uint64_t BlockSizeFromTTs;
};

} // namespace nMMU
#endif // FLEXUS_SLICES__TRANS_STATE_HPP_INCLUDED
