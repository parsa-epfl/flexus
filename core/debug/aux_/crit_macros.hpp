#if BOOST_PP_LESS_EQUAL(DBG__internal_tmp_MinimumSeverity, DBG_internal_Sev_to_int(Crit))
// Crit debugging enabled
#if (DBG__internal_tmp_MinimumSeverity == DBG_internal_Sev_to_int(Crit))
#define DBG__internal_MinimumSeverity DBG_internal_Sev_to_int(Crit)
#endif

#define DBG__internal_Crit(Sev, operations) DBG__internal_PROCESS_DBG(Sev, operations) /**/

#else
// Crit debugging disabled

#define DBG__internal_Crit(Sev, operations)                                                        \
  do {                                                                                             \
    /* Prevent unused variable warnings if debugging operation is below threshold */               \
    if (false) {                                                                                   \
      DBG__internal_PROCESS_DBG(Sev, operations);                                                  \
    }                                                                                              \
  } while (0)

#endif
