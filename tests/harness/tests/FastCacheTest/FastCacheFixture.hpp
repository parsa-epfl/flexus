class FastCacheTestFixture : public testing::Test {

private:
protected:
  FastCacheTestFixture();
  void SetUp() override;
  void TearDown() override;

public:
  static void InitializeFastCacheConfiguration(
      FastCacheConfiguration_struct &aCfg, int MTWidth, int Size, int Associativity, int BlockSize,
      bool CleanEvictions, Flexus::SharedTypes::tFillLevel CacheLevel, bool NotifyReads,
      bool NotifyWrites, bool TraceTracker, int RegionSize, int RTAssoc, int RTSize,
      std::string RTReplPolicy, int ERBSize, bool StdArray, bool BlockScout, bool SkewBlockSet,
      std::string Protocol, bool UsingTraces, bool TextFlexpoints, bool GZipFlexpoints,
      bool DowngradeLRU, bool SnoopLRU);

  static void InitializeTLBRequest(Flexus::SharedTypes::TranslationPtr &tlbRequest,
                                   unsigned int Vaddr, unsigned int Paddr);

  static void InitializeJumpTable(FastCacheJumpTable &aJumpTable);

  static bool func_wire_available_RequestOut(Flexus::Core::index_t idx);
  static void func_wire_manip_RequestOut(Flexus::Core::index_t idx, MemoryMessage &p);
  static bool func_wire_available_SnoopOutI(Flexus::Core::index_t idx);
  static void func_wire_manip_SnoopOutI(Flexus::Core::index_t idx, MemoryMessage &p);
  static bool func_wire_available_SnoopOutD(Flexus::Core::index_t idx);
  static void func_wire_manip_SnoopOutD(Flexus::Core::index_t idx, MemoryMessage &p);
  static bool func_wire_available_Reads(Flexus::Core::index_t idx);
  static void func_wire_manip_Reads(Flexus::Core::index_t idx, MemoryMessage &p);
  static bool func_wire_available_Writes(Flexus::Core::index_t idx);
  static void func_wire_manip_Writes(Flexus::Core::index_t idx, MemoryMessage &p);
  static bool func_wire_available_RegionNotify(Flexus::Core::index_t idx);
  static void func_wire_manip_RegionNotify(Flexus::Core::index_t idx, RegionScoutMessage &p);
};
