#if BOOST_PP_LESS_EQUAL(DBG__internal_tmp_MinimumSeverity, DBG_internal_Sev_to_int(Dev))
// Dev debugging enabled
#if (DBG__internal_tmp_MinimumSeverity == DBG_internal_Sev_to_int(Dev))
#define DBG__internal_MinimumSeverity DBG_internal_Sev_to_int(Dev)
#endif

#define DBG__internal_Dev(Sev, operations) DBG__internal_PROCESS_DBG(Sev, operations) /**/

#else
// Dev debugging disabled

#define DBG__internal_Dev(Sev, operations)                                                                             \
    do {                                                                                                               \
        /* Prevent unused variable warnings if debugging operation is below threshold */                               \
        if (false) { DBG__internal_PROCESS_DBG(Sev, operations); }                                                     \
    } while (0)

#endif
