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
#include <core/debug/debugger.hpp>
#include <core/debug/parser.hpp>

#define PHOENIX_LIMIT 6

#include <boost/spirit/include/classic_attribute.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_file_iterator.hpp>
#include <boost/spirit/include/classic_utility.hpp>
#include <core/boost_extensions/phoenix.hpp>

namespace Flexus {

extern std::string oldcwd;

namespace Dbg {

namespace aux_ {
using namespace boost::spirit::classic;
using namespace phoenix;

int32_t s_crit  = SevCrit;
int32_t s_tmp   = SevTmp;
int32_t s_dev   = SevDev;
int32_t s_trace = SevTrace;
int32_t s_iface = SevIface;
int32_t s_vverb = SevVerb;
int32_t s_verb  = SevVVerb;
int32_t s_inv   = SevInv;

// The parse context.  The parser has one context object.  This object lets the
// parser accumulate parser pieces of the debug configuration and then hand them
// off to the debugger.
struct Context
{
    std::unique_ptr<CompoundFilter> theFilters;
    std::unique_ptr<CompoundAction> theActions;
    std::string theTargetName;

  public:
    Context()
      : theFilters()
      , theActions()
    {
    }

    void reset()
    {
        theFilters.reset(new CompoundFilter());
        theActions.reset(new CompoundAction());
    }

    void finishTarget()
    {
        Debugger::theDebugger->add(new Target(theTargetName, theFilters.release(), theActions.release()));
        reset();
    }

    void addAction(Action* anAction)
    {                                                       // Ownership of anAction passed in
        theActions->add(std::unique_ptr<Action>(anAction)); // Onwership of anAction passed to theActions
    }

    void addFilter(Filter* aFilter)
    {                             // Ownership of aFilter passed in
        theFilters->add(aFilter); // Ownership of aFilter passed to theFilteer
    }

    void addAtStmt(int64_t aCycle, Action* anAction)
    {                                                   // Ownership of anAction passed in
        Debugger::theDebugger->addAt(aCycle, anAction); // Onwership of anAction passed to debugger
    }

    void setTargetName(std::string const& aName) { theTargetName = aName; }
};

// Closures for parse rules.  These closures set up local variable scopes during
// parsing so parse rules can pass data around
struct log_closure : public boost::spirit::classic::closure<log_closure, Action*, CompoundFormat*>
{
    member1 theAction;
    member2 theFormat;
};

struct spill_closure : public boost::spirit::classic::closure<spill_closure, Action*, CompoundFormat*, std::string>
{
    member1 theAction;
    member2 theFormat;
    member3 theBuffer;
};

struct at_closure : public boost::spirit::classic::closure<at_closure, uint64_t>
{
    member1 theCycle;
};

struct format_closure : public boost::spirit::classic::closure<format_closure, Format*>
{
    member1 theFormat;
};

struct return_string_closure : public boost::spirit::classic::closure<return_string_closure, std::string>
{
    member1 theRetVal;
};

struct return_longlong_closure : public boost::spirit::classic::closure<return_longlong_closure, int64_t>
{
    member1 theRetVal;
};

struct compound_filter_closure : public boost::spirit::classic::closure<compound_filter_closure, CompoundFilter*>
{
    member1 theChain;
};

struct return_filter_closure : public boost::spirit::classic::closure<return_filter_closure, Filter*>
{
    member1 theRetVal;
};

struct return_action_closure : public boost::spirit::classic::closure<return_action_closure, Action*>
{
    member1 theRetVal;
};

struct general_filter_closure : public boost::spirit::classic::closure<general_filter_closure, Filter*, std::string>
{
    member1 theRetVal;
    member2 theFieldName;
};

struct severity_filter_closure : public boost::spirit::classic::closure<severity_filter_closure, Filter*, int64_t>
{
    member1 theRetVal;
    member2 theSeverity;
};

struct return_severity_closure : public boost::spirit::classic::closure<return_severity_closure, int64_t>
{
    member1 theRetVal;
};

// The Debugger's parse grammar
struct DebugGrammar : public boost::spirit::classic::grammar<DebugGrammar>
{

    // The parse context used to accumulate the results of the parse
    Context& theContext;

    DebugGrammar(Context& aContext)
      : theContext(aContext)
    {
    }

