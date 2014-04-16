#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT DirTest
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/DirectoryTransport.hpp>

COMPONENT_NO_PARAMETERS ;

/*
COMPONENT_INTERFACE(
  PORT(  PushOutput, DirectoryTransport, RequestOut )
  PORT(  PushInput,  DirectoryTransport, ResponseIn )
  DRIVE( DirTestDrive )
);
*/

struct DirTestJumpTable;
struct DirTestInterface : public Flexus::Core::ComponentInterface {
  struct RequestOut {
    typedef Flexus::SharedTypes::DirectoryTransport payload;
    typedef Flexus::Core::aux_::push port_type;
    static const bool is_array = false;
  };

  struct ResponseIn {
    typedef Flexus::SharedTypes::DirectoryTransport payload;
    typedef Flexus::Core::aux_::push port_type;
    static const bool is_array = false;
  };
  virtual void push(ResponseIn const &, ResponseIn::payload &) = 0;
  virtual bool available(ResponseIn const &) = 0;

  struct DirTestDrive {
    static std::string name() {
      return "DirTestInterface::DirTestDrive";
    }
  };
  virtual void drive(DirTestDrive const & ) = 0;

  static DirTestInterface * instantiate(DirTestConfiguration::cfg_struct_ & aCfg, DirTestJumpTable & aJumpTable, Flexus::Core::index_t anIndex, Flexus::Core::index_t aWidth);
  typedef DirTestConfiguration_struct configuration;
  typedef DirTestJumpTable jump_table;

};

struct DirTestJumpTable {
  typedef DirTestInterface iface;
  bool (*wire_available_RequestOut)(Flexus::Core::index_t anIndex);
  void (*wire_manip_RequestOut)(Flexus::Core::index_t anIndex, iface::RequestOut::payload & aPayload);
  DirTestJumpTable() {
    wire_available_RequestOut = 0;
    wire_manip_RequestOut = 0;
  }
  void check(std::string const & anInstance) {
    if ( wire_available_RequestOut == 0 ) {
      DBG_(Crit, ( << anInstance << " port " "RequestOut" " is not wired" ) );
    }
  }
};

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT DirTest
