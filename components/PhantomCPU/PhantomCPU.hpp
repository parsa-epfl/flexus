#ifndef FLEXUS_PhantomCPU_HPP
#define FLEXUS_PhantomCPU_HPP

#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Transports/PredictorTransport.hpp> /* CMU-ONLY */
#include <components/uFetch/uFetchTypes.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/simulator_layout.hpp>

// clang-format off

#define FLEXUS_BEGIN_COMPONENT PhantomCPU
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( IndexOffSet, uint32_t, "Index Offset", "index_offset", 0)
  PARAMETER( EstimatedIPC, uint32_t, "Estimated IPC", "estimated_ipc", 5)
);

COMPONENT_INTERFACE(
  DRIVE(PhantomDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT PhantomCPU

// clang-format on
#endif