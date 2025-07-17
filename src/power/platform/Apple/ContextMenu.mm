#import <AppKit/AppKit.h>
#include "ContextMenu.hpp"
#include <utility> // For std::move

/**
 * @interface ContextMenuActionTarget
 * @brief A helper Objective-C class to act as a target for NSMenuItem.
 *
 * NSMenuItem requires its target to be an Objective-C object that responds
 * to a selector. This class holds a C++ std::function and executes it
 * when its -performAction: method is called by the menu item.
 */
@interface ContextMenuActionTarget : NSObject

// Use 'assign' for the C++ object. Its lifetime is managed by the ContextMenu::show method.
@property (nonatomic, assign) std::function<void()> callback;

- (instancetype)initWithCallback:(std::function<void()>)cb;
- (IBAction)performAction:(id)sender;

@end

@implementation ContextMenuActionTarget

- (instancetype)initWithCallback:(std::function<void()>)cb {
	self = [super init];
	if (self) {
		// Move the callback into the property.
		_callback = std::move(cb);
	}
	return self;
}

- (IBAction)performAction:(id)sender {
	// If the callback is valid, execute it.
	if (self.callback) {
		self.callback();
	}
}

@end


// --- Implementation of the C++ ContextMenu class ---

void ContextMenu::addItem(std::string title, Callback callback) {
	items.push_back({MenuItem::Type::Item, std::move(title), std::move(callback)});
}

void ContextMenu::addSeparator() {
	items.push_back({MenuItem::Type::Separator, "", nullptr});
}

void ContextMenu::show() {
	// Find the current key window and its content view. The key window is the one
	// that is currently accepting user input.
	NSWindow* keyWindow = [NSApp keyWindow];
	if (!keyWindow) {
		// If there's no key window, we can't show a menu. This can happen if the
		// app is not in the foreground or has no visible windows.
		NSLog(@"ContextMenu::show failed: No key window available.");
		return;
	}
	NSView* view = [keyWindow contentView];
	if (!view) {
		NSLog(@"ContextMenu::show failed: Key window has no content view.");
		return;
	}
	
	// The NSMenu that will be displayed.
	NSMenu* menu = [[NSMenu alloc] init];
	
	// This array is CRUCIAL. It holds strong references to the target objects.
	// Without it, the ContextMenuActionTarget instances would be deallocated
	// before the user could click a menu item, leading to a crash.
	NSMutableArray* targets = [NSMutableArray array];
	
	for (const auto& item : items) {
		if (item.type == MenuItem::Type::Separator) {
			[menu addItem:[NSMenuItem separatorItem]];
		} else {
			// 1. Create the Objective-C target that will hold our C++ lambda.
			ContextMenuActionTarget* target = [[ContextMenuActionTarget alloc] initWithCallback:item.callback];
			[targets addObject:target]; // Keep the target alive!
			
			// 2. Create the NSMenuItem using the C++ item's title.
			NSString* title = [NSString stringWithUTF8String:item.title.c_str()];
			NSMenuItem* menuItem = [[NSMenuItem alloc] initWithTitle:title
															  action:@selector(performAction:)
													   keyEquivalent:@""];
			// 3. Set the target of the menu item to our helper object.
			[menuItem setTarget:target];
			
			// 4. Add the fully configured item to the menu.
			[menu addItem:menuItem];
		}
	}
	
	// Get the mouse event that triggered this. This ensures the menu appears at the cursor.
	NSEvent* event = [NSApp currentEvent];
	
	// If show() was called not from a mouse down event, create a synthetic event
	// at the current mouse location to position the menu correctly.
	if (!event || (event.type != NSEventTypeRightMouseDown && event.type != NSEventTypeLeftMouseDown)) {
		NSPoint mouseLocation = [NSEvent mouseLocation];
		event = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown
								   location:mouseLocation
							  modifierFlags:0
								  timestamp:0
							   windowNumber:[view.window windowNumber]
									context:nil
								eventNumber:0
								 clickCount:1
								   pressure:0];
	}
	
	
	// Finally, display the context menu. This call is blocking until the user makes a selection or dismisses the menu.
	[NSMenu popUpContextMenu:menu withEvent:event forView:view];
}
