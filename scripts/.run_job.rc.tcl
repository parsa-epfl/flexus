#func generate_job_dir_name {} {
# if {$interactive} { return $basedir/interactive-$config }
# return $basedir/$config-$simulator-%d%b%y-%H%M%S
#}

#func generate_run_dir_name {} {
# set name [file join $flexstate $phase $flexpoint]
# return $jobspec/[string map {/ _} $name]
#}

#set condorreqs {((Arch == "INTEL") || (Arch == "X86_64")) && (Disk != 0) && ((Memory * 1024) >= 1) && (Machine != "parsapc1.epfl.ch")}

set condorreqs { ((Arch == "X86_64") && (Disk != 0) && ((Memory * 1024) >= 1) && (Machine != "parsapc1.epfl.ch") && (Machine != "parsanode011"))}

array set jobspecmap {

	spec2k-int-1 { spec2k_bzip2 spec2k_crafty spec2k_eon spec2k_gap spec2k_gcc spec2k_gzip spec2k_mcf spec2k_parser spec2k_perlbmk spec2k_twolf spec2k_vortex spec2k_vpr }
	spec2k-fp-1 { spec2k_ammp spec2k_applu spec2k_apsi spec2k_art spec2k_equake spec2k_facerec spec2k_fma3d spec2k_galgel spec2k_lucas spec2k_mesa spec2k_mgrid spec2k_sixtrack spec2k_swim spec2k_wupwise }
	spec2k-1 { spec2k-int-1 spec2k-fp-1 }

	web-1   { apache_1cpu_20cl zeus_1cpu_20cl }
	web-4   { apache_4cpu_40cl2 zeus_4cpu_40cl }
	web-16  { apache_16cpu_40cl zeus_16cpu_40cl }
	oltp-1  { db2v8_tpcc_1cpu_64cl }
	oltp-4  { db2v8_tpcc_4cpu_64cl oracle_4cpu_16cl }
	oltp-16 { db2v8_tpcc_iosrv_16cpu_64cl oracle_16cpu_16cl }
	dss-1   { db2v8_tpch_qry2_1cpu db2v8_tpch_qry17_1cpu }
	dss-4   { db2v8_tpch_qry2_4cpu db2v8_tpch_qry17_4cpu }
	dss-16  { db2v8_tpch_qry2_16cpu db2v8_tpch_qry17_16cpu }
	sci-4   { em3d_4cpu_valop moldyn_4cpu_bitmap2 }
	sci-16  { em3d_large ocean_large }

	commercial-1  { web-1 oltp-1 dss-1 }
	commercial-4  { web-4 oltp-4 dss-4 }
	commercial-16 { web-16 oltp-16 dss-16 }

	all-1  { commercial-1 spec2k-1 }
	all-4  { commercial-4 sci-4 }
	all-16 { commercial-16 sci-16 }

	all { all-1 all-4 all-16 }
}
	# apache_16cpu_40cl2 is cmp (baseline65nm -state)
	#web-16  { apache_16cpu_40cl2 zeus_16cpu_40cl }

array set flexus_commands_trace {
        throughput_workloads {
flexus.set "-magic-break:stop_cycle" "200000000"
flexus.set-region-interval "100000000"
instruction-fetch-mode instruction-fetch-trace
        }
}