    template<typename ScannerT>
    struct definition
    {
        // Declare all rules, and their associated closures, if any
        rule<ScannerT> debug_cfg;
        rule<ScannerT> statement;
        rule<ScannerT, at_closure::context_t> at_stmt;
        rule<ScannerT> target_stmt;
        rule<ScannerT> target_clause;
        rule<ScannerT> filter_expr;
        rule<ScannerT, compound_filter_closure::context_t> include_filter_chain;
        rule<ScannerT, compound_filter_closure::context_t> exclude_filter_chain;
        rule<ScannerT> and_filter;
        rule<ScannerT, return_filter_closure::context_t> filter;
        rule<ScannerT, severity_filter_closure::context_t> severity_filter;
        rule<ScannerT, general_filter_closure::context_t> general_filter;
        rule<ScannerT, return_filter_closure::context_t> category_filter;
        rule<ScannerT, return_filter_closure::context_t> address_filter;
        rule<ScannerT, return_filter_closure::context_t> idx_filter;
        rule<ScannerT, return_string_closure::context_t> field_name;
        rule<ScannerT, return_severity_closure::context_t> severity;
        rule<ScannerT> comparison_clause;
        rule<ScannerT> equal_clause;
        rule<ScannerT> not_equal_clause;
        rule<ScannerT> less_than_clause;
        rule<ScannerT> greater_than_clause;
        rule<ScannerT> less_equal_clause;
        rule<ScannerT> greater_equal_clause;
        rule<ScannerT> exists_clause;
        rule<ScannerT, return_longlong_closure::context_t> numeric_value;
        rule<ScannerT, return_string_closure::context_t> string_value;
        rule<ScannerT, return_action_closure::context_t> action_stmt;
        rule<ScannerT, return_action_closure::context_t> abort_action;
        rule<ScannerT, return_action_closure::context_t> break_action;
        rule<ScannerT, return_action_closure::context_t> print_stats_action;
        rule<ScannerT, return_action_closure::context_t> severity_action;
        rule<ScannerT, log_closure::context_t> log_action;
        rule<ScannerT> console;
        rule<ScannerT, return_string_closure::context_t> file;
        rule<ScannerT, format_closure::context_t> format_spec;

