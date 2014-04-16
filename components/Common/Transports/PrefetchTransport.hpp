#ifndef FLEXUS_TRANSPORTS__PREFETCH_TRANSPORT_HPP_INCLUDED
#define FLEXUS_TRANSPORTS__PREFETCH_TRANSPORT_HPP_INCLUDED

#include <core/transport.hpp>

#include <components/Common/Slices/PrefetchMessage.hpp>
#include <components/Common/Slices/PrefetchCommand.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

namespace Flexus {
namespace SharedTypes {

#ifndef FLEXUS_TAG_PrefetchMessageTag
#define FLEXUS_TAG_PrefetchMessageTag
struct PrefetchMessageTag_t {};
namespace {
PrefetchMessageTag_t PrefetchMessageTag;
}
#endif //FLEXUS_TAG_PredictorMessageTag

#ifndef FLEXUS_TAG_PrefetchCommandTag
#define FLEXUS_TAG_PrefetchCommandTag
struct PrefetchCommandTag_t {};
namespace {
PrefetchCommandTag_t PrefetchCommandTag;
}
#endif //FLEXUS_TAG_PredictorMessageTag

#ifndef FLEXUS_TAG_TransactionTrackerTag
#define FLEXUS_TAG_TransactionTrackerTag
struct TransactionTrackerTag_t {};
struct TransactionTracker;
namespace {
TransactionTrackerTag_t TransactionTrackerTag;
}
#endif //FLEXUS_TAG_TransactionTrackerTag

typedef Transport
< mpl::vector
< transport_entry< PrefetchMessageTag_t, PrefetchMessage >
, transport_entry< PrefetchCommandTag_t, PrefetchCommand >
, transport_entry< TransactionTrackerTag_t, TransactionTracker >
>
> PrefetchTransport;

} //namespace SharedTypes
} //namespace Flexus

#endif //FLEXUS_TRANSPORTS__PREFETCH_TRANSPORT_HPP_INCLUDED