array set flexus_commands_flexpoint {
	sci {
flexus.set "-magic-break:iter" "1"
flexus.set "-magic-break:stop_cycle" "150000"
flexus.set-region-interval "50000"
flexus.set "-memory-map:round_robin" "false"
flexus.set "-memory-map:page_map" "false"
	}
	spec2k {
flexus.set "-magic-break:iter" "1"
flexus.set "-magic-break:stop_cycle" "150000"
flexus.set-region-interval "50000"
flexus.set "-memory-map:round_robin" "false"
flexus.set "-memory-map:page_map" "false"
	}
	tpcc {
flexus.set "-magic-break:stop_cycle" "50000000"
flexus.set-region-interval "50000"
flexus.set "-magic-break:trans_type" "0"
console_tracker.add-break-string "bash"
instruction-fetch-mode instruction-fetch-trace
	}
	commercial {
#flexus.set "-magic-break:stop_cycle" "10000000"
flexus.set "-magic-break:stop_cycle" "5000000"
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}
	tpch_long {
flexus.set "-magic-break:stop_cycle" "300000"
flexus.set-region-interval "100000"
	}








	zeus_16cpu_40cl {
flexus.set "-magic-break:stop_cycle" "24999999"
#server_cpu0.cycle-break 25000000 
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}

	zeus_4cpu_40cl {
#flexus.set "-magic-break:stop_cycle" "50000000"
server_cpu0.cycle-break 50000000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}

	db2v8_tpch_qry2_4cpu {
#flexus.set "-magic-break:stop_cycle" "10000000"
cpu0.cycle-break 10000000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}

	db2v8_tpch_qry2_16cpu {
#flexus.set "-magic-break:stop_cycle" "5000000"
cpu0.cycle-break 5000000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}

	db2v8_tpch_qry17_4cpu {
#flexus.set "-magic-break:stop_cycle" "10000000"
cpu0.cycle-break 10000000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}

	db2v8_tpch_qry17_16cpu {
#flexus.set "-magic-break:stop_cycle" "2000000"
cpu0.cycle-break 2000000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}

	apache_16cpu_40cl {
#flexus.set "-magic-break:stop_cycle" "25000000"
server_cpu0.cycle-break 25000000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}

	apache_16cpu_40cl2 {
#flexus.set "-magic-break:stop_cycle" "25000000"
server_cpu0.cycle-break 25000000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}

	apache_4cpu_40cl2 {
#flexus.set "-magic-break:stop_cycle" "50000000"
server_cpu0.cycle-break 50000000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}

        db2v8_tpcc_4cpu_64cl {
#flexus.set "-magic-break:stop_cycle" "50000000"
cpu0.cycle-break 50000000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
        }

	db2v8_tpcc_nort_16cpu_64cl {
#flexus.set "-magic-break:stop_cycle" "25000000"
cpu0.cycle-break 25000000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}

        oracle_4cpu_16cl {
#flexus.set "-magic-break:stop_cycle" "50000000"
cpu0.cycle-break 50000000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
        }

        oracle_16cpu_16cl {
#flexus.set "-magic-break:stop_cycle" "25000000"
cpu0.cycle-break 25000000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
        }

	flexus_test_app_v9 {
cpu0.cycle-break 9000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}

	flexus_test_app_x86 {
cpu0.cycle-break 9000
flexus.set-region-interval "50000"
instruction-fetch-mode instruction-fetch-trace
	}
}

array set flexus_commands_timing {
	common {
flexus.set "-magic-break:stop_cycle" "150000"
flexus.set-region-interval "50000"
	}
}

array set flexus_commands_phase {
        oracle_4cpu_16cl {
cpu0.cycle-break 1000000000
instruction-fetch-mode instruction-fetch-trace
        }
}


#foreach type {sci spec2k tpcc commercial tpch_long} {
#append flexus_commands($type) {
#flexus.set "-magic-break:stop_cycle" "15000"
#flexus.set-region-interval "5000"
#}
#}
rungen phase {
	{ oracle_4cpu_16cl baseline  0:0 $flexus_commands_phase(oracle_4cpu_16cl)  }
}

