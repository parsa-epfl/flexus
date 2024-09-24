#if BOOST_PP_LESS_EQUAL(DBG__internal_tmp_MinimumSeverity, DBG_internal_Sev_to_int(Tmp))
// Tmp debugging enabled
#if (DBG__internal_tmp_MinimumSeverity == DBG_internal_Sev_to_int(Tmp))
#define DBG__internal_MinimumSeverity DBG_internal_Sev_to_int(Tmp)
#endif

#define DBG__internal_Tmp(Sev, operations) DBG__internal_PROCESS_DBG(Sev, operations) /**/

#else
// Tmp debugging disabled

#define DBG__internal_Tmp(Sev, operations)                                                                             \
    do {                                                                                                               \
        /* Prevent unused variable warnings if debugging operation is below threshold */                               \
        if (false) { DBG__internal_PROCESS_DBG(Sev, operations); }                                                     \
    } while (0)

#endif
