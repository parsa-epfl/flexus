#if DBG_SetAssertions
// Crit debugging enabled
#undef DBG__internal_ASSERTIONS_ENABLED
#define DBG__internal_ASSERTIONS_ENABLED 1

#undef DBG__internal_Assert
#define DBG__internal_Assert(sev, condition, operations)                                           \
  DBG__internal_PROCESS_DBG(sev, Assert() Condition(!(condition)) operations) /**/

#else // DBG_SetAssertions

#undef DBG__internal_ASSERTIONS_ENABLED
#define DBG__internal_ASSERTIONS_ENABLED 0

#undef DBG__internal_Assert
#define DBG__internal_Assert(...)                                                                  \
  do {                                                                                             \
  } while (0)

#endif // DBG_SetAssertions
