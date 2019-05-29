#ifndef FLEXUS__RMCENTRY_HPP_INCLUDED
#define FLEXUS__RMCENTRY_HPP_INCLUDED

#ifdef FLEXUS_RMCEntry_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::RMCEntry data type"
#endif
#define FLEXUS_RMCEntry_TYPE_PROVIDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/debug/debug.hpp>

#include <core/types.hpp>
#include <core/exception.hpp>

#include <components/CommonQEMU/rmc_breakpoints.hpp>

namespace Flexus {
namespace SharedTypes {
	
struct RMCEntry {
public:
	RMCEntry() {
		type = 0;
		v_address = 0;
		size = 0;
		context_id = 0;
	}
	
	RMCEntry(uint64_t t, uint64_t va, uint64_t s, int aCPUid) {
		type = t;
		v_address = va;
		size = s;
		cpu_id = aCPUid;
		context_id = -1;
	}	
	
	RMCEntry(uint64_t t, uint64_t cid, uint64_t va, uint64_t s, int aCPUid) {
		type = t;
		v_address = va;
		size = s;
		cpu_id = aCPUid;
		context_id = (int64_t)cid;
	}	
	
	~RMCEntry() {;}
	
	//accessors
	const uint64_t getType() const{
		return type;
	}	
	const uint64_t getAddress() const{
		return v_address;
	}	
	const uint64_t getSize() const{
		return size;
	}	
	const int getCPUid() const{
		return cpu_id;
	}
	const uint64_t getContext() const{
		return context_id;
	}	
	
protected:
	uint64_t type;
	uint64_t v_address;
	uint64_t size;
	int64_t context_id;
	int cpu_id;
};		

struct SABReEntry {
public:
    SABReEntry(uint32_t anATTidx, uint64_t anAddress, uint32_t aLength) {
        ATTidx = anATTidx;
        baseAddress = anAddress;
        SABReLength = aLength;
    }

    ~SABReEntry() {}

    //accessors
    const uint32_t getATTidx() const{
        return ATTidx;
    }

    const uint64_t getAddress() const{
        return baseAddress;
    }

    const uint32_t getSABReLength() const{
        return SABReLength;
    }

protected:
    uint32_t   ATTidx, SABReLength;
    uint64_t   baseAddress;
};
}
}
#endif //FLEXUS__RMCENTRY_HPP_INCLUDED

