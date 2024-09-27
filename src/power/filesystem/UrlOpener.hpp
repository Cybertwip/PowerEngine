#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#elif defined(__APPLE__)
#include <cstdlib>
#else
#include <cstdlib>
#endif

class UrlOpener {
public:
	/**
	 * Opens the given URL in the default web browser.
	 * @param url The URL to open.
	 * @return true if the operation was successful, false otherwise.
	 */
	static bool openUrl(const std::string& url) {
#ifdef _WIN32
		// Convert std::string to wide string
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), (int)url.length(), NULL, 0);
		std::wstring wurl(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, url.c_str(), (int)url.length(), &wurl[0], size_needed);
		
		// Use ShellExecuteW to open the URL
		HINSTANCE result = ShellExecuteW(NULL, L"open", wurl.c_str(), NULL, NULL, SW_SHOWNORMAL);
		// According to documentation, the return value is greater than 32 if successful
		return reinterpret_cast<intptr_t>(result) > 32;
#elif defined(__APPLE__)
		// Construct the command: open "url"
		std::string command = "open \"" + url + "\"";
		int result = system(command.c_str());
		return result == 0;
#else
		// For other platforms, attempt to use xdg-open
		std::string command = "xdg-open \"" + url + "\"";
		int result = system(command.c_str());
		return result == 0;
#endif
	}
};

