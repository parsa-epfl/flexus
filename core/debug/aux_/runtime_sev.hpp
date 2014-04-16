struct DBG__SetRuntimeMinSev {
  DBG__SetRuntimeMinSev() {
    Flexus::Dbg::Debugger::debugger().setMinSev(Flexus::Dbg::tSeverity::Severity(  DBG_internal_Sev_to_int( DBG_SetInitialRuntimeMinSev  ) ) );
  }

} DBG__theSetRuntimeMinSev;

