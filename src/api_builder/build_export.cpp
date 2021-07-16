#include "build_export.h"

#include "utilities.h"

#include <fstream>
#include <sstream>

void buildExport(const std::string& mode, const std::string& input, const std::string& output)
{
	bool isLinux = mode == "linux";

	std::ofstream outfile(output.c_str());

	std::string jsBuff = loadFile(input);

	try {
		auto root = json::parse( jsBuff );

		assert ( root.is_object() );

		auto function_root = json_optional<json>(root, "functions");

		assert( function_root.is_array() );

		if (isLinux) {
			outfile << "{\nglobal:\n";
		}

		for (auto& it: function_root)
		{
			std::string funcname = json_optional<std::string>(it, "functionname");
			if (!funcname.empty()) {
				if (isLinux) { // Linux
					outfile << "\t" << funcname << ";\n";
				} else { // OS X
					outfile << "_" << funcname << "\n";
				}
			}
		}

		if (isLinux) {
			outfile << "local: *;\n};\n";
		}
	} catch(json::parse_error &_ex) {
		die("Unable to parse JSON file");
	}
}