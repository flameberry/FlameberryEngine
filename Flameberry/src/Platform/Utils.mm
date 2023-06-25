#include "Utils.h"

#import <Cocoa/Cocoa.h>
#import <objc/objc-class.h>

#import <string>

#include "Core/Core.h"

static std::function<void()> g_SaveSceneCallback, g_SaveSceneAsCallback, g_OpenSceneCallback;

@interface MenuBar : NSObject
- (void) SaveSceneCallback;
- (void) SaveSceneAsCallback;
- (void) OpenSceneCallback;

- (void) DrawNative;
@end

@implementation MenuBar

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
    
    NSMenuItem* saveItem = [[NSMenuItem alloc] initWithTitle:@"Save Scene" action:@selector(SaveSceneCallback) keyEquivalent:@"s"];
    [saveItem setTarget:self];
    
    NSMenuItem* saveAsItem = [[NSMenuItem alloc] initWithTitle:@"Save Scene As..." action:@selector(SaveSceneAsCallback) keyEquivalent:@"S"];
    [saveAsItem setTarget:self];
    
    NSMenuItem* loadItem = [[NSMenuItem alloc] initWithTitle:@"Load Scene" action:@selector(OpenSceneCallback) keyEquivalent:@"o"];
    [loadItem setTarget:self];
    
    [fileSubMenu addItem:saveItem];
    [fileSubMenu addItem:saveAsItem];
    [fileSubMenu addItem:loadItem];
    
    [fileMenu setSubmenu:fileSubMenu];
    [NSApp setMainMenu:mainMenuBar];
}
@end

static MenuBar* g_MenuBar;

namespace Flameberry {
    namespace platform {
        void DrawNativeMacOSMenuBar()
        {
            g_MenuBar = [[MenuBar alloc]init];
            [g_MenuBar DrawNative];
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

        void OpenInFinder(const char* path)
        {
            NSString* _NSPath = [NSString stringWithFormat:@"file://%s", path];
            NSURL* url = [NSURL URLWithString:_NSPath];
            NSArray *fileURLs = [NSArray arrayWithObjects:url, nil];
            [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:fileURLs];
        }

        std::string OpenDialog()
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

        std::string SaveDialog()
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
