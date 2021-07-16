#include "utilities.h"

#include "build_loader.h"
#include "build_csharp.h"
#include "build_export.h"
#include "build_include.h"

NORETURN void usage(const std::string& prog, const std::string& error = "")
{
	std::cout
		<< "Usage: " << prog << " mode API.json [template_file] output_file\n"
		<< "  Modes are\n"
		<< "    loader : takes the API.json and output file as arguments and generates a loader_proc header file for use by loader.cpp\n"
		<< "    osx    : takes the API.json and output file as arguments and generates an OS X symbol export file\n"
		<< "    linux  : takes the API.json and output file as arguments and generates an Linux symbol export file\n"
		<< "    include: takes any input file and output file as arguments and loads the input file into an C includable output file\n"
		<< "    csharp : takes the API.json and output file as arguments and generates a CS wrapper module\n"
		<< "\n";
	if (!error.empty()) {
		std::cerr << "Error: " << error << std::endl;
	}
	exit(1);
}

int main(int argc, char* argv[])
{
	if (argc < 4) {
		usage(argv[0]);
	}
	const std::string mode = argv[1];
	const std::string json_file = argv[2];
	const std::string template_file = argc > 4 ? argv[3] : "";
	const std::string output_file = argc > 4 ? argv[4] : argv[3];

	if (mode == "loader") {
		buildLoader(json_file, output_file);
	} else if (mode == "csharp") {
		buildCSharp(json_file, template_file, output_file);
	} else if (mode == "osx" || mode == "linux") {
		buildExport(mode, json_file, output_file);
	} else if (mode == "include") {
		buildInclude(mode, json_file, output_file);
	} else {
		usage(argv[0], "Unknown mode");
	}

	return 0;
}