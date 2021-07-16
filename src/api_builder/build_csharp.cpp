#include "build_csharp.h"

#include "utilities.h"

#include <fstream>
#include <sstream>
#include <map>
#include <unordered_set>

/*
 * find the next token replacement.
 *
 * start - where the search should start in the template, if a match is found start is updated to point to the start of the match.
 * token - the found token
 *
 * returns position to resume searching for tokens.
 */
size_t template_find_token( const std::string& tpl, size_t& pos, std::string& token ) {
	size_t start = tpl.find("@", pos);
	if( start == tpl.npos )
		return tpl.npos;

	size_t end = tpl.find("@", start+1);
	if( end == tpl.npos )
		return tpl.npos;

	// ignore @@
	if( end - start == 1 ) {
		pos = end+1;
		return template_find_token( tpl, pos, token );
	}

	token = tpl.substr(start+1, end - start-1);

	pos = start;

	return end+1;
}

template< typename T >
void template_replace_tokens( std::ostream& out, const std::string& _template, T& processToken ) {
	size_t start = 0;
	size_t end = 0;
	size_t last = 0;

	std::string token;

	while( ( end = template_find_token( _template, start, token ) ) != _template.npos ) {
		// write content just before the found token
		if( start-last > 0 )
			out.write( _template.c_str() + last, start-last );

		processToken( out, token );

		start = last = end;
	}

	// write anything remaining.
	if( start < _template.length() )
		out.write( _template.c_str() + last, _template.length() - last );
}

template<typename T>
static bool has_any_options( json& options, const T& list ) {
	if (!options.is_object()) return false;

	for( auto& option : list ) {
		if (options.contains("option")) return true;
	}

	return false;
}

static std::unordered_set< std::string > split( const std::string& s, char delimiter ) {
	std::unordered_set< std::string > parts;

	std::stringstream ss(s);

	std::string tok;

	while(std::getline( ss, tok, delimiter ) ){
		parts.insert( tok );
	}

	return parts;
}

static void parseTypedefs(json& root, std::map< std::string, std::string >& typeMap )
{
	assert(root.is_array());

	for (auto& it: root) {
		std::string tdef =  json_optional<std::string>(it, "typedef");

		if( tdef.empty() )
			continue;

		auto cstype = json_optional<json>(it, "cstype");
		auto mapped = json_optional<std::string>(cstype, "mapped");

		if (!mapped.empty()) {
			typeMap.insert( std::make_pair( tdef, mapped ) );
		}
	}
}

void write_functions( std::ostream& out, const json& function_root, const std::map< std::string, std::string>& typeMap, const std::string& _template, const std::unordered_set< std::string >& includes, const std::unordered_set< std::string >& excludes ) {
	assert(function_root.is_array());

	for (auto& it: function_root)
	{
		std::string comment = json_optional<std::string>(it, "_comment");
		if (!comment.empty()) {
			out << "\n// " << comment << "\n";
		}
		std::string funcname = json_optional<std::string>(it, "functionname");
		std::string returntype = json_optional<std::string>(it, "returntype");
		
		if (!funcname.empty() && !returntype.empty()) {

			auto params = json_optional<json>(it, "params");
			auto options = json_optional<json>(it, "options");

			if( ! excludes.empty() && has_any_options( options, excludes ) )
				continue;

			if( !includes.empty() && !has_any_options( options, includes ) )
				continue;

			std::stringstream ss;

			if (params.is_array()) {
				ss << "(";
				std::stringstream argss;
				for (auto& pit : params)
				{
					std::string paramtype = json_optional<std::string>(pit, "paramtype");
					auto typeIt = typeMap.find( paramtype );
					if( typeIt != typeMap.end() ) {
						paramtype = typeIt->second;
					}

					std::string paramname = json_optional<std::string>(pit, "paramname");
					ss << " " << paramtype << " " << paramname << ",";
				}
				ss.seekp(-1, std::ios_base::cur);
				ss << " )";
			} else {
				ss << "()";
			}

			std::map< std::string, std::string > data;
			data.emplace( "functionname", funcname );

			auto typeIt = typeMap.find( returntype );
			if( typeIt != typeMap.end() ) {
				returntype = typeIt->second;
			}

			data.emplace( "returntype", returntype );
			data.emplace( "params", ss.str() );

			auto processToken = [data] ( std::ostream& out, const std::string& token ) {
				if( data.find( token ) != data.end() )
					out << data.find(token)->second;
			};

			template_replace_tokens( out, _template, processToken );
		}
	}
}

void write_enums( std::ostream& out, const json& enum_root ) {
	assert(enum_root.is_array());

	for (auto& it: enum_root)
	{
		std::string enumname =  json_optional<std::string>(it, "enumname");
		auto values = json_optional<json>(it, "values");

		if( enumname.empty() || !values.is_array() )
			continue;

		out << "\n";
		out << "	public enum " << enumname << "\n";
		out << "	{\n";

		for (auto& vit: values) {
			std::string doc =  json_optional<std::string>(vit, "doc");
			std::string name =  json_optional<std::string>(vit, "name");
			std::string value =  json_optional<std::string>(vit, "value");

			if( ! doc.empty() )
				out << "		// " << doc << "\n";

			out << "		" << name << " = " << value << ",\n";
		}

		out << "	}\n";
	}
}

static std::string function_template = (
						 "\n"
						 "		[DllImport (NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]\n"
						 "		public static extern @returntype@ @functionname@@params@;\n"
						 );


void buildCSharp(const std::string& input, const std::string& _template, const std::string& output)
{
	std::ofstream outfile(output.c_str());

	std::string jsBuff = loadFile(input);

	std::string tpl = loadFile( _template );
	if( tpl.empty() ) {
		die("Unable to load template file");
	}

	try {
		auto root = json::parse(jsBuff);

		std::map< std::string, std::string > typeMap;

		auto typedef_root = json_optional<json>(root, "typedefs");
		if( typedef_root.is_array() )
			parseTypedefs( typedef_root, typeMap );

		auto processToken = [root,typeMap] ( std::ostream& out, const std::string& token ) {

			if( ! token.compare( 0, 9, "functions" ) ) {

				std::unordered_set<std::string> includes;
				std::unordered_set<std::string> excludes;

				for( auto& option : split( token.substr(10), ' ' ) ) {
					if( option.length() < 3 )
						continue;
					else if( option[0] == '-' ) {
						excludes.insert( option.substr(1) );
					} else { // if( option[0] == '+' ) ) { include is implicit
						includes.insert( option.substr(1) );
					}
				}

				write_functions( out, root["functions"], typeMap, function_template, includes, excludes );
			} else if( token == "enums" ) {
				write_enums( out, root["enums"] );
			}
		};

		template_replace_tokens(outfile, tpl, processToken );
	} catch(json::parse_error& _ex) {
		die("Unable to parse JSON file");
	}
}