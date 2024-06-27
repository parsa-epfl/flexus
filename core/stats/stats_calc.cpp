//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#include <boost/spirit/include/classic_core.hpp>
// #include <boost/spirit/core.hpp>
#include <core/stats.hpp>
#include <functional>
#include <iostream>
#include <stack>
#include <string>

namespace Flexus {
namespace Stat {
namespace aux_ {

using namespace boost::spirit::classic;
using namespace std;

struct push_float
{
    push_float(stack<float>& eval_)
      : eval(eval_)
    {
    }

    void operator()(float val) const { eval.push(val); }

    stack<float>& eval;
};

template<typename op>
struct do_op
{
    do_op(op const& the_op, stack<float>& eval_)
      : m_op(the_op)
      , eval(eval_)
    {
    }

    void operator()(char const*, char const*) const
    {
        float rhs = eval.top();
        eval.pop();
        float lhs = eval.top();
        eval.pop();

        // cout << "popped " << lhs << " and " << rhs << " from the stack. ";
        // cout << "pushing " << m_op(lhs, rhs) << " onto the stack.\n";
        eval.push(m_op(lhs, rhs));
    }

    op m_op;
    stack<float>& eval;
};

template<class op>
do_op<op>
make_op(op const& the_op, stack<float>& eval)
{
    return do_op<op>(the_op, eval);
}

struct push_ci
{

    stack<float>& eval;
    std::map<std::string, Measurement*>& msmts;
    std::string const& defaults;

    push_ci(stack<float>& eval_, std::map<std::string, Measurement*>& msmts_, std::string const& defaults_)
      : eval(eval_)
      , msmts(msmts_)
      , defaults(defaults_)
    {
    }

    void operator()(char const* aStart, char const* anEnd) const
    {
        std::string aField(aStart, anEnd);

        SimpleMeasurement* avg   = dynamic_cast<SimpleMeasurement*>(msmts["avg"]);
        SimpleMeasurement* count = dynamic_cast<SimpleMeasurement*>(msmts["count"]);
        SimpleMeasurement* stdev = dynamic_cast<SimpleMeasurement*>(msmts["stdev"]);

        if (avg && count && stdev) {
            float val = 1.96 * stdev->asDouble(aField) / avg->asDouble(aField) / sqrt(count->asDouble(aField));
            eval.push(val);
        } else {
            std::cerr << "{ERR: cannot locate sampling measurements for ci calculation}" << std::endl;
        }
    }
};

struct push_field
{

    stack<float>& eval;
    std::map<std::string, Measurement*>& msmts;
    std::string const& defaults;

    push_field(stack<float>& eval_, std::map<std::string, Measurement*>& msmts_, std::string const& defaults_)
      : eval(eval_)
      , msmts(msmts_)
      , defaults(defaults_)
    {
    }

    void operator()(char const* aStart, char const* anEnd) const
    {
        std::string aField(aStart, anEnd);
        SimpleMeasurement* selected;
        std::string::size_type at = aField.find('@');
        if (at != std::string::npos) {
            std::string msmt(aField, at + 1);
            aField   = aField.substr(0, at);
            selected = dynamic_cast<SimpleMeasurement*>(msmts[msmt]);
            if (!selected) { std::cerr << "{ERR: request for non-existant measurement: " << msmt << '}' << std::endl; }
        } else {
            selected = dynamic_cast<SimpleMeasurement*>(msmts[defaults]);
        }
        if (selected) {
            float val = selected->asDouble(aField);
            eval.push(val);
        }
    }
};

struct push_sum
{

    stack<float>& eval;
    std::map<std::string, Measurement*>& msmts;
    std::string const& defaults;

    push_sum(stack<float>& eval_, std::map<std::string, Measurement*>& msmts_, std::string const& defaults_)
      : eval(eval_)
      , msmts(msmts_)
      , defaults(defaults_)
    {
    }

