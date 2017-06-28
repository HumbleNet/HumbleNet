#include "build_export.h"

#include "utilities.h"

#include <fstream>
#include <sstream>

void buildExport(const std::string& mode, const std::string& input, const std::string& output)
{
	bool isLinux = mode == "linux";

	std::ofstream outfile(output.c_str());

	std::string jsBuff = loadFile(input);

	json_value *root = json_parse(jsBuff.c_str(), jsBuff.size());
	if (root) {
		json_value *function_root = get_object_key(root, "functions");

		assert(function_root->type == json_array);

		if (isLinux) {
			outfile << "{\nglobal:\n";
		}

		for (auto& it: jsonArrayIterator(function_root))
		{
			std::string funcname = get_object_string_key(it, "functionname");
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

		json_value_free(root);
	} else {
		die("Unable to parse JSON file");
	}
}
