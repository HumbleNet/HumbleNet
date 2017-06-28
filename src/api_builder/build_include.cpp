#include "build_include.h"

#include "utilities.h"

#include <fstream>

std::string varSafeName(const std::string& name) {
	std::string ret = name;

	auto pos = ret.rfind('/');

	if (pos != ret.npos) {
		ret = ret.substr(pos + 1);
	}

	pos = ret.find('.');
	if (pos != ret.npos) {
		ret = ret.substr(0, pos);
	}

	return ret;
}

static char hexdigit[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

void buildInclude(const std::string& mode, const std::string& input, const std::string& output)
{
	std::ofstream outfile(output.c_str());

	std::ifstream infile(input.c_str(), std::ios_base::binary);

	char buff[2048];
	std::streamsize total_bytes = 0;

	outfile << "const unsigned char " << varSafeName(output) << "[] = {\n";

	while (infile) {
		infile.read(buff, sizeof(buff));
		auto bytes_read = infile.gcount();
		total_bytes += bytes_read;
		for (int i = 0; i < bytes_read; ++i) {
			if ((i % 16) == 0) {
				outfile << "\t";
			}
			outfile << "0x" << hexdigit[(buff[i] >> 4) & 0xf] << hexdigit[buff[i] & 0xf] << ", ";
			if ((i % 16) == 15) {
				outfile << "\n";
			}
		}
	}

	outfile << "};\n";
	outfile << "unsigned int " << varSafeName(output) << "_len = " << total_bytes << ";\n";
}