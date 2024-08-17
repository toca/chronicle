#include "completion.h"
#include "completionitem.h"
#include "stringutil.h"
#include <vector>
#include <string>

namespace Completion
{
	// declaration
	std::vector<std::wstring> Tokenize(const std::wstring& input);
	std::optional<std::filesystem::path> TryToGetPath(const std::wstring& str);



	std::vector<Item> Candidates(const std::wstring& input, const std::wstring& currentDirectoryStr)
	{
		bool moving = false;
		if (_wcsnicmp(input.c_str(), L"cd ", 3) == 0) {
			moving = true;
		}
		if (_wcsnicmp(input.c_str(), L"pushd ", 6) == 0) {
			moving = true;
		}

		// Tokenize
		std::vector<std::wstring> tokens = Tokenize(input);
		std::wstring current = tokens.empty() ? L"" : tokens.back();

		// Target Directory
		std::filesystem::path targetDir(currentDirectoryStr);
		std::optional<std::filesystem::path> dir = TryToGetPath(current);
		if (dir) {
			current = current.substr(dir->wstring().length());
			if (dir->is_absolute()) {
				targetDir = *dir;
			}
			else {
				targetDir /= *dir;
			}
		}

		// Find directory items
		std::vector<Item> result{};
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(targetDir)) {
			std::wstring filename = entry.path().filename().wstring();

			if (moving && !entry.is_directory()) {
				continue;
			}

			if (filename.starts_with(current)) {
				if (entry.is_directory()) {
					filename += L"\\";
				}
				result.push_back({ filename, filename.substr(current.length())});
			}
		}
		return result;
	}

	std::vector<std::wstring> Tokenize(const std::wstring& input)
	{
		std::vector<std::wstring> tokens{};
		bool quote = false;
		size_t offset = 0;
		for (size_t i = 0; i < input.size(); i++) {

			if (input[i] == L'"') {
				quote = !quote;
			}
			else if (!quote && input[i] == L' ') {
				tokens.push_back(std::wstring(input.begin() + offset, input.begin() + i));
				offset = i + 1;
			}
		}
		tokens.push_back(input.substr(offset));
		return tokens;
	}


	std::optional<std::filesystem::path> TryToGetPath(const std::wstring& str)
	{
		if (str.find(L"/") == std::wstring::npos && str.find(L"\\") == std::wstring::npos) {
			return std::nullopt;
		}
		try {
			std::filesystem::path path(str);
			if (path.has_filename()) {
				return path.parent_path() / L"";
			}
			return path / L"";
		}
		catch (const std::filesystem::filesystem_error&) {
			return std::nullopt;
		}
	}

}