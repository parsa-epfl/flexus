set basedir $::env(HOME)/results
set specdir /home/parsacom/tools/flexus/global_job-specs
set userspecdir $env(HOME)/specs
set ckptdir /net/icfiler3/vol/units/parsa/parsacom/workloads/ckpts
set simicsdir /home/parsacom/tools/simics
set remote_system Condor
set gosh_presimics "# commands to run before executing simics"
set postprocess /home/parsacom/tools/flexus/postprocess.sh
lappend auto_path /home/parsacom/tools/tcllib/lib /home/parsacom/tools/tclx/lib

#### Condor example, creates one submission file for all jobs ####
set condorreqs "TRUE"
set remote_args_Condor {
	{reqs.arg {True} {Additional Condor requirements}}
}
set remote_exec_Condor {
	set submitfile $job_dir/.skel/submit.$jobspec.$flexstate
	set fd [open $submitfile "w"]
	puts $fd "Universe = vanilla
Getenv = True
Requirements = Mips > 700 && ($condorreqs) && ($reqs)
Executable = $job_dir/.skel/global/go.sh
Output = stdout
Error = stderr
Log = condor.log
Rank = 100000*(TARGET.TotalCpus - TARGET.TotalLoadAvg) + TARGET.Mips
Notification = error
Copy_To_Spool = False
### Transfer_Files used with old (<6.5.3) versions of Condor
#Transfer_Files = ONEXIT
Should_Transfer_Files = yes
When_To_Transfer_Output = ON_EXIT"
	foreach dir $run_dir_list {
		puts $fd "InitialDir = $dir
Args = [lrange [file split $dir] end-1 end]
Transfer_Input_Files = [join [glob -tails -directory $dir *] ,]
Queue\n"
	}
	close $fd
	if {!$norun} {
		puts -nonewline stderr "	Submitting to condor..."
		exec condor_submit $submitfile >> $job_dir/.skel/submit.log
	}
}

#### PBS example, creates individual submission file for each job ####
set pbsemail your@address.here
set remote_args_PBS {
	{pbstime.arg {04:00:00} {Time limit for PBS jobs hrs:min:sec}}
	{nopreempt {Disable preempt flag when submitting to PBS}}
	{jobemail.arg {$pbsemail} {Email address for PBS}}
}
set remote_exec_PBS {
	set preempt_opt ",qos=preempt"
	if {$nopreempt} {
		set preempt_opt ""
	}
	#Limits submissions to nodes with external IPs only	
	#PBS -l nodes=1:opt240:ppn=1,walltime=$pbstime$preempt_opt
	foreach dir $run_dir_list {
		set submitfile $dir/submit.$jobspec.$flexstate
		set fd [open $submitfile "w"]
		puts $fd "#!/bin/sh
#PBS -N $jobspec
#PBS -l nodes=1:ppn=1,pmem=3276mb,walltime=$pbstime$preempt_opt
#PBS -q route
#PBS -M $jobemail
#PBS -m a
#PBS -o $dir/stdout
#PBS -e $dir/stderr
#PBS -V
#
cd $dir
./go.sh
rm -rf $dir/flexus_state_in"
		close $fd
		if {!$norun} {
			puts -nonewline stderr "	Submitting to pbs..."
			exec qsub $submitfile >> $dir/submit.log
		}
	}
}
