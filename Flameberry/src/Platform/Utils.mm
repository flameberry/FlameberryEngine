#include "Utils.h"

#include <Cocoa/Cocoa.h>
#include <string>

namespace Flameberry {
    namespace platform { 
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
