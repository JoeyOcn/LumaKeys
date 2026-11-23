// Ayudas de ventanas solo para Mac (Objective-C++)
#import <Cocoa/Cocoa.h>

// Sube la ventana (por ejemplo el piano roll en pantalla completa) por encima de las
// ventanas flotantes de los plugins del DAW, o la regresa a su nivel normal de "siempre al frente".
void midicatSetWindowAbove (void* nsViewHandle, bool above)
{
    NSView* view = (__bridge NSView*) nsViewHandle;
    if (view == nil) return;
    NSWindow* w = [view window];
    if (w == nil) return;
    const NSInteger level = above ? (NSFloatingWindowLevel + 2) : NSFloatingWindowLevel;
    if ([w level] != level)
    {
        [w setLevel: level];
        if (above) [w orderFrontRegardless];
    }
}

bool midicatIsWindowFullScreen (void* nsViewHandle)
{
    NSView* view = (__bridge NSView*) nsViewHandle;
    if (view == nil) return false;
    NSWindow* w = [view window];
    return w != nil && ([w styleMask] & NSWindowStyleMaskFullScreen) != 0;
}
