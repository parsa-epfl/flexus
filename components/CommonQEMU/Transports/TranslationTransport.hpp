#ifndef FLEXUS_TRANSPORTS__TRANSLATION_TRANSPORT_HPP_INCLUDED
#define FLEXUS_TRANSPORTS__TRANSLATION_TRANSPORT_HPP_INCLUDED

#include <boost/mpl/vector.hpp>
#include <components/CommonQEMU/Translation.hpp>
#include <components/MMU/TranslationState.hpp>
#include <core/transport.hpp>

#pragma GCC diagnostic ignored "-Wunused-variable"

namespace Flexus {
namespace SharedTypes {

#ifndef FLEXUS_TAG_TranslationBasicTag
#define FLEXUS_TAG_TranslationBasicTag
struct TranslationBasicTag_t
{};
namespace {
TranslationBasicTag_t TranslationBasicTag;
}
#endif // FLEXUS_TAG_TranslationBasicTag

#ifndef FLEXUS_TAG_TranslationStatefulTag
#define FLEXUS_TAG_TranslationStatefulTag
struct TranslationStatefulTag_t
{};
namespace {
TranslationStatefulTag_t TranslationStatefulTag;
}
#endif // FLEXUS_TAG_TranslationStatefulTag

typedef Transport<mpl::vector<transport_entry<TranslationBasicTag_t, Translation>,
                              transport_entry<TranslationStatefulTag_t, nMMU::TranslationState>>>
  TranslationTransport;

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_TRANSPORTS__TRANSLATION_TRANSPORT_HPP_INCLUDED
