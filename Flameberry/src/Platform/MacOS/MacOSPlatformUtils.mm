#include "Platform/PlatformUtils.h"

#import <Cocoa/Cocoa.h>
#import <objc/objc-class.h>

#import <string>

#define GLFW_EXPOSE_NATIVE_COCOA
#import <GLFW/glfw3native.h>

#include "Core/Core.h"
#include "Core/Application.h"

static std::function<void()> g_NewSceneCallback, g_SaveSceneCallback, g_SaveSceneAsCallback, g_OpenSceneCallback;

@interface MenuBar : NSObject
- (void) SaveSceneCallback;
- (void) SaveSceneAsCallback;
- (void) OpenSceneCallback;

- (void) DrawNative;
@end

@implementation MenuBar

- (void) NewSceneCallback { g_NewSceneCallback(); }
- (void) SaveSceneCallback { g_SaveSceneCallback(); }
- (void) SaveSceneAsCallback { g_SaveSceneAsCallback(); }
- (void) OpenSceneCallback { g_OpenSceneCallback(); }

- (void) DrawNative
{
    NSMenu *mainMenuBar = [[NSMenu alloc]init];
    NSMenuItem *menuBarItem = [[NSMenuItem alloc] init];
    [mainMenuBar addItem:menuBarItem];
    
    NSMenu *appMenu = [[NSMenu alloc]init];
    
    NSString* title = @"About Flameberry Engine";
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title action:@selector(setNilSymbol:) keyEquivalent:@"p"];
    [appMenu addItem:item];
    
    NSString* quitTitle = @"Quit Flameberry Engine";
    NSMenuItem* quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    
    [menuBarItem setSubmenu:appMenu];
    
    NSMenu* fileSubMenu = [[NSMenu alloc]initWithTitle:@"File"];
    NSMenuItem* fileMenu = [[NSMenuItem alloc]init];
    
    [mainMenuBar addItem:fileMenu];
    
    NSMenuItem* newItem = [[NSMenuItem alloc] initWithTitle:@"New Scene" action:@selector(NewSceneCallback) keyEquivalent:@"n"];
    [newItem setTarget:self];

    NSMenuItem* saveItem = [[NSMenuItem alloc] initWithTitle:@"Save Scene" action:@selector(SaveSceneCallback) keyEquivalent:@"s"];
    [saveItem setTarget:self];
    
    NSMenuItem* saveAsItem = [[NSMenuItem alloc] initWithTitle:@"Save Scene As..." action:@selector(SaveSceneAsCallback) keyEquivalent:@"S"];
    [saveAsItem setTarget:self];
    
    NSMenuItem* loadItem = [[NSMenuItem alloc] initWithTitle:@"Load Scene" action:@selector(OpenSceneCallback) keyEquivalent:@"o"];
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

@interface TitlebarView : NSView
- (void) drawRect:(NSRect)dirtyRect;
@end

@implementation TitlebarView
- (void) drawRect:(NSRect)dirtyRect
{
}
@end

namespace Flameberry {
    namespace platform {
        void UI_CustomTitleBar()
        {
            NSWindow* window = glfwGetCocoaWindow(Application::Get().GetWindow().GetGLFWwindow());
            window.titlebarAppearsTransparent = true; // gives it "flat" look
            window.backgroundColor = [NSColor colorWithRed:0.15f green:0.15f blue:0.15f alpha:1.0f]; // set the background color
        }
        
