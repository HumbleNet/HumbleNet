#include "build_loader.h"

#include "utilities.h"

#include <fstream>
#include <sstream>

void buildLoader(const std::string& input, const std::string& output)
{
	std::ofstream outfile(output.c_str());

	std::string jsBuff = loadFile(input);

	try {
		auto root = json::parse(jsBuff);
		auto function_root = json_optional<json>(root, "functions");

		assert(function_root.is_array());

		for (auto& it: function_root)
		{
			std::string comment = json_optional<std::string>(it, "_comment");
			if (!comment.empty()) {
				outfile << "// Comment " << comment << "\n";
			}
			std::string funcname = json_optional<std::string>(it, "functionname");
			std::string returntype = json_optional<std::string>(it, "returntype");
			if (!funcname.empty() && !returntype.empty()) {
				auto params = json_optional<json>(it, "params");
				outfile << "#ifndef HUMBLENET_SKIP_" << funcname << "\n";
				std::stringstream ss;
				ss << "LOADER_PROC(";
				ss << returntype;
				ss << ",";
				ss << funcname;
				ss << ",";
				if (params.is_array()) {
					ss << "(";
					std::stringstream argss;
					for (auto& pit : params)
					{
						std::string paramtype = json_optional<std::string>(pit, "paramtype");
						std::string paramname = json_optional<std::string>(pit, "paramname");
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
	} catch( json::parse_error& _ex ) {
		die("Unable to parse JSON file");
	}
}