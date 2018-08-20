#include "bitUtilities.hpp"

typedef unsigned long long address_t;

address_t 
extractBitsWithBounds(address_t input, unsigned upperBitBound, unsigned lowerBitBound)
{
    address_t result = input >> lowerBitBound;
    address_t upperMask = (1ULL << (upperBitBound-lowerBitBound+1))-1;
    return result & upperMask;
}

address_t 
extractBitsWithRange(address_t input, unsigned lbound, unsigned numBits)
{
    address_t result = input >> lbound;
    address_t upperMask = (1ULL << numBits)-1;
    return result & upperMask;
}

bool 
extractSingleBitAsBool(address_t input, unsigned bitshift)
{
    address_t rawbit = ((input >> bitshift) & 0x1);
    return rawbit ? true : false ;
}

