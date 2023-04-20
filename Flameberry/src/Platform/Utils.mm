#include "Utils.h"

#include <Cocoa/Cocoa.h>
#include <string>

namespace Flameberry {
    namespace Platform { 
        void OpenInFinder(const char* path)
        {
            NSString* _NSPath = [NSString stringWithFormat:@"file://%s", path];
            NSURL* url = [NSURL URLWithString:_NSPath];
            NSArray *fileURLs = [NSArray arrayWithObjects:url, nil];
            [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:fileURLs];
        }
    }
}