        // The grammar definition
        definition(DebugGrammar const& self)
        {
            // TODO - semantic actions

            debug_cfg = +(statement);

            statement = target_stmt[phoenix::bind(&Context::finishTarget)(phoenix::var(self.theContext))] | at_stmt;

            at_stmt =
              str_p("at") >> numeric_value[at_stmt.theCycle = arg1] >>
              action_stmt[phoenix::bind(&Context::addAtStmt)(phoenix::var(self.theContext), at_stmt.theCycle, arg1)];

            target_stmt = str_p("target") >>
                          string_value[phoenix::bind(&Context::setTargetName)(phoenix::var(self.theContext), arg1)] >>
                          ch_p('{') >> target_clause >> ch_p('}');

            target_clause = str_p("filter") >> ch_p('{') >> *filter_expr >> ch_p('}') >> str_p("action") >> ch_p('{') >>
                            *(action_stmt[phoenix::bind(&Context::addAction)(phoenix::var(self.theContext), arg1)]) >>
                            ch_p('}');

            filter_expr =
              +(include_filter_chain[phoenix::bind(&Context::addFilter)(phoenix::var(self.theContext), arg1)] |
                exclude_filter_chain[phoenix::bind(&Context::addFilter)(phoenix::var(self.theContext), arg1)]);

            include_filter_chain =
              ch_p('+')[include_filter_chain.theChain = phoenix::new_0((IncludeFilter*)0)] >>
              filter[phoenix::bind(&CompoundFilter::add)(*include_filter_chain.theChain, arg1)] >>
              *(ch_p('&') >> filter[phoenix::bind(&CompoundFilter::add)(*include_filter_chain.theChain, arg1)]) >> ';';

            exclude_filter_chain =
              ch_p('-')[exclude_filter_chain.theChain = phoenix::new_0((ExcludeFilter*)0)] >>
              filter[phoenix::bind(&CompoundFilter::add)(*exclude_filter_chain.theChain, arg1)] >>
              *(ch_p('&') >> filter[phoenix::bind(&CompoundFilter::add)(*exclude_filter_chain.theChain, arg1)]) >> ';';

            filter = address_filter[filter.theRetVal = arg1] | idx_filter[filter.theRetVal = arg1] |
                     general_filter[filter.theRetVal = arg1] | severity_filter[filter.theRetVal = arg1] |
                     category_filter[filter.theRetVal = arg1]
              // TODO - Add remaining filters
              ;

            severity_filter =
              severity[severity_filter.theSeverity = arg1] >>
              (ch_p('+')[severity_filter.theRetVal = phoenix::new_2((OpFilter<LongLong, OpGreaterEqual>*)0,
                                                                    "SeverityNumeric",
                                                                    severity_filter.theSeverity)] |
               ch_p('-')[severity_filter.theRetVal = phoenix::new_2((OpFilter<LongLong, OpLessEqual>*)0,
                                                                    "SeverityNumeric",
                                                                    severity_filter.theSeverity)] |
               epsilon_p[severity_filter.theRetVal = phoenix::new_2((OpFilter<LongLong, OpEqual>*)0,
                                                                    "SeverityNumeric",
                                                                    severity_filter.theSeverity)]);

            severity = as_lower_d[(str_p("critical") | str_p("crit"))[severity.theRetVal = s_crit] |
                                  (str_p("development") | str_p("dev"))[severity.theRetVal = s_dev] |
                                  (str_p("trace") | str_p("trc"))[severity.theRetVal = s_trace] |
                                  (str_p("interface") | str_p("iface"))[severity.theRetVal = s_iface] |
                                  (str_p("verbose") | str_p("verb"))[severity.theRetVal = s_verb] |
                                  (str_p("veryverbose") | str_p("vverb"))[severity.theRetVal = s_vverb]];

            general_filter = field_name[general_filter.theFieldName = arg1] >> comparison_clause;

            address_filter =
              ch_p('@') >> numeric_value[address_filter.theRetVal =
                                           phoenix::new_2((OpFilter<LongLong, OpEqual>*)0, "Address", arg1)];

            idx_filter = ch_p('[') >>
                         numeric_value[idx_filter.theRetVal =
                                         phoenix::new_2((OpFilter<LongLong, OpEqual>*)0, "FlexusIdx", arg1)] >>
                         ch_p(']');

            category_filter =
              lexeme_d[(+((alnum_p | '_') - space_p))]
                      [category_filter.theRetVal =
                         phoenix::new_1((CategoryFilter*)0, phoenix::construct_<std::string>(arg1, arg2))];

            field_name =
              confix_p('{',
                       (+(alnum_p | '_'))[field_name.theRetVal = phoenix::construct_<std::string>(arg1, arg2)],
                       '}');

            comparison_clause = equal_clause | not_equal_clause | less_than_clause | greater_than_clause |
                                less_equal_clause | greater_equal_clause | exists_clause
              // TODO - implement and add range and regex filters
              ;

            equal_clause =
              str_p("==") >>
              (string_value[general_filter.theRetVal =
                              phoenix::new_2((OpFilter<String, OpEqual>*)0, general_filter.theFieldName, arg1)] |
               numeric_value[general_filter.theRetVal =
                               phoenix::new_2((OpFilter<LongLong, OpEqual>*)0, general_filter.theFieldName, arg1)]);

            not_equal_clause =
              str_p("!=") >>
              (string_value[general_filter.theRetVal =
                              phoenix::new_2((OpFilter<String, OpNotEqual>*)0, general_filter.theFieldName, arg1)] |
               numeric_value[general_filter.theRetVal =
                               phoenix::new_2((OpFilter<LongLong, OpNotEqual>*)0, general_filter.theFieldName, arg1)]);

            less_than_clause =
              str_p("<") >>
              (string_value[general_filter.theRetVal =
                              phoenix::new_2((OpFilter<String, OpLessThan>*)0, general_filter.theFieldName, arg1)] |
               numeric_value[general_filter.theRetVal =
                               phoenix::new_2((OpFilter<LongLong, OpLessThan>*)0, general_filter.theFieldName, arg1)]);

            greater_than_clause =
              str_p(">") >>
              (string_value[general_filter.theRetVal =
                              phoenix::new_2((OpFilter<String, OpGreaterThan>*)0, general_filter.theFieldName, arg1)] |
               numeric_value[general_filter.theRetVal = phoenix::new_2((OpFilter<LongLong, OpGreaterThan>*)0,
                                                                       general_filter.theFieldName,
                                                                       arg1)]);

            less_equal_clause =
              str_p("<=") >>
              (string_value[general_filter.theRetVal =
                              phoenix::new_2((OpFilter<String, OpLessEqual>*)0, general_filter.theFieldName, arg1)] |
               numeric_value[general_filter.theRetVal =
                               phoenix::new_2((OpFilter<LongLong, OpLessEqual>*)0, general_filter.theFieldName, arg1)]);

            greater_equal_clause =
              str_p(">=") >>
              (string_value[general_filter.theRetVal =
                              phoenix::new_2((OpFilter<String, OpGreaterEqual>*)0, general_filter.theFieldName, arg1)] |
               numeric_value[general_filter.theRetVal = phoenix::new_2((OpFilter<LongLong, OpGreaterEqual>*)0,
                                                                       general_filter.theFieldName,
                                                                       arg1)]);

            exists_clause =
              str_p("exists")[general_filter.theRetVal = phoenix::new_1((ExistsFilter*)0, general_filter.theFieldName)];

            string_value =
              lexeme_d[confix_p(ch_p('"'),
                                (*c_escape_ch_p)[string_value.theRetVal = phoenix::construct_<std::string>(arg1, arg2)],
                                '"')];

            numeric_value = str_p("0x") >> int_parser<int64_t, 16, 1, -1>()[numeric_value.theRetVal = arg1] |
                            int_parser<int64_t, 10, 1, -1>()[numeric_value.theRetVal = arg1];

            action_stmt = log_action[action_stmt.theRetVal = arg1] | abort_action[action_stmt.theRetVal = arg1] |
                          break_action[action_stmt.theRetVal = arg1] | severity_action[action_stmt.theRetVal = arg1] |
                          print_stats_action[action_stmt.theRetVal = arg1]

              // TODO - add other actions
              ;

            /*
                        save_action           =   str_p("save")
                                                  >> file
                                                        [ save_action.theBuffer = arg1
               ]
                                                  >> numeric_value
                                                        [ save_action.theAction=
               phoenix::new_2( (SaveAction *) 0, save_action.theBuffer, arg1 ) ]
                                                  >>  ';'
                                              ;
            */

            log_action = str_p("log") >>
                         (console[log_action.theFormat = phoenix::new_0((CompoundFormat*)0)]
                                 [log_action.theAction = phoenix::new_1((ConsoleLogAction*)0, log_action.theFormat)] |
                          file[log_action.theFormat = phoenix::new_0((CompoundFormat*)0)]
                              [log_action.theAction = phoenix::new_2((FileLogAction*)0, arg1, log_action.theFormat)]) >>
                         *(format_spec[phoenix::bind(&CompoundFormat::add)(*log_action.theFormat, arg1)]) >> ';';
            /*
                        spill_action            =   str_p("spill")
                                                  >>  file
                                                          [ spill_action.theBuffer =
               arg1 ]
                                                  >>  str_p("->")
                                                  >>  ( console
                                                          [ spill_action.theFormat =
               phoenix::new_0( (CompoundFormat *) 0) ] [ spill_action.theAction =
               phoenix::new_2( (ConsoleSpillAction *) 0, spill_action.theBuffer,
               spill_action.theFormat ) ] | file [ spill_action.theFormat =
               phoenix::new_0( (CompoundFormat *) 0) ] [ spill_action.theAction =
               phoenix::new_3( (FileSpillAction *) 0, spill_action.theBuffer, arg1,
               spill_action.theFormat ) ]
                                                      )
                                                  >> *(
                                                        format_spec
                                                          [ phoenix::bind(
               &CompoundFormat::add) ( *spill_action.theFormat, arg1 ) ]
                                                      )
                                                  >>  ';'
                                              ;
            */
            abort_action = str_p("abort")[abort_action.theRetVal = phoenix::new_0((AbortAction*)0)] >> ';';

            severity_action = str_p("set-global-severity") >>
                              severity[severity_action.theRetVal = phoenix::new_1((SeverityAction*)0, arg1)] >> ';';

            break_action = str_p("break")[break_action.theRetVal = phoenix::new_0((BreakAction*)0)] >> ';';

            print_stats_action =
              str_p("print-stats")[print_stats_action.theRetVal = phoenix::new_0((PrintStatsAction*)0)] >> ';';

            console = str_p("console");

            file = confix_p('(', (*anychar_p)[file.theRetVal = phoenix::construct_<std::string>(arg1, arg2)], ')');

            format_spec = string_value[format_spec.theFormat = phoenix::new_1((StaticFormat*)0, arg1)] |
                          field_name[format_spec.theFormat = phoenix::new_1((FieldFormat*)0, arg1)]
              // TODO - change to field_spec to support arrays?
              ;
        }