rungen trace {
        { apache_1cpu_20cl		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { apache_4cpu_40cl2		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { apache_16cpu_40cl		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { apache_16cpu_40cl2		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { apache_16cpu_40cl3		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { zeus_1cpu_20cl		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { zeus_4cpu_40cl		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { zeus_16cpu_40cl		baseline  0:0 $flexus_commands_trace(throughput_workloads) }

        { db2v8_tpcc_4cpu_64cl		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { db2v8_tpcc_nort_16cpu_64cl	baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { db2v8_tpcc_iosrv_16cpu_64cl	baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { oracle_4cpu_16cl		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { oracle_16cpu_16cl		baseline  0:0 $flexus_commands_trace(throughput_workloads) }

        { db2v8_tpch_qry2_4cpu		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { db2v8_tpch_qry17_1cpu		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { spec2k_art		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { db2v8_tpch_qry6_1cpu		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { db2v8_tpch_qry17_4cpu		baseline  0:0 $flexus_commands_trace(throughput_workloads) }

        { db2v8_tpch_qry2_16cpu		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { db2v8_tpch_qry17_16cpu	baseline  0:0 $flexus_commands_trace(throughput_workloads) }

        { em3d_4cpu_valop		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { moldyn_4cpu_bitmap2		baseline  0:0 $flexus_commands_trace(throughput_workloads) }

        { em3d_large			baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { ocean_large			tight     0:0 $flexus_commands_trace(throughput_workloads) }

        { flexus_test_app_v9		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
        { flexus_test_app_x86		baseline  0:0 $flexus_commands_trace(throughput_workloads) }
}

rungen flexpoint {
	{ spec2k_bzip2    baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_crafty   baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_eon      baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_gap      baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_gcc      baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_gzip     baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_mcf      baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_parser   baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_perlbmk  baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_twolf    baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_vortex   baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_vpr      baseline  0:0 $flexus_commands_flexpoint(spec2k) }

	{ spec2k_ammp     baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_applu    baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_apsi     baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_art      baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_equake   baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_facerec  baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_fma3d    baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_galgel   baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_lucas    baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_mesa     baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_mgrid    baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_sixtrack baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_swim     baseline  0:0 $flexus_commands_flexpoint(spec2k) }
	{ spec2k_wupwise  baseline  0:0 $flexus_commands_flexpoint(spec2k) }

	{ apache_1cpu_20cl   baseline  0:0 $flexus_commands_flexpoint(commercial)     }
	{ apache_4cpu_40cl2  baseline  0:0 $flexus_commands_flexpoint(apache_4cpu_40cl2)     }
	{ apache_16cpu_40cl  baseline  0:0 $flexus_commands_flexpoint(apache_16cpu_40cl)     }
	{ apache_16cpu_40cl2 baseline  0:0 $flexus_commands_flexpoint(apache_16cpu_40cl2)     }
	{ zeus_1cpu_20cl     baseline  0:0 $flexus_commands_flexpoint(commercial)     }
	{ zeus_4cpu_40cl     baseline  0:0 $flexus_commands_flexpoint(zeus_4cpu_40cl)     }
	{ zeus_16cpu_40cl    baseline  0:0 $flexus_commands_flexpoint(zeus_16cpu_40cl)     }

	{ db2v8_tpcc_4cpu_64cl        baseline  0:0 $flexus_commands_flexpoint(db2v8_tpcc_4cpu_64cl)  }
	{ db2v8_tpcc_nort_16cpu_64cl  baseline  0:0 $flexus_commands_flexpoint(db2v8_tpcc_nort_16cpu_64cl) }
	{ db2v8_tpcc_iosrv_16cpu_64cl baseline  0:0 $flexus_commands_flexpoint(tpcc)  }
	{ oracle_4cpu_16cl            baseline  0:0 $flexus_commands_flexpoint(oracle_4cpu_16cl)  }
	{ oracle_16cpu_16cl           baseline  0:0 $flexus_commands_flexpoint(oracle_16cpu_16cl)  }

	{ db2v8_tpch_qry2_4cpu   baseline  0:0 $flexus_commands_flexpoint(db2v8_tpch_qry2_4cpu) }
	{ db2v8_tpch_qry17_4cpu  baseline  0:0 $flexus_commands_flexpoint(db2v8_tpch_qry17_4cpu) }

	{ db2v8_tpch_qry2_16cpu  baseline       0:0 $flexus_commands_flexpoint(db2v8_tpch_qry2_16cpu) }
	{ db2v8_tpch_qry17_16cpu baseline  0:0 $flexus_commands_flexpoint(db2v8_tpch_qry17_16cpu) }

	{ em3d_4cpu_valop        baseline  0:0 $flexus_commands_flexpoint(sci)        }
	{ moldyn_4cpu_bitmap2    baseline  0:0 $flexus_commands_flexpoint(sci)        }

	{ em3d_large             baseline  0:0 $flexus_commands_flexpoint(sci)        }
	{ ocean_large            tight     0:0 $flexus_commands_flexpoint(sci)        }

	{ flexus_test_app_v9	baseline	0:0 $flexus_commands_flexpoint(flexus_test_app_v9)	}
	{ flexus_test_app_x86	baseline	0:0 $flexus_commands_flexpoint(flexus_test_app_x86)	}
}

rungen timing {
	{ spec2k_bzip2    baseline  x:x $flexus_commands_timing(common) }
	{ spec2k_crafty   baseline  x:x $flexus_commands_timing(common) }
	{ spec2k_eon      baseline  x:x $flexus_commands_timing(common) }
	{ spec2k_gcc      baseline  x:x $flexus_commands_timing(common) }
	{ spec2k_gzip     baseline  x:x $flexus_commands_timing(common) }
	{ spec2k_mcf      baseline  x:x $flexus_commands_timing(common) }
	{ spec2k_perlbmk  baseline  x:x $flexus_commands_timing(common) }
	{ spec2k_twolf    baseline  x:x $flexus_commands_timing(common) }
	{ spec2k_vortex   baseline  x:x $flexus_commands_timing(common) }
	{ spec2k_vpr      baseline  x:x $flexus_commands_timing(common) }

	{ spec2k_art      baseline  x:x $flexus_commands_timing(common) }
	{ spec2k_galgel   baseline  x:x $flexus_commands_timing(common) }
	{ spec2k_mgrid    baseline  x:x $flexus_commands_timing(common) }
	{ spec2k_sixtrack baseline  x:x $flexus_commands_timing(common) }
	{ apache_1cpu_20cl   baseline   x:x    $flexus_commands_timing(common)   }
	{ apache_4cpu_40cl2  baseline 0-7:5-24 $flexus_commands_timing(common)   }
	{ apache_16cpu_40cl  baseline 0-1:5-24 $flexus_commands_timing(common)   }
	{ apache_16cpu_40cl2 baseline 0-1:5-24 $flexus_commands_timing(common)   }
	{ zeus_1cpu_20cl     baseline   x:x    $flexus_commands_timing(common)   }
	{ zeus_4cpu_40cl     baseline 0-7:5-19 $flexus_commands_timing(common)   }
	{ zeus_16cpu_40cl    baseline 0-1:5-24 $flexus_commands_timing(common)   }

	{ db2v8_tpcc_4cpu_64cl        baseline 0-10:5-24 $flexus_commands_timing(common)  }
	{ db2v8_tpcc_nort_16cpu_64cl  baseline 0-1:1-24 $flexus_commands_timing(common) }
	{ db2v8_tpcc_iosrv_16cpu_64cl baseline 0-1:5-24 $flexus_commands_timing(common)  }
	{ oracle_4cpu_16cl            baseline 0-7:1-9 $flexus_commands_timing(common)  }
	{ oracle_16cpu_16cl           baseline 5,7:5-24 $flexus_commands_timing(common)  }

	{ db2v8_tpch_qry2_4cpu   baseline  0:1-117 $flexus_commands_timing(common) }
	{ db2v8_tpch_qry17_4cpu  baseline  0:1-120 $flexus_commands_timing(common) }

	{ db2v8_tpch_qry2_16cpu  baseline    0:1-67 $flexus_commands_timing(common)  }
	{ db2v8_tpch_qry17_16cpu baseline  0:1-175 $flexus_commands_timing(common) }

	{ em3d_4cpu_valop        baseline  0:1-30 $flexus_commands_timing(common)        }
	{ moldyn_4cpu_bitmap2    baseline  0:1-50 $flexus_commands_timing(common)        }

	{ em3d_large             baseline  0:1-28 $flexus_commands_timing(common)        }
	{ ocean_large            tight     0:1-77 $flexus_commands_timing(common)        }

	{ flexus_test_app_v9	baseline	0:18-28 $flexus_commands_timing(common)	}
	{ flexus_test_app_x86	baseline	0:12-24 $flexus_commands_timing(common)	}
}
