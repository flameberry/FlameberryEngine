#include "Platform/PlatformUtils.h"

#import <Cocoa/Cocoa.h>

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

static NSView* g_TitleBarView;

namespace Flameberry {

    namespace Platform {

        struct TitlebarData
        {
            NSView* TitlebarView;
            NSTextView* PrimaryTitleTextView;
            NSTextView* SecondaryTitleTextView;
        };

        static TitlebarData s_TitlebarData;
        
        void TitlebarNative::CreateForGLFWwindow(GLFWwindow* window, float titleBarHeight)
        {
            NSWindow* nativeWindow = glfwGetCocoaWindow(window);
            NSView* nativeView = glfwGetCocoaView(window);
            
            [nativeWindow setTitlebarAppearsTransparent:YES];
            nativeWindow.titleVisibility = NSWindowTitleHidden;
            
            NSWindowStyleMask windowMask = NSWindowStyleMaskFullSizeContentView | NSWindowStyleMaskBorderless | NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
            [nativeWindow setStyleMask: windowMask];
            
            // This line is a 'hack' to shift the traffic lights down to match the bigger custom title bar size
            nativeWindow.toolbar = [NSToolbar new]; // <-- Note this is `new` and not `init`
            // nativeWindow.toolbarStyle = NSWindowToolbarStyleUnified;
            
            // Remove the whitish border from default macos window
            [nativeWindow setOpaque:NO];
            [nativeWindow setBackgroundColor:[NSColor clearColor]];
            
            // Create a title bar view
            s_TitlebarData.TitlebarView = [[NSView alloc] init];
            
            // Customize the appearance of the title bar
            [s_TitlebarData.TitlebarView setBackgroundColor:[NSColor colorWithRed:28.0f/255.0f green:28.0f/255.0f blue:28.0f/255.0f alpha:1.0f]]; // Set the background color
            
            // Icon
            NSImage* iconImage = [[NSImage alloc] initWithContentsOfFile:@"/Users/flameberry/Developer/FlameberryEngine/FlameberryEditor/Assets/Icons/FlameberryIcon.png"];
            
            if (iconImage)
            {
                // Resize the image if needed
                [iconImage setSize:NSMakeSize(30, 30)]; // Adjust the size as per your requirement
            }
            
            // Icon Button
            NSButton* iconButton = [[NSButton alloc] initWithFrame:NSMakeRect(75, 5, 30, 30)]; // Adjust the frame as needed
            [iconButton setImage:iconImage];
            [iconButton setButtonType:NSButtonTypeMomentaryPushIn];
            [iconButton setBordered:NO];
            
            [s_TitlebarData.TitlebarView addSubview:iconButton];
            
            NSFont *defaultUIFont = [NSFont systemFontOfSize:NSFont.systemFontSize];

            // Add Primary Title
            s_TitlebarData.PrimaryTitleTextView = [[NSTextView alloc] initWithFrame:NSMakeRect(110, titleBarHeight / 2 - 12, 200, 20)];
            s_TitlebarData.PrimaryTitleTextView.string = @"Flameberry Engine";
            s_TitlebarData.PrimaryTitleTextView.font = defaultUIFont;
            s_TitlebarData.PrimaryTitleTextView.backgroundColor = [NSColor clearColor];
            [s_TitlebarData.TitlebarView addSubview:s_TitlebarData.PrimaryTitleTextView];
            
            // Project Title
            s_TitlebarData.SecondaryTitleTextView = [[NSTextView alloc] initWithFrame:NSMakeRect(1100, 8, 150, 20)];
            s_TitlebarData.SecondaryTitleTextView.string = @"";
            s_TitlebarData.SecondaryTitleTextView.font = defaultUIFont;
            s_TitlebarData.SecondaryTitleTextView.backgroundColor = [NSColor clearColor];
            [s_TitlebarData.TitlebarView addSubview:s_TitlebarData.SecondaryTitleTextView];
            
            // Add the title bar view to the NSWindow's content view
            [[nativeWindow contentView] addSubview:s_TitlebarData.TitlebarView];
            
            InvalidateFrameAndContentFrameRect(window, titleBarHeight);
        }
        
        void TitlebarNative::InvalidateFrameAndContentFrameRect(GLFWwindow* window, float titleBarHeight)
        {
            NSWindow* nativeWindow = glfwGetCocoaWindow(window);
            NSView* nativeView = glfwGetCocoaView(window);
            
            CGFloat windowWidth = NSWidth(nativeWindow.frame);
            CGFloat windowHeight = NSHeight(nativeWindow.frame);
            
            // Calculate the y-coordinate of the title bar
            CGFloat contentHeight = windowHeight - titleBarHeight;
            
            NSRect contentRect = NSMakeRect(0, 0, windowWidth, contentHeight);
            [nativeView setFrame:contentRect];
            
            // Create a custom NSView for the title bar
            NSRect titleBarRect = NSMakeRect(0, contentHeight, windowWidth, titleBarHeight);
            [s_TitlebarData.TitlebarView setFrame:titleBarRect];
            [s_TitlebarData.TitlebarView setWantsLayer:YES]; // Enable layer-backed views for better performance or custom drawing
            
            // Reevaluate position of Secondary Title bar, because of it being on the rightmost end of the title bar
            CGFloat spacing = 5.0f, secondaryTitleWidth = NSWidth(s_TitlebarData.SecondaryTitleTextView.frame), secondaryTitleHeight = NSHeight(s_TitlebarData.SecondaryTitleTextView.frame);
            NSRect secondaryTitleRect = NSMakeRect(windowWidth - spacing - secondaryTitleWidth, titleBarHeight / 2.0f - secondaryTitleHeight / 2.0f, secondaryTitleWidth, secondaryTitleHeight);
            [s_TitlebarData.SecondaryTitleTextView setFrame:secondaryTitleRect];
        }
        
        void TitlebarNative::SetPrimaryTitle(const std::string& title)
        {
            [s_TitlebarData.PrimaryTitleTextView setString:[NSString stringWithUTF8String:title.c_str()]];
        }
        
        void TitlebarNative::SetSecondaryTitle(const std::string& secondaryTitle)
        {
            [s_TitlebarData.SecondaryTitleTextView setString:[NSString stringWithUTF8String:secondaryTitle.c_str()]];
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
        
        std::string OpenFolder()
        {
            NSWindow *keyWindow = [NSApp keyWindow];
            NSArray* URLs;
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            
            [panel setCanChooseFiles:NO];
            [panel setCanChooseDirectories:YES];
            
            [panel setAllowsMultipleSelection : NO];
            [panel setResolvesAliases : YES];
            [panel setTreatsFilePackagesAsDirectories : YES];
            [panel setMessage : @"Select a folder."];
            [panel runModal];
            URLs = [panel URLs];
            [keyWindow makeKeyWindow];
            if ([URLs count])
                return std::string([[[URLs objectAtIndex:0] path] UTF8String]);
            return "";
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
