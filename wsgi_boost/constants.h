#pragma once
/*
Pre-defined constants

Copyright (c) 2016 Roman Miroshnychenko <romanvm@yandex.ua>
License: MIT, see License.txt
*/

#include <unordered_map>
#include <array>
#include <string>

const std::array<char, 16> hex_chars{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

const std::array<std::string, 9> compressables{{
		".txt",
		".htm",
		".html",
		".xml",
		".css",
		".js",
		".json",
		".ttf",
		".svg"
	}};