        rule<ScannerT> const& start() const { return debug_cfg; }
    };
};

} // namespace aux_

class ParserImpl : public Parser
{
    aux_::Context theContext;
    aux_::DebugGrammar theDebugGrammar;

  public:
    ParserImpl()
      : theDebugGrammar(theContext)
    {
    }

    virtual ~ParserImpl() {}

    void parse(std::string const& aFileName)
    {
        theContext.reset();

        boost::spirit::classic::file_iterator<> start_iter(aFileName.c_str());
        boost::spirit::classic::file_iterator<> end_iter = start_iter.make_end();

        auto parsed =
          boost::spirit::classic::parse(start_iter, end_iter, theDebugGrammar, boost::spirit::classic::space_p);

        if (parsed.hit)
            std::cout << "Successfully parsed debug configurations from " << aFileName << "\n";
        else
            std::cout << "DEBUG ERROR: Failed to parse debug configurations from " << aFileName << "\n";
    }
};

Parser&
Parser::parser()
{
    static ParserImpl theStaticParser;
    return theStaticParser;
}

void
Debugger::initialize()
{
    Parser::parser().parse(Flexus::oldcwd + "/debug.cfg");
}

void
Debugger::addFile(std::string const& aFile)
{
    Parser::parser().parse(aFile.c_str());
}

} // namespace Dbg
} // namespace Flexus