    void operator()(char const* aStart, char const* anEnd) const
    {
        std::string aField(aStart, anEnd);
        SimpleMeasurement* selected;
        std::string::size_type at = aField.find('@');
        if (at != std::string::npos) {
            std::string msmt(aField, at + 1);
            aField   = aField.substr(0, at);
            selected = dynamic_cast<SimpleMeasurement*>(msmts[msmt]);
            if (!selected) { std::cerr << "{ERR: request for non-existant measurement: " << msmt << '}' << std::endl; }
        } else {
            selected = dynamic_cast<SimpleMeasurement*>(msmts[defaults]);
        }
        if (selected) {
            float val = selected->sumAsLongLong(aField);
            eval.push(val);
        }
    }
};

struct push_min
{

    stack<float>& eval;
    std::map<std::string, Measurement*>& msmts;
    std::string const& defaults;

    push_min(stack<float>& eval_, std::map<std::string, Measurement*>& msmts_, std::string const& defaults_)
      : eval(eval_)
      , msmts(msmts_)
      , defaults(defaults_)
    {
    }

    void operator()(char const* aStart, char const* anEnd) const
    {
        std::string aField(aStart, anEnd);
        SimpleMeasurement* selected;
        std::string::size_type at = aField.find('@');
        if (at != std::string::npos) {
            std::string msmt(aField, at + 1);
            aField   = aField.substr(0, at);
            selected = dynamic_cast<SimpleMeasurement*>(msmts[msmt]);
            if (!selected) { std::cerr << "{ERR: request for non-existant measurement: " << msmt << '}' << std::endl; }
        } else {
            selected = dynamic_cast<SimpleMeasurement*>(msmts[defaults]);
        }
        if (selected) {
            float val = selected->minAsLongLong(aField);
            eval.push(val);
        }
    }
};

struct push_max
{
    stack<float>& eval;
    std::map<std::string, Measurement*>& msmts;
    std::string const& defaults;

    push_max(stack<float>& eval_, std::map<std::string, Measurement*>& msmts_, std::string const& defaults_)
      : eval(eval_)
      , msmts(msmts_)
      , defaults(defaults_)
    {
    }

    void operator()(char const* aStart, char const* anEnd) const
    {
        std::string aField(aStart, anEnd);
        SimpleMeasurement* selected;
        std::string::size_type at = aField.find('@');
        if (at != std::string::npos) {
            std::string msmt(aField, at + 1);
            aField   = aField.substr(0, at);
            selected = dynamic_cast<SimpleMeasurement*>(msmts[msmt]);
            if (!selected) { std::cerr << "{ERR: request for non-existant measurement: " << msmt << '}' << std::endl; }
        } else {
            selected = dynamic_cast<SimpleMeasurement*>(msmts[defaults]);
        }
        if (selected) {
            float val = selected->maxAsLongLong(aField);
            eval.push(val);
        }
    }
};

struct push_avg
{

    stack<float>& eval;
    std::map<std::string, Measurement*>& msmts;
    std::string const& defaults;

    push_avg(stack<float>& eval_, std::map<std::string, Measurement*>& msmts_, std::string const& defaults_)
      : eval(eval_)
      , msmts(msmts_)
      , defaults(defaults_)
    {
    }

    void operator()(char const* aStart, char const* anEnd) const
    {
        std::string aField(aStart, anEnd);
        SimpleMeasurement* selected;
        std::string::size_type at = aField.find('@');
        if (at != std::string::npos) {
            std::string msmt(aField, at + 1);
            aField   = aField.substr(0, at);
            selected = dynamic_cast<SimpleMeasurement*>(msmts[msmt]);
            if (!selected) { std::cerr << "{ERR: request for non-existant measurement: " << msmt << '}' << std::endl; }
        } else {
            selected = dynamic_cast<SimpleMeasurement*>(msmts[defaults]);
        }
        if (selected) {
            float val = selected->avgAsDouble(aField);
            eval.push(val);
        }
    }
};

struct do_negate
{
    do_negate(stack<float>& eval_)
      : eval(eval_)
    {
    }

