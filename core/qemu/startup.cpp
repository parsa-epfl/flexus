#include <boost/algorithm/string.hpp>
#include <boost/version.hpp>
#include <core/component.hpp>
#include <core/configuration.hpp>
#include <core/debug/debug.hpp>
#include <core/flexus.hpp>
#include <core/simulator_name.hpp>
#include <core/target.hpp>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#define QEMUFLEX_FLEXUS_INTERNAL
namespace Flexus {
namespace Qemu {
namespace API {
#include <core/qemu/api.h>
} // namespace API
} // namespace Qemu
} // namespace Flexus

#include <fstream>

// For debug purposes
#include <iostream>

namespace Flexus {

std::string oldcwd;

namespace Core {
void
CreateFlexusObject();
void
PrepareFlexusObject();
void
initFlexus();
void
deinitFlexus();
void
setCfg(const char* aFile);
} // namespace Core

} // end namespace Flexus

namespace {

using std::cerr;
using std::endl;

// clang-format off
void print_copyright() {

  cerr << "////////////////////////////////////////////////////////////////////////////////////////////////////////" << endl;
  cerr << "//                                                                                                    //" << endl;
  cerr << "//           ************                                                                             //" << endl;
  cerr << "//          **************                                                                            //" << endl;
  cerr << "//   *      **************      *                                                                     //" << endl;
  cerr << "//  **      ***  ****  ***     ***                                                                    //" << endl;
  cerr << "// ***      **************      ***                                                                   //" << endl;
  cerr << "// ****     **************     ****                                                                   //" << endl;
  cerr << "//  ******************************   ***********    *                                                 //" << endl;
  cerr << "//    *********        **********   *************  ***                                                //" << endl;
  cerr << "//      *****            *****      ***            ***       *****      *        *                    //" << endl;
  cerr << "//      ****              ****      ***            ***    ***********  ****    ****                   //" << endl;
  cerr << "//      ****               ***      **********     ***   ****     ****   ********                     //" << endl;
  cerr << "//      ****          *** ****      **********     ***   *************     ****                       //" << endl;
  cerr << "//       ****          ******       ***            ***   ***             ********                     //" << endl;
  cerr << "//        *****        *******      ***            ***    ****   ****   ****  ****                    //" << endl;
  cerr << "//           ****    ****   ***     ***            ***      ********   ***      ***                   //" << endl;
  cerr << "//                                                                                                    //" << endl;
  cerr << "//   QFlex (C) 2016-2020, Parallel Systems Architecture Lab, EPFL                                     //" << endl;
  cerr << "//   All rights reserved.                                                                             //" << endl;
  cerr << "//   Website: https://qflex.epfl.ch                                                                   //" << endl;
  cerr << "//   QFlex uses software developed externally:                                                        //" << endl;
  cerr << "//   [NS-3](https://www.gnu.org/copyleft/gpl.html)                                                    //" << endl;
  cerr << "//   [QEMU](http://wiki.qemu.org/License)                                                             //" << endl;
  cerr << "//   [SimFlex] (http://parsa.epfl.ch/simflex/)                                                        //" << endl;
  cerr << "//                                                                                                    //" << endl;
  cerr << "//   Redistribution and use in source and binary forms, with or without modification,                 //" << endl;
  cerr << "//   are permitted provided that the following conditions are met:                                    //" << endl;
  cerr << "//                                                                                                    //" << endl;
  cerr << "//       * Redistributions of source code must retain the above copyright notice,                     //" << endl;
  cerr << "//         this list of conditions and the following disclaimer.                                      //" << endl;
  cerr << "//       * Redistributions in binary form must reproduce the above copyright notice,                  //" << endl;
  cerr << "//         this list of conditions and the following disclaimer in the documentation                  //" << endl;
  cerr << "//         and/or other materials provided with the distribution.                                     //" << endl;
  cerr << "//       * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,                    //" << endl;
  cerr << "//         nor the names of its contributors may be used to endorse or promote                        //" << endl;
  cerr << "//         products derived from this software without specific prior written                         //" << endl;
  cerr << "//         permission.                                                                                //" << endl;
  cerr << "//                                                                                                    //" << endl;
  cerr << "//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND                //" << endl;
  cerr << "//   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                    //" << endl;
  cerr << "//   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                           //" << endl;
  cerr << "//   DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,                      //" << endl;
  cerr << "//   EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR                      //" << endl;
  cerr << "//   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE                  //" << endl;
  cerr << "//   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                      //" << endl;
  cerr << "//   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT                       //" << endl;
  cerr << "//   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF                 //" << endl;
  cerr << "//   THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                     //" << endl;
  cerr << "////////////////////////////////////////////////////////////////////////////////////////////////////////" << endl << endl << endl;;
  cerr << "//   QFlex simulator - Built as " << Flexus::theSimulatorName << endl << endl;
}
// clang-format on

extern "C"
{

    void flexus_init(Flexus::Qemu::API::QEMU_API_t* qemu,
                     Flexus::Qemu::API::FLEXUS_API_t* flexus,
                     uint32_t ncores,
                     const char* cfg,
                     const char* dbg,
                     Flexus::Qemu::API::cycles_opts cycles,
                     const char* cwd)
    {
        auto oldcwd = get_current_dir_name();

        Flexus::oldcwd = oldcwd;

        free(oldcwd);

        Flexus::Qemu::API::qemu_api = *qemu;
        Flexus::Qemu::API::FLEXUS_get_api(flexus);

        print_copyright();

        if (getenv("WAITFORSIGCONT")) {
            std::cerr << "Waiting for SIGCONT..." << std::endl;
            std::cerr << "Attach gdb with the following command and 'c' from the gdb prompt:" << std::endl;
            std::cerr << "  gdb - " << getpid() << std::endl;
            raise(SIGSTOP);
        }

        DBG_(Dev, (<< "Initializing Flexus."));
        DBG_(Dev,
             (<< "Compiled with Boost: " << BOOST_VERSION / 100000 << "." << BOOST_VERSION / 100 % 1000 << "."
              << BOOST_VERSION % 100));

        Flexus::Core::setCfg(cfg);
        Flexus::Core::PrepareFlexusObject();
        Flexus::Core::CreateFlexusObject();

        if (cwd) {
            cerr << "Switching to directory: " << cwd << endl;
            [[maybe_unused]] int x = chdir(cwd);
        }

        Flexus::Dbg::Debugger::theDebugger->initialize();

        Flexus::Core::theFlexus->setStopCycle(cycles.until_stop);
        Flexus::Core::theFlexus->setStatInterval(cycles.stats_interval);
        Flexus::Core::theFlexus->set_log_delay(cycles.log_delay);

        if (dbg) Flexus::Core::theFlexus->setDebug(dbg);


        Flexus::Core::ComponentManager::getComponentManager().instantiateComponents(ncores);

        // backdoor
        char* args = getenv("FLEXUS_EXTRA_ARGS");

        if (args) {
            std::string line(args);
            std::vector<std::string> strs;

            boost::split(strs, line, boost::is_any_of(" \t\""), boost::token_compress_on);

            auto& manager = Flexus::Core::ConfigurationManager::getConfigurationManager();

            for (auto it = strs.begin(); it != strs.end();) {
                if (it->c_str()[0] == '-') {
                    auto& key = *it++;

                    if (it != strs.end()) {
                        auto& val = *it++;
                        manager.set(key, val);
                    } else
                        break;

                } else
                    it++;
            }
        }

        Flexus::Core::initFlexus();

        DBG_(Iface, (<< "Flexus Initialized."));
    }

    void flexus_deinit(void)
    {
        Flexus::Core::deinitFlexus();
    }
}
}
