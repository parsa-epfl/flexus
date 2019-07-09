#if BOOST_PP_LESS_EQUAL(DBG__internal_tmp_MinimumSeverity, DBG_internal_Sev_to_int(Iface))
// Crit debugging enabled
#if (DBG__internal_tmp_MinimumSeverity == DBG_internal_Sev_to_int(Iface))
#define DBG__internal_MinimumSeverity DBG_internal_Sev_to_int(Iface)
#endif

#define DBG__internal_Iface(Sev, operations) DBG__internal_PROCESS_DBG(Sev, operations) /**/

#else
// Crit debugging disabled

#define DBG__internal_Iface(...)                                                                   \
  do {                                                                                             \
  } while (0)

#endif