    void operator()(char const*, char const*) const
    {
        float lhs = eval.top();
        eval.pop();

        eval.push(-lhs);
    }

    stack<float>& eval;
};

struct do_sqrt
{
    do_sqrt(stack<float>& eval_)
      : eval(eval_)
    {
    }

    void operator()(char const*, char const*) const
    {
        float lhs = eval.top();
        eval.pop();

        eval.push(sqrt(lhs));
    }

    stack<float>& eval;
};

struct calculator : public grammar<calculator>
{
    calculator(stack<float>& eval_,
               std::map<std::string, Measurement*>& measurements,
               std::string const& default_measurement)
      : eval(eval_)
      , theMeasurements(measurements)
      , theDefault(default_measurement)
    {
    }

    stack<float>& eval;
    std::map<std::string, Measurement*>& theMeasurements;
    std::string const& theDefault;

    template<typename ScannerT>
    struct definition
    {
        definition(calculator const& self)
        {
            real = real_p[push_float(self.eval)];

            field = ch_p('{') >>
                    lexeme_d[*(~ch_p('}'))][push_field(self.eval, self.theMeasurements, self.theDefault)] >> ch_p('}');

            ci = str_p("ci95{") >> lexeme_d[*(~ch_p('}'))][push_ci(self.eval, self.theMeasurements, self.theDefault)] >>
                 ch_p('}');

            sum = str_p("sum{") >>
                  lexeme_d[*(~ch_p('}'))][push_sum(self.eval, self.theMeasurements, self.theDefault)] >> ch_p('}');

            min = str_p("min{") >>
                  lexeme_d[*(~ch_p('}'))][push_min(self.eval, self.theMeasurements, self.theDefault)] >> ch_p('}');

            max = str_p("max{") >>
                  lexeme_d[*(~ch_p('}'))][push_max(self.eval, self.theMeasurements, self.theDefault)] >> ch_p('}');

            avg = str_p("avg{") >>
                  lexeme_d[*(~ch_p('}'))][push_avg(self.eval, self.theMeasurements, self.theDefault)] >> ch_p('}');

            factor = real | field | ci | sum | min | max | avg | '(' >> expression >> ')' |
                     ('-' >> factor)[do_negate(self.eval)] | ('+' >> factor) |
                     (str_p("sqrt(") >> expression >> ')')[do_sqrt(self.eval)];

            term = factor >> *(('*' >> factor)[make_op(multiplies<float>(), self.eval)] |
                               ('/' >> factor)[make_op(divides<float>(), self.eval)]);

            expression = term >> *(('+' >> term)[make_op(plus<float>(), self.eval)] |
                                   ('-' >> term)[make_op(minus<float>(), self.eval)]);
        }

        rule<ScannerT> expression, term, factor, real, field, sum, min, max, avg, ci;
        rule<ScannerT> const& start() const { return expression; }
    };
};

void
doEXPR(std::ostream& anOstream,
       std::string const& options,
       std::map<std::string, Measurement*>& measurements,
       std::string const& default_measurement)
{

    stack<float> eval;
    calculator calc(eval, measurements, default_measurement); //  Our parser

    try {
        parse_info<> info = parse(options.c_str(), calc, space_p);

        if (info.full) {
            if (eval.size() == 1) {
                anOstream << std::fixed << std::setw(5) << std::setprecision(2) << std::showpoint << std::right
                          << static_cast<float>(eval.top());
            } else {
                anOstream << "{ERR:Bad EXPR: Unable to calculate}";
            }
        } else {
            anOstream << "{ERR:Bad EXPR: Parse failed at " << info.stop << "}";
        }

    } catch (CalcException& anException) {
        anOstream << anException.theReason;
    } catch (...) {
        anOstream << "{ERR:Attempt to access non-SimpleMeasurement}";
    }
}

} // namespace aux_
} // namespace Stat
} // namespace Flexus
