#ifndef FLEXUS_TRANSPORTS__PREDICTOR_TRANSPORT_HPP_INCLUDED
#define FLEXUS_TRANSPORTS__PREDICTOR_TRANSPORT_HPP_INCLUDED

#include <core/transport.hpp>

#include <components/Common/Slices/PredictorMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

namespace Flexus {
namespace SharedTypes {

#ifndef FLEXUS_TAG_PredictorMessageTag
#define FLEXUS_TAG_PredictorMessageTag
struct PredictorMessageTag_t {};
namespace {
PredictorMessageTag_t PredictorMessageTag;
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
< transport_entry< PredictorMessageTag_t, PredictorMessage >
, transport_entry< TransactionTrackerTag_t, TransactionTracker >
>
> PredictorTransport;

} //namespace SharedTypes
} //namespace Flexus

#endif //FLEXUS_TRANSPORTS__PREDICTOR_TRANSPORT_HPP_INCLUDED

