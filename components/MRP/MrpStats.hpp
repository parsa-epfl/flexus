namespace nMrpStats {

using namespace Flexus::Stat;

struct MrpStats {

  /*
    StatPredictionCounter PredExact;
    StatPredictionCounter PredMoreActual;
    StatPredictionCounter PredMorePredicted;
    StatPredictionCounter PredInexactMoreActual;
    StatPredictionCounter PredInexactMorePredicted;
    StatPredictionCounter PredDifferent;
    StatPredictionCounter PredOpposite;
  */

  StatPredictionCounter RealReaders_User;
  StatPredictionCounter RealHits_User;
  StatPredictionCounter RealTraining_User;
  StatPredictionCounter RealMisses_User;
  StatPredictionCounter DanglingMisses_User;

  StatPredictionCounter RealReaders_OS;
  StatPredictionCounter RealHits_OS;
  StatPredictionCounter RealTraining_OS;
  StatPredictionCounter RealMisses_OS;
  StatPredictionCounter DanglingMisses_OS;

#ifdef MRP_INSTANCE_COUNTERS
  StatInstanceCounter<int64_t> HitInstances;
  StatInstanceCounter<int64_t> TrainingInstances;
  StatInstanceCounter<int64_t> MissInstances;

#endif

  StatCounter ReadInitial;
  StatCounter ReadAgain;

  StatCounter Predict;
  StatCounter SigCreation;
  StatCounter SigEviction;
  StatCounter TrainingRead_User;
  StatCounter TrainingRead_OS;
  StatCounter TrainingWrite;
  StatCounter EvictedPred;
  StatCounter NoHistory;

  StatAnnotation Organization;

  MrpStats(std::string const & aName)
    :
/*     PredExact                ( aName + "-PredExact"               )
   , PredMoreActual           ( aName + "-PredMoreActual"          )
   , PredMorePredicted        ( aName + "-PredMorePredicted"       )
   , PredInexactMoreActual    ( aName + "-PredInexactMoreActual"   )
   , PredInexactMorePredicted ( aName + "-PredInexactMorePredicted")
   , PredDifferent            ( aName + "-PredDifferent"           )
   , PredOpposite             ( aName + "-PredOpposite"            )
*/
    RealReaders_User         ( aName + "-user-RealReaders"       )
    , RealHits_User            ( aName + "-user-RealHits"          )
    , RealTraining_User        ( aName + "-user-RealTraining"      )
    , RealMisses_User          ( aName + "-user-RealMisses"        )
    , DanglingMisses_User      ( aName + "-user-DanglingMisses"    )
    , RealReaders_OS           ( aName + "-OS-RealReaders"         )
    , RealHits_OS              ( aName + "-OS-RealHits"            )
    , RealTraining_OS          ( aName + "-OS-RealTraining"        )
    , RealMisses_OS            ( aName + "-OS-RealMisses"          )
    , DanglingMisses_OS        ( aName + "-OS-DanglingMisses"      )

#ifdef MRP_INSTANCE_COUNTERS
    , HitInstances             ( std::string("allOnly-") + aName + "-HitInstances"      )
    , TrainingInstances        ( std::string("allOnly-") + aName + "-TrainingInstances" )
    , MissInstances            ( std::string("allOnly-") + aName + "-MissInstances"     )
#endif //MRP_INSTANCE_COUNTERS

    , ReadInitial              ( aName + "-ReadInitial"             )
    , ReadAgain                ( aName + "-ReadAgain"               )
    , Predict                  ( aName + "-Predict"                 )
    , SigCreation              ( aName + "-SigCreation"             )
    , SigEviction              ( aName + "-SigEviction"             )
    , TrainingRead_User        ( aName + "-user-TrainingRead"       )
    , TrainingRead_OS          ( aName + "-OS-TrainingRead"         )
    , TrainingWrite            ( aName + "-TrainingWrite"           )
    , EvictedPred              ( aName + "-EvictedPred"             )
    , NoHistory                ( aName + "-NoHistory"               )
#ifdef MRP_INFINITE
    , Organization            ( aName + "-Organization", std::string("infinite"))
#else
    , Organization            ( aName + "-Organization", std::string("finite"))
#endif
  {}

};  // end struct MrpStats

} //end nMrpStats
