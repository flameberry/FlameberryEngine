#include "FileDialog.h"

#include <Cocoa/Cocoa.h>

#include <iostream>

namespace Flameberry {
    std::string FileDialog::OpenDialog()
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
    
    std::string FileDialog::SaveDialog()
    {
        NSSavePanel *panel = [NSSavePanel savePanel];
        NSString *fileName = @"untitled.berry";
        
        [panel setMessage:@"Select a path to save the scene file"]; // Message inside modal window
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
