#if BOOST_PP_LESS_EQUAL(DBG__internal_tmp_MinimumSeverity, DBG_internal_Sev_to_int(Iface))
// Iface debugging enabled
#if (DBG__internal_tmp_MinimumSeverity == DBG_internal_Sev_to_int(Iface))
#define DBG__internal_MinimumSeverity DBG_internal_Sev_to_int(Iface)
#endif

#define DBG__internal_Iface(Sev, operations) DBG__internal_PROCESS_DBG(Sev, operations) /**/

#else
// Iface debugging disabled

#define DBG__internal_Iface(Sev, operations)                                                                           \
    do {                                                                                                               \
        /* Prevent unused variable warnings if debugging message is below threshold */                                 \
        if (false) { DBG__internal_PROCESS_DBG(Sev, operations); }                                                     \
    } while (0)

#endif
