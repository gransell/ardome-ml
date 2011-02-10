/*
 *  stack_dump.mm
 *  au_base
 *
 *  Created by Mikael Gransell on 3/20/07.
 *  Copyright 2007 Ardendo AB. All rights reserved.
 *
 */

#include "stack_dump.hpp"
#include <boost/cstdint.hpp>
#include <boost/regex.hpp>
#include <opencorelib/cl/minimal_string_defines.hpp>
 
#include <execinfo.h>
#include <stdio.h>
#include <iostream>

namespace abi {
extern "C" char* __cxa_demangle (const char* mangled_name,
                                 char* buf,
                                 size_t* n,
                                 int* status);
    
}

namespace olib { namespace opencorelib { namespace mac {

using namespace abi;


	// Parses a template expression.
	//
	// Puts the outer template name into 'before' its suffix and 'after'
	// and the inner template arguments into 'inner' and returns true.
	//
	// Does nothing and returns false if it isn't recognized as a template.
	bool extract_template(const std::string& in, std::string& before, std::string& inner, std::string& after)
	{
		size_t start, i, j = 0, n = 1;

		i = in.find_first_of("<>");
		if (i == std::string::npos || in[i] == '>' || i == 0)
			return false;

		start = i + 1;
		j = i + 1;

		while ((i = in.find_first_of("<>", j)) != std::string::npos)
		{
			in[i] == '<' ? ++n : --n;

			if (!n) {
				// We've found the matching end tag
				before = in.substr(0, start - 1);
				inner = in.substr(start, i - start);
				after = in.substr(i + 1);
				return true;
			}

			j = i + 1;
		}

		return false;
	}

	std::string prettify_templated_part(const std::string& part, int base_indent = 0, int space_indent = 0, std::string prefix = "")
	{
		std::string indent(base_indent, '\t');
		std::string next(space_indent * 2, ' ');

		std::string before, inner, after;

		if (extract_template(part, before, inner, after)) {
			return prettify_templated_part(before, base_indent, space_indent + 1, prefix) + "<\n" +
			       prettify_templated_part(inner,  base_indent, space_indent + 2) + "\n" +
			       prettify_templated_part(after,  base_indent, space_indent + 1, ">");
		} else {
			return indent + next + prefix + part;
		}
	}

	// Nicens up and prettifies a function name, especially if they have a
	// deep template hierarchy
	std::string prettify_templated_function(const std::string& func, int base_indent = 0)
	{
		std::string indent(base_indent, '\t');

		boost::regex e("\\s*"  "([^(]+)"  "\\((.*)\\)"  "(.*)");
		boost::smatch what;

		if (!boost::regex_match( func, what, e, boost::match_extra ))
			return indent + func;

		std::string funcname(what[1]);
		std::string params(what[2]);
		std::string qualifiers(what[3]);

		if (params.find('<') != std::string::npos)
			params = "(\n" + prettify_templated_part(params, base_indent, 1) + "\n" + indent + ")";
		else
			params = "(" + params + ")";

		return
			prettify_templated_part(funcname, base_indent) +
			params + qualifiers;
	}

#ifdef OLIB_ON_LINUX
	std::string demangle_line(const std::string& line)
	{
		boost::regex e("\\s*([^(]*)"  "\\("  "([^+)]*)"  "[^[]*"  "\\[(.*)\\].*");
		boost::smatch what;

		if( boost::regex_match( line, what, e, boost::match_extra ) )
		{
			// Demangle name
			int status = 0;
			char demangled[4096];
			size_t l = sizeof(demangled);
			std::string routine( what[2] );
			__cxa_demangle( routine.c_str() , demangled, &l, &status );
			if( status >= 0 )
			{
				std::string demang(demangled);
				demang = prettify_templated_function(demang, 2);

				return "\tFunction:\n" + demang + "\n\tFile: " + std::string(what[1]);
			}
		}

		return "\t( Function unknown )\n\tSymbol: " + line;
	}
#elif defined(OLIB_ON_MAC)
	std::string demangle_line(const std::string& line)
	{
		boost::regex e("\\s*(\\d+)\\s+(\\S+)\\s+(\\S+)\\s*(\\S+)(.+)");
		boost::smatch what;

		if( boost::regex_match( line, what, e, boost::match_extra ) )
		{
			// Demangle name
			int status = 0;
			char demangled[4096];
			size_t l = sizeof(demangled);
			std::string routine( what[4] );
			__cxa_demangle( routine.c_str() , demangled, &l, &status );
			if( status >= 0 )
			{
				return std::string(what[1]) + "\t" + std::string(what[2]) + "\t" + demangled;
			}
		}

		return line;
	}
#endif

	utf8_string get_stack_trace()
	{

		void* callstack[128];
		int i, frames = backtrace(callstack, 128);
		char** strs = backtrace_symbols(callstack, frames);
		char sframenum[64];

		utf8_string trace = "";

		// spaces (line number) spaces (library) spaces (adr) space (routine)
		boost::smatch what;

		for (i = 0; i < frames; ++i) {
			snprintf(sframenum, sizeof sframenum, "Frame %d:\n", i + 1);
			trace += sframenum;

			utf8_string line(strs[i]);
			trace += demangle_line(line) + "\n";

		}

		free(strs);

		return trace;
	}
} } }

