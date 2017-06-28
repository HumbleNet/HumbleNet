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

static bool has_option( json_value* options, const std::string& option ) {
	if( !options )
		return false;

	for( auto& it : jsonArrayIterator(options) ) {
		if( get_object_string( it ) == option )
			return true;
	}

	return false;
}

template<typename T>
static bool has_any_options( json_value* options, const T& list ) {
	if( !options )
		return false;

	for( auto& option : list ) {
		if( has_option( options, option ) )
			return true;
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

static void parseTypedefs(json_value* root, std::map< std::string, std::string >& typeMap )
{
	assert(root->type == json_array);

	for (auto& it: jsonArrayIterator(root)) {
		std::string tdef = get_object_string_key( it, "typedef" );

		if( tdef.empty() )
			continue;

		json_value* cstype = get_object_key( it, "cstype" );
		std::string mapped = get_object_string_key( cstype, "mapped" );

		if( ! mapped.empty() )
			typeMap.insert( std::make_pair( tdef, mapped ) );
	}
}

void write_functions( std::ostream& out, json_value* function_root, const std::map< std::string, std::string>& typeMap, const std::string& _template, const std::unordered_set< std::string >& includes, const std::unordered_set< std::string >& excludes ) {
	assert(function_root->type == json_array);

	for (auto& it: jsonArrayIterator(function_root))
	{
		std::string comment = get_object_string_key(it, "_comment");
		if (!comment.empty()) {
			out << "\n// " << comment << "\n";
		}
		std::string funcname = get_object_string_key(it, "functionname");
		std::string returntype = get_object_string_key(it, "returntype");
		
		if (!funcname.empty() && !returntype.empty()) {

			json_value* params = get_object_key(it, "params");
			json_value* options = get_object_key(it, "options");

			if( ! excludes.empty() && has_any_options( options, excludes ) )
				continue;

			if( !includes.empty() && !has_any_options( options, includes ) )
				continue;

			std::stringstream ss;

			if (params) {
				ss << "(";
				std::stringstream argss;
				for (auto& pit : jsonArrayIterator(params))
				{
					std::string paramtype = get_object_string_key(pit, "paramtype");
					auto typeIt = typeMap.find( paramtype );
					if( typeIt != typeMap.end() ) {
						paramtype = typeIt->second;
					}

					std::string paramname = get_object_string_key(pit, "paramname");
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

void write_enums( std::ostream& out, json_value* enum_root ) {
	assert(enum_root->type == json_array);

	for (auto& it: jsonArrayIterator(enum_root))
	{
		std::string enumname = get_object_string_key( it, "enumname");
		json_value* values = get_object_key( it, "values" );

		if( enumname.empty() || !values || values->type != json_array )
			continue;

		out << "\n";
		out << "	public enum " << enumname << "\n";
		out << "	{\n";

		for (auto& vit: jsonArrayIterator(values)) {
			std::string doc = get_object_string_key( vit, "doc" );
			std::string name = get_object_string_key( vit, "name" );
			std::string value = get_object_string_key( vit, "value" );

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

	json_value *root = json_parse(jsBuff.c_str(), jsBuff.size());
	if (root) {
		std::map< std::string, std::string > typeMap;

		json_value *typedef_root = get_object_key(root, "typedefs");
		if( typedef_root )
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

				write_functions( out, get_object_key(root,"functions"), typeMap, function_template, includes, excludes );
			} else if( token == "enums" ) {
				write_enums( out, get_object_key(root,"enums") );
			}
		};

		template_replace_tokens(outfile, tpl, processToken );

		json_value_free(root);
	} else {
		die("Unable to parse JSON file");
	}
}
