#ifndef _BIT_UTILITIES_HPP_DEFINED
#define _BIT_UTILITIES_HPP_DEFINED
// Helper for shifting and extracting values from bit-ranges
unsigned long long extractBitsWithBounds(unsigned long long input, unsigned upperBitBound,
                                         unsigned lowerBitBound);
unsigned long long extractBitsWithRange(unsigned long long input, unsigned lowerBound,
                                        unsigned numBits);
bool extractSingleBitAsBool(unsigned long long input, unsigned bitNum);
#endif // _BIT_UTILITIES_HPP_DEFINED_
