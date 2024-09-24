#ifndef _CACHEPLACEMENTPOLICYDEFN_HPP
#define _CACHEPLACEMENTPOLICYDEFN_HPP

// Cache placement policy
enum tPlacement
{
    kRNUCACache,
    kPrivateCache,
    kSharedCache,
    kASRCache
};

// Page classes
enum tPageState
{
    kPageStateInvalid         = 0x0,
    kPageStatePrivate         = 0x1,
    kPageStateShared          = 0x2,
    kPageStateCustomerService = 0x3
};

#endif // _CACHEPLACEMENTPOLICYDEFN_HPP
