#pragma once

#include <string>
#include <vector>
#include <functional>

// Forward-declare NSView to avoid including AppKit in this pure C++ header.
// The __OBJC__ macro is defined by the Objective-C++ compiler.
#ifdef __OBJC__
@class NSView;
#else
typedef void NSView;
#endif

/**
 * @class ContextMenu
 * @brief A C++ class to build and show a native macOS context menu.
 *
 * This class provides a simple C++ interface to create a menu with items
 * that trigger C++ lambda callbacks. The actual UI is handled by Objective-C++
 * in the implementation file.
 */
class ContextMenu {
public:
	// Define a type alias for the callback function for convenience.
	using Callback = std::function<void()>;
	
	/**
	 * @brief Adds a clickable item to the menu.
	 * @param title The text to display for the menu item.
	 * @param callback The C++ lambda or function to execute when the item is clicked.
	 */
	void addItem(std::string title, Callback callback);
	
	/**
	 * @brief Adds a visual separator line to the menu.
	 */
	void addSeparator();
	
	/**
	 * @brief Displays the context menu on the screen at the current mouse position.
	 *
	 * This method will attempt to find the application's key window and present
	 * the menu from its main view. It does not require a view to be passed in.
	 */
	void show();
	
private:
	// Internal struct to represent a menu item or a separator.
	struct MenuItem {
		enum class Type { Item, Separator };
		Type type = Type::Item;
		std::string title;
		Callback callback;
	};
	
	// A list of all items and separators to be added to the menu.
	std::vector<MenuItem> items;
};
