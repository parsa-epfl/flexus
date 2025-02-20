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



std::string license_text = R"(
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃                                                                                           ┃
┃                                                                                           ┃
┃                ************                                                               ┃
┃               **************                                                              ┃
┃        *      **************      *                                                       ┃
┃       **      ***  ****  ***     ***                                                      ┃
┃      ***      **************      ***                                                     ┃
┃      ****     **************     ****                                                     ┃
┃       ******************************   ***********    *                                   ┃
┃         *********        **********   *************  ***                                  ┃
┃           *****            *****      ***            ***       *****      *        *      ┃
┃           ****              ****      ***            ***    ***********  ****    ****     ┃
┃           ****               ***      **********     ***   ****     ****   ********       ┃
┃           ****          *** ****      **********     ***   *************     ****         ┃
┃            ****          ******       ***            ***   ***             ********       ┃
┃             *****        *******      ***            ***    ****   ****   ****  ****      ┃
┃                ****    ****   ***     ***            ***      ********   ***      ***     ┃
┃                                                                                           ┃
┃                                                                                           ┃
┃                                                                                           ┃
┃                                                                                           ┃
┃     ***Software developed externally (not by the QFlex group)***                          ┃
┃                                                                                           ┃
┃     QFlex consists of several software components that are governed by various            ┃
┃     licensing terms, in addition to software that was developed internally.               ┃
┃     Anyone interested in using QFlex needs to fully understand and abide by the           ┃
┃     licenses governing all the software components.                                       ┃
┃                                                                                           ┃
┃       * [QEMU] (https://wiki.qemu.org/License)                                            ┃
┃       * [Boost] (https://www.boost.org/users/license.html)                                ┃
┃       * [Conan] (https://github.com/conan-io/conan/blob/develop2/LICENSE.md)              ┃
┃                                                                                           ┃
┃     **QFlex License**                                                                     ┃
┃                                                                                           ┃
┃     QFlex                                                                                 ┃
┃     Copyright (c) 2025, Parallel Systems Architecture Lab, EPFL                           ┃
┃     All rights reserved.                                                                  ┃
┃                                                                                           ┃
┃     Redistribution and use in source and binary forms, with or without modification,      ┃
┃     are permitted provided that the following conditions are met:                         ┃
┃                                                                                           ┃
┃      * Redistributions of source code must retain the above copyright notice,             ┃
┃        this list of conditions and the following disclaimer.                              ┃
┃      * Redistributions in binary form must reproduce the above copyright notice,          ┃
┃        this list of conditions and the following disclaimer in the documentation          ┃
┃        and/or other materials provided with the distribution.                             ┃
┃      * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,            ┃
┃        nor the names of its contributors may be used to endorse or promote                ┃
┃        products derived from this software without specific prior written                 ┃
┃        permission.                                                                        ┃
┃                                                                                           ┃
┃     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND       ┃
┃     ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED         ┃
┃     WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                ┃
┃     DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,           ┃
┃     EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           ┃
┃     CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE       ┃
┃     GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)           ┃
┃     HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT            ┃
┃     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF      ┃
┃     THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          ┃
┃                                                                                           ┃
┃                                                                                           ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
)";




extern "C"
{

    void flexus_init(Flexus::Qemu::API::QEMU_API_t* qemu,
                     Flexus::Qemu::API::FLEXUS_API_t* flexus,
                     uint32_t ncores,
                     const char* cfg,
                     const char* dbg,
                     Flexus::Qemu::API::cycles_opts cycles,
                     const char* freq,
                     const char* cwd)
    {
        auto oldcwd = get_current_dir_name();

        Flexus::oldcwd = oldcwd;

        free(oldcwd);

        Flexus::Qemu::API::qemu_api = *qemu;
        Flexus::Qemu::API::FLEXUS_get_api(flexus);

        std::cout << license_text;

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
            std::cerr << "Switching to directory: " << cwd << std::endl;
            [[maybe_unused]] int x = chdir(cwd);
        }

        Flexus::Dbg::Debugger::theDebugger->initialize();

        Flexus::Core::theFlexus->setStopCycle(cycles.until_stop);
        Flexus::Core::theFlexus->setStatInterval(cycles.stats_interval);
        Flexus::Core::theFlexus->set_log_delay(cycles.log_delay);

        if (dbg) Flexus::Core::theFlexus->setDebug(dbg);


        Flexus::Core::ComponentManager::getComponentManager().instantiateComponents(ncores, freq);

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

        DBG_(VVerb, (<< "Flexus Initialized."));
    }

    void flexus_deinit(void)
    {
        Flexus::Core::deinitFlexus();
    }
}
}
