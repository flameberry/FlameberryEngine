#include "Platform/PlatformUtils.h"

#import <Cocoa/Cocoa.h>

#import <string>

#define GLFW_EXPOSE_NATIVE_COCOA
#import <GLFW/glfw3native.h>

#include "Core/Application.h"
#include "Core/Core.h"

static std::function<void()> g_NewSceneCallback, g_SaveSceneCallback, g_SaveSceneAsCallback, g_OpenSceneCallback;

@interface MenuBar : NSObject
- (void)SaveSceneCallback;
- (void)SaveSceneAsCallback;
- (void)OpenSceneCallback;

- (void)DrawNative;
@end

@implementation MenuBar

- (void)NewSceneCallback {
	g_NewSceneCallback();
}
- (void)SaveSceneCallback {
	g_SaveSceneCallback();
}
- (void)SaveSceneAsCallback {
	g_SaveSceneAsCallback();
}
- (void)OpenSceneCallback {
	g_OpenSceneCallback();
}

- (void)DrawNative {
	NSMenu* mainMenuBar = [[NSMenu alloc] init];
	NSMenuItem* menuBarItem = [[NSMenuItem alloc] init];
	[mainMenuBar addItem:menuBarItem];

	NSMenu* appMenu = [[NSMenu alloc] init];

	NSString* title = @"About Flameberry Engine";
	NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title action:@selector(setNilSymbol:) keyEquivalent:@"p"];
	[appMenu addItem:item];

	NSString* quitTitle = @"Quit Flameberry Engine";
	NSMenuItem* quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle
														  action:@selector(terminate:)
												   keyEquivalent:@"q"];
	[appMenu addItem:quitMenuItem];

	[menuBarItem setSubmenu:appMenu];

	NSMenu* fileSubMenu = [[NSMenu alloc] initWithTitle:@"File"];
	NSMenuItem* fileMenu = [[NSMenuItem alloc] init];

	[mainMenuBar addItem:fileMenu];

	NSMenuItem* newItem = [[NSMenuItem alloc] initWithTitle:@"New Scene"
													 action:@selector(NewSceneCallback)
											  keyEquivalent:@"n"];
	[newItem setTarget:self];

	NSMenuItem* saveItem = [[NSMenuItem alloc] initWithTitle:@"Save Scene"
													  action:@selector(SaveSceneCallback)
											   keyEquivalent:@"s"];
	[saveItem setTarget:self];

	NSMenuItem* saveAsItem = [[NSMenuItem alloc] initWithTitle:@"Save Scene As..."
														action:@selector(SaveSceneAsCallback)
												 keyEquivalent:@"S"];
	[saveAsItem setTarget:self];

	NSMenuItem* loadItem = [[NSMenuItem alloc] initWithTitle:@"Load Scene"
													  action:@selector(OpenSceneCallback)
											   keyEquivalent:@"o"];
	[loadItem setTarget:self];

	[fileSubMenu addItem:newItem];
	[fileSubMenu addItem:saveItem];
	[fileSubMenu addItem:saveAsItem];
	[fileSubMenu addItem:loadItem];

	[fileMenu setSubmenu:fileSubMenu];
	[NSApp setMainMenu:mainMenuBar];
}
@end

static MenuBar* g_MenuBar;

namespace Flameberry {

	namespace Platform {

		void TitlebarNative::CreateForGLFWwindow(GLFWwindow* window, float titleBarHeight) {
			NSWindow* nativeWindow = glfwGetCocoaWindow(window);

			[nativeWindow setTitlebarAppearsTransparent:YES];
			nativeWindow.titleVisibility = NSWindowTitleHidden;

			NSWindowStyleMask windowMask = NSWindowStyleMaskFullSizeContentView | NSWindowStyleMaskBorderless
				| NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable
				| NSWindowStyleMaskResizable;

			[nativeWindow setStyleMask:windowMask];

			// This line is a 'hack' to shift the traffic lights down to match the bigger
			// custom title bar size
			nativeWindow.toolbar = [NSToolbar new]; // <-- Note this is `new` and not `init`
			nativeWindow.toolbarStyle = NSWindowToolbarStyleUnifiedCompact;

			// Remove the whitish border from default macos window
			[nativeWindow setOpaque:NO];
			[nativeWindow setBackgroundColor:[NSColor clearColor]];

			InvalidateFrameAndContentFrameRect(window, titleBarHeight);
		}

