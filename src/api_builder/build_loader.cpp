#include "build_loader.h"

#include "utilities.h"

#include <fstream>
#include <sstream>

void buildLoader(const std::string& input, const std::string& output)
{
	std::ofstream outfile(output.c_str());

	std::string jsBuff = loadFile(input);

	json_value *root = json_parse(jsBuff.c_str(), jsBuff.size());
	if (root) {
		json_value *function_root = get_object_key(root, "functions");

		assert(function_root->type == json_array);

		for (auto& it: jsonArrayIterator(function_root))
		{
			std::string comment = get_object_string_key(it, "_comment");
			if (!comment.empty()) {
				outfile << "// Comment " << comment << "\n";
			}
			std::string funcname = get_object_string_key(it, "functionname");
			std::string returntype = get_object_string_key(it, "returntype");
			if (!funcname.empty() && !returntype.empty()) {
				json_value* params = get_object_key(it, "params");
				outfile << "#ifndef HUMBLENET_SKIP_" << funcname << "\n";
				std::stringstream ss;
				ss << "LOADER_PROC(";
				ss << returntype;
				ss << ",";
				ss << funcname;
				ss << ",";
				if (params) {
					ss << "(";
					std::stringstream argss;
					for (auto& pit : jsonArrayIterator(params))
					{
						std::string paramtype = get_object_string_key(pit, "paramtype");
						std::string paramname = get_object_string_key(pit, "paramname");
						ss << paramtype << " " << paramname << ",";
						argss << paramname << ",";
					}
					ss.seekp(-1, std::ios_base::cur);
					ss << "),(";
					ss << argss.str();
					ss.seekp(-1, std::ios_base::cur);
					ss << "),";
				} else {
					ss << "(void),(),";
				}
				if (returntype != "void") {
					ss << "return";
				}
				ss << ")\n";

				outfile << ss.str() << "#endif\n";
			}
		}
		json_value_free(root);
	} else {
		die("Unable to parse JSON file");
	}
}
