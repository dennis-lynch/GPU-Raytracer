#pragma once
#include <stdint.h>

#include "Core/String.h"
#include "Core/StringView.h"
#include "Core/Allocators/LinearAllocator.h"

namespace Util {
	StringView get_directory(StringView filename);
	StringView remove_directory(StringView filename);

	StringView get_file_extension(StringView filename);

	StringView substr(StringView str, size_t offset, size_t len = -1);

	String combine_stringviews(StringView path, StringView filename, Allocator * allocator = nullptr);

	const char * find_last_after(StringView haystack, StringView needles);

	const char * strstr(StringView haystack, StringView needle);

	String to_string(bool     value,                     Allocator * allocator = nullptr);
	String to_string(int64_t  value, int64_t  base = 10, Allocator * allocator = nullptr);
	String to_string(uint64_t value, uint64_t base = 10, Allocator * allocator = nullptr);
	String to_string(double   value,                     Allocator * allocator = nullptr);
}