		static bool IsWindowFullScreen(NSWindow* window) {
			return ([window styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen;
		}

		void TitlebarNative::InvalidateFrameAndContentFrameRect(GLFWwindow* window, float titleBarHeight) {
			NSWindow* nativeWindow = glfwGetCocoaWindow(window);
			if (IsWindowFullScreen(nativeWindow)) {
				[nativeWindow.toolbar setVisible:false];
			} else {
				[nativeWindow.toolbar setVisible:true];
			}
		}

		void TitlebarNative::SetGradient(const glm::vec4& color) {}

		void TitlebarNative::SetPrimaryTitle(const std::string& title) {}

		void TitlebarNative::SetSecondaryTitle(const std::string& secondaryTitle) {}

		void CreateMenuBar() {
			g_MenuBar = [[MenuBar alloc] init];
			[g_MenuBar DrawNative];
		}

		void SetNewSceneCallbackMenuBar(const std::function<void()>& callback) {
			g_NewSceneCallback = callback;
		}

		void SetSaveSceneCallbackMenuBar(const std::function<void()>& callback) {
			g_SaveSceneCallback = callback;
		}

		void SetSaveSceneAsCallbackMenuBar(const std::function<void()>& callback) {
			g_SaveSceneAsCallback = callback;
		}

		void SetOpenSceneCallbackMenuBar(const std::function<void()>& callback) {
			g_OpenSceneCallback = callback;
		}

		void OpenInExplorerOrFinder(const char* path) {
			NSString* _NSPath = [NSString stringWithFormat:@"file://%s", path];
			NSURL* url = [NSURL URLWithString:_NSPath];
			NSArray* fileURLs = [NSArray arrayWithObjects:url, nil];
			[[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:fileURLs];
		}

		std::string OpenFolder() {
			NSWindow* keyWindow = [NSApp keyWindow];
			NSArray* URLs;
			NSOpenPanel* panel = [NSOpenPanel openPanel];

			[panel setCanChooseFiles:NO];
			[panel setCanChooseDirectories:YES];

			[panel setAllowsMultipleSelection:NO];
			[panel setResolvesAliases:YES];
			[panel setTreatsFilePackagesAsDirectories:YES];
			[panel setMessage:@"Select a folder."];
			[panel runModal];
			URLs = [panel URLs];
			[keyWindow makeKeyWindow];
			if ([URLs count]) return std::string([[[URLs objectAtIndex:0] path] UTF8String]);
			return "";
		}

		std::string OpenFile(const char* filter) {
			NSWindow* keyWindow = [NSApp keyWindow];
			NSArray* URLs;
			NSOpenPanel* panel = [NSOpenPanel openPanel];

			[panel setAllowsMultipleSelection:NO];
			[panel setResolvesAliases:YES];
			[panel setTreatsFilePackagesAsDirectories:YES];
			[panel setMessage:@"Select a scene file."];
			[panel setAllowsMultipleSelection:NO];
			[panel runModal];
			URLs = [panel URLs];
			[keyWindow makeKeyWindow];
			if ([URLs count]) return std::string([[[URLs objectAtIndex:0] path] UTF8String]);
			return "";
		}

		std::string SaveFile(const char* filter) {
			NSSavePanel* panel = [NSSavePanel savePanel];
			NSString* fileName = @"Untitled.berry";
			[panel setMessage:@"Select a path to save the scene file"];

			// Message inside modal window
			[panel setAllowsOtherFileTypes:YES];
			[panel setExtensionHidden:YES];
			[panel setCanCreateDirectories:YES];
			[panel setNameFieldStringValue:fileName];
			[panel setTitle:@"Saving scene..."];
			NSInteger result = [panel runModal];
			NSError* error = nil;
			if (result == NSModalResponseOK) {
				NSString* path0 = [[panel URL] path];
				if (error) [NSApp presentError:error];
				else
					return std::string([path0 UTF8String]);
			} else
				[panel close];
			return "";
		}

	} // namespace Platform

} // namespace Flameberry
