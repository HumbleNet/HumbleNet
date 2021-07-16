#ifdef _MSC_VER
// silence warnings about fopen
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "utilities.h"

void die(const std::string& error, int code)
{
	std::cerr << "Error: " << error << std::endl;
	exit(code);
}

std::string loadFile(const std::string& file)
{
	std::string ret = "";

	char *buff = nullptr;

	FILE *fp = fopen(file.c_str(), "rb");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		size_t size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buff = new char[size];
		if (fread(buff, 1, size, fp) == size) {
			ret.assign(buff, size);
		}
		delete [] buff;
		fclose(fp);
	}
	return ret;
}