        void CreateCustomTitleBar()
        {
            NSWindow* window = glfwGetCocoaWindow(Application::Get().GetWindow().GetGLFWwindow());
            
            [window setStyleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable | NSWindowStyleMaskFullSizeContentView];
            [window setBackingType:NSBackingStoreBuffered];
            [window setTitleVisibility:NSWindowTitleHidden];
//
//            NSTextField *textField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 100, 22)];
//            [textField setStringValue:@"Flameberry Engine"];
//            [textField setTextColor:[NSColor whiteColor]];
//            [textField setBordered:NO];
//            [textField setDrawsBackground:NO];
//
//            NSView *accessoryView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, window.contentView.frame.size.width, 36)];
//            [accessoryView setWantsLayer:YES];
//            [accessoryView.layer setBackgroundColor:[[NSColor redColor] CGColor]];
//            [accessoryView addSubview:textField];
//            [accessoryView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
//
////            NSViewController *accessoryViewController = [[NSTitlebarAccessoryViewController alloc] init];
////            [accessoryViewController setView:accessoryView];
////            accessoryViewController.layoutAttribute = NSLayoutAttributeTop;
//
////            [window addTitlebarAccessoryViewController:accessoryViewController];
//            [window setTitlebarAppearsTransparent:YES];
//            [window.contentView addSubview:accessoryView];
//
//            [window makeKeyAndOrderFront:nil];

            // Create a custom view for the title bar
            NSView *titleBarView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 100, 22)];
            [titleBarView setWantsLayer:YES];
            [titleBarView.layer setBackgroundColor:[[NSColor redColor] CGColor]];
            
            // Get the superview of the content view
            NSView *themeFrame = [[window contentView] superview];
            
            // Get the first subview of the superview
            NSView *firstSubview = [[themeFrame subviews] objectAtIndex:0];
            
            // Set the autoresizing mask of the custom view
            [titleBarView setAutoresizingMask:(NSViewMinYMargin | NSViewWidthSizable)];
            
            // Add the custom view to the superview
            [themeFrame addSubview:titleBarView positioned:NSWindowAbove relativeTo:firstSubview];
        }
        
        void CreateMenuBar()
        {
            g_MenuBar = [[MenuBar alloc]init];
            [g_MenuBar DrawNative];
        }
        
        void SetNewSceneCallbackMenuBar(const std::function<void()>& callback)
        {
            g_NewSceneCallback = callback;
        }

        void SetSaveSceneCallbackMenuBar(const std::function<void()>& callback)
        {
            g_SaveSceneCallback = callback;
        }

        void SetSaveSceneAsCallbackMenuBar(const std::function<void()>& callback)
        {
            g_SaveSceneAsCallback = callback;
        }

        void SetOpenSceneCallbackMenuBar(const std::function<void()>& callback)
        {
            g_OpenSceneCallback = callback;
        }

        void OpenInExplorerOrFinder(const char* path)
        {
            NSString* _NSPath = [NSString stringWithFormat:@"file://%s", path];
            NSURL* url = [NSURL URLWithString:_NSPath];
            NSArray *fileURLs = [NSArray arrayWithObjects:url, nil];
            [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:fileURLs];
        }

        std::string OpenFile(const char* filter)
        { 
            NSWindow *keyWindow = [NSApp keyWindow];
            NSArray* URLs;
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            [panel setAllowsMultipleSelection : NO];
            [panel setResolvesAliases : YES];
            [panel setTreatsFilePackagesAsDirectories : YES];
            [panel setMessage : @"Select a scene file."];
            [panel setAllowsMultipleSelection: NO];
            [panel runModal];
            URLs = [panel URLs];
            [keyWindow makeKeyWindow];
            if ([URLs count])
                return std::string([[[URLs objectAtIndex:0] path] UTF8String]);        
            return "";    
        }

        std::string SaveFile(const char* filter)
        {
            NSSavePanel *panel = [NSSavePanel savePanel];
            NSString *fileName = @"Untitled.berry";
            [panel setMessage:@"Select a path to save the scene file"]; 

            // Message inside modal window        
            [panel setAllowsOtherFileTypes:YES];        
            [panel setExtensionHidden:YES];        
            [panel setCanCreateDirectories:YES];
            [panel setNameFieldStringValue:fileName];        
            [panel setTitle:@"Saving scene..."];
            NSInteger result = [panel runModal];
            NSError *error = nil;               
            if (result == NSModalResponseOK)
            { 
                NSString *path0 = [[panel URL] path];               
                if (error)             
                    [NSApp presentError:error];   
                else        
                    return std::string([path0 UTF8String]);    
            }       
            else           
                [panel close];   
            return "";    
        }
    }
}
