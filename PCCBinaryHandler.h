#pragma once

#include "../../Shared-ASI/Common.h"

#define MIN_REASONABLE_BINARY_FILE_SIZE 1

/**
  * Loads binary form of Unreal Script from file, as exported by ME3Tweaks' Package Editor.
  *     The result needs to be unpacked by Unreal script loader.
  *
  *     @param
  *			path	Absolute or Relative to the game's exe.
  *			bin		loaded array of bytes from file			
  *			size	size of the loaded array in bytes
  *
  *     @return
  *			true	only if the bin points to a valid byte array of size length, responsibilty for deallocation is the caller's.
  */
bool loadBin(const char* path, BYTE*& bin, long& size) {

	FILE* f = fopen(path, "rb");
	if (NULL == f)
		goto err;

	if (!fseek(f, 0, SEEK_END) || 0 != ferror(f))
		goto err;

	size = ftell(f) + 1;
	if (size < MIN_REASONABLE_BINARY_FILE_SIZE)
		goto err;

	if (!fseek(f, 0, SEEK_SET) || 0 != ferror(f))
		goto err;

	bin = (BYTE*)calloc(size, 0);
	if (NULL == bin)
		goto err;

	if (size != fread(bin, sizeof(char), size, f) || 0 != ferror(f))
		goto err;

	fclose(f);
	return true;

err:
	writeln("Load bin error.");
	if(NULL != bin) {
		free(f);
	}
	fclose(f);
	return false;
}