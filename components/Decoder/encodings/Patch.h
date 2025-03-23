#include <stdint.h>

#include <string>
#include <unordered_map>

#include "components/Decoder/BitManip.hpp"
#include "components/Decoder/Instruction.hpp"
#include "components/Decoder/SemanticInstruction.hpp"


namespace nDecoder {

class InstrPatch {
public:
    typedef std::map<char, uint32_t> map;

    InstrPatch(const std::string &str): _str(str), _mask(0), _dest(0) {
        if (str.length() != 32) {
            fprintf(stderr, "ERROR: invalid instruction matcher string: %s\n", str.c_str());
            exit(1);
        }

        for (int i = 0; i < 32; i++) {
            int j = 31 - i;
            int b = 1 << j;

            switch (str[i]) {
                case '1':
                    _dest |= b;

                case '0':
                    _mask |= b;
                    break;

                default:
                    if (_map.find(str[i]) == _map.end())
                        _map.emplace(str[i], std::make_pair(j, j));
                    else
                        _map[str[i]].second = j;
            }
        }
    }

    bool hit(uint32_t raw) {
        return (raw & _mask) == _dest;
    }

    const uint32_t min() {
        return _dest;
    }

    const uint32_t max() {
        return _dest | ~_mask;
    }

    archinst dec(archcode const &opcode, uint32_t cpu, int64_t seq) {
        map map;

        for (const auto &f: _map)
            map[f.first] = extract32(opcode.theOpcode,
                                     f.second.second,
                                     f.second.first - f.second.second + 1);

        auto ins = new SemanticInstruction(opcode.thePC,
                                           opcode.theOpcode,
                                           opcode.theBPState,
                                           cpu,
                                           seq);

        dec(ins, map);

        return ins;
    }

    virtual void dec(SemanticInstruction *ins, const map &map) = 0;

private:
    std::string _str;
    std::unordered_map<char, std::pair<int, int>> _map;

    uint32_t _mask;
    uint32_t _dest;
};

void initPatch();

InstrPatch *hitPatch(uint32_t raw);

}