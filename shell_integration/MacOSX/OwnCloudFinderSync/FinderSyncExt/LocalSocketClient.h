//
//  LocalSocketClient.h
//  FinderSyncExt
//
//  Created by Claudio Cambra on 17/02/2022.
//

#import "LineProcessor.h"

#ifndef LocalSocketClient_h
#define LocalSocketClient_h
#define BUF_SIZE 4096

/// Class handling the (asynchronous) communication with a server over a local (UNIX) socket.
///
/// The implementation uses a `DispatchQueue` and `DispatchSource`s to handle asynchronous communication and thread
/// safety. All public/@objc function can be called from any thread/queue. The delegate that handles the
/// line-decoding is **not invoked on the UI thread**, but the (random) thread associated with the `DispatchQueue`!
/// If any UI work needs to be done, the class implementing the `LineProcessor` protocol should dispatch this work
/// on the main queue (so the UI thread) itself!
///
/// Other than the `init(withSocketPath:, lineProcessor)` and the `start()` method, all work is done "on the dispatch
/// queue". The `localSocketQueue` is a serial dispatch queue (so a maximum of 1, and only 1, task is run at any
/// moment), which guarantees safe access to instance variables. Both `askOnSocket(_:, query:)` and
/// `askForIcon(_:, isDirectory:)` will internally dispatch the work on the `DispatchQueue`.
///
/// Sending and receiving data to and from the socket, is handled by two `DispatchSource`s. These will run an event
/// handler when data can be read from resp. written to the socket. These handlers will also be run on the
/// `DispatchQueue`.

@interface LocalSocketClient : NSObject
{
    char inBuffer[BUF_SIZE];
    unsigned long inBufferCount;
    char outBuffer[BUF_SIZE];
    unsigned long outBufferCount;
}

@property NSString* socketPath;
@property LineProcessor* lineProcessor;
@property int sock;
@property dispatch_queue_t localSocketQueue;
@property dispatch_source_t readSource;
@property dispatch_source_t writeSource;

- (instancetype)init:(NSString*)socketPath lineProcessor:(LineProcessor*)lineProcessor;
- (BOOL)isConnected;
- (void)start;
- (void)restart;
- (void)closeConnection;
- (NSString*)strErr;
- (void)askOnSocket:(NSString*)path query:(NSString*)verb;
- (void)askForIcon:(NSString*)path isDirectory:(BOOL)isDirectory;
- (void)readFromSocket;
- (void)writeToSocket;
- (void)processInBuffer;

@end
#endif /* LocalSocketClient_h */
