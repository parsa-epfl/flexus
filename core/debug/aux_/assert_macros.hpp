#if DBG_SetAssertions
// Assertions enabled
#undef DBG__internal_ASSERTIONS_ENABLED
#define DBG__internal_ASSERTIONS_ENABLED 1

#undef DBG__internal_Assert
#define DBG__internal_Assert(sev, condition, operations)                                           \
  DBG__internal_PROCESS_DBG(sev, Assert() Condition(!(condition)) operations) /**/

#else // DBG_SetAssertions

// Assertions disabled

#undef DBG__internal_ASSERTIONS_ENABLED
#define DBG__internal_ASSERTIONS_ENABLED 0

#undef DBG__internal_Assert
#define DBG__internal_Assert(...)                                                                  \
  do {                                                                                             \
    /* Prevent unused variable warnings if assertions are disabled */                              \
    if (false) {                                                                                   \
      DBG__internal_PROCESS_DBG(sev, Assert() Condition(!(condition)) operations);                 \
    }                                                                                              \
  } while (0)

#endif // DBG_SetAssertions
