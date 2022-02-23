//
//  LocalSocketClient.m
//  FinderSyncExt
//
//  Created by Claudio Cambra on 17/02/2022.
//

#import <Foundation/Foundation.h>
#import "LocalSocketClient.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>

@implementation LocalSocketClient

- (instancetype)init:(NSString*)socketPath lineProcessor:(LineProcessor*)lineProcessor
{
    NSLog(@"Initiating local socket client.");
    self = [super init];
    
    if(self) {
        self->inBufferCount = 0;
        self->outBufferCount = 0;
        
        self.socketPath = socketPath;
        self.lineProcessor = lineProcessor;
        
        self.sock = -1;
        self.localSocketQueue = dispatch_queue_create("localSocketQueue", DISPATCH_QUEUE_SERIAL);
    }
        
    return self;
}

- (BOOL)isConnected
{
    NSLog(@"Checking is connected: %@", self.sock != -1 ? @"YES" : @"NO");
    return self.sock != -1;
}

- (void)start
{
    if([self isConnected]) {
        NSLog(@"Socket client already connected. Not starting.");
        return;
    }
    
    struct sockaddr_un localSocketAddr;
    unsigned long socketPathByteCount = [self.socketPath lengthOfBytesUsingEncoding:NSUTF8StringEncoding]; // add 1 for the NUL terminator char
    int maxByteCount = sizeof(localSocketAddr.sun_path);
    
    if(socketPathByteCount > maxByteCount) {
        // LOG THAT THE SOCKET PATH IS TOO LONG HERE
        NSLog(@"Socket path %@ is too long: maximum socket path length is %i, this path is of length %lu", self.socketPath, maxByteCount, socketPathByteCount);
        return;
    }
    
    NSLog(@"Opening local socket...");
    
    // LOG THAT THE SOCKET IS BEING OPENED HERE
    self.sock = socket(AF_LOCAL, SOCK_STREAM, 0);
    
    if(self.sock == -1) {
        NSLog(@"Cannot open socket: %@", [self strErr]);
        [self restart];
        return;
    }
    
    NSLog(@"Local socket opened. Connecting to %@ ...", self.socketPath);
    
    localSocketAddr.sun_family = AF_LOCAL & 0xff;
    
    const char* pathBytes = [self.socketPath UTF8String];
    strcpy(localSocketAddr.sun_path, pathBytes);
    
    int connectionStatus = connect(self.sock, (struct sockaddr*)&localSocketAddr, sizeof(localSocketAddr));
    
    if(connectionStatus == -1) {
        NSLog(@"Could not connect to %@: %@", self.socketPath, [self strErr]);
        [self restart];
        return;
    }
    
    int flags = fcntl(self.sock, F_GETFL, 0);
    
    if(fcntl(self.sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        NSLog(@"Could not set socket to non-blocking mode: %@", [self strErr]);
        [self restart];
        return;
    }
    
    NSLog(@"Connected to socket. Setting up dispatch sources...");
    
    self.readSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, self.sock, 0, self.localSocketQueue);
    dispatch_source_set_event_handler(self.readSource, ^(void){ [self readFromSocket]; });
    dispatch_source_set_cancel_handler(self.readSource, ^(void){
        self.readSource = nil;
        [self closeConnection];
    });
    
    self.writeSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, self.sock, 0, self.localSocketQueue);
    dispatch_source_set_event_handler(self.writeSource, ^(void){ [self writeToSocket]; });
    dispatch_source_set_cancel_handler(self.writeSource, ^(void){
        self.writeSource = nil;
        [self closeConnection];
    });
    
    // These dispatch sources are suspended upon creation.
    // We resume the writeSource when we actually have something to write, suspending it again once our outBuffer is empty.
    // We start the readSource now.
    
    NSLog(@"Starting to read from socket");
    
    dispatch_resume(self.readSource);
    [self askOnSocket:@"" query:@"GET_STRINGS"];
}

- (void)restart
{
    NSLog(@"Restarting connection to socket.");
    [self closeConnection];
    dispatch_async(dispatch_get_main_queue(), ^(void){
        [NSTimer scheduledTimerWithTimeInterval:5 repeats:NO block:^(NSTimer* timer) {
            [self start];
        }];
    });
}

- (void)closeConnection
{
    NSLog(@"Closing connection.");
    dispatch_source_cancel(self.readSource);
    dispatch_source_cancel(self.writeSource);
    self.readSource = nil;
    self.writeSource = nil;
    memset(self->inBuffer, 0, BUF_SIZE);
    self->inBufferCount = 0;
    memset(self->outBuffer, 0, BUF_SIZE);
    self->outBufferCount = 0;
    
    if(self.sock != -1) {
        close(self.sock);
        self.sock = -1;
    }
}

- (NSString*)strErr
{
    int err = errno;
    const char *errStr = strerror(err);
    NSString *errorStr = [NSString stringWithUTF8String:errStr];
    
    if([errorStr length] == 0) {
        return errorStr;
    } else {
        return [NSString stringWithFormat:@"Unknown error code: %i", err];
    }
}

- (void)askOnSocket:(NSString *)path query:(NSString *)verb
{
    NSString *line = [NSString stringWithFormat:@"%@:%@\n", verb, path];
    dispatch_async(self.localSocketQueue, ^(void) {
        if(![self isConnected]) {
            return;
        }
        
        BOOL writeSourceIsSuspended = self->outBufferCount <= 0;
        
        const char* utf8Line = [line UTF8String];
        strcpy(self->outBuffer + self->outBufferCount, utf8Line);
        self->outBufferCount += sizeof(utf8Line);
        
        NSLog(@"Writing to out buffer: %@", [NSString stringWithUTF8String:utf8Line]);
        NSLog(@"Out buffer now %li bytes", self->outBufferCount);
        
        if(writeSourceIsSuspended) {
            NSLog(@"Resuming write dispatch source.");
            dispatch_resume(self.writeSource);
        }
    });
}

- (void)writeToSocket
{
    if(![self isConnected]) {
        return;
    }
    
    if(self->outBufferCount <= 0) {
        NSLog(@"Empty out buffer, suspending write dispatch source.");
        dispatch_suspend(self.writeSource);
        return;
    }
    
    NSLog(@"About to write %li bytes from outbuffer to socket.", self->outBufferCount);
    
    for(int i = 0; i < self->outBufferCount; i++) {
        NSLog(@"Writing: %c", self->outBuffer[i]);
    }
    
    long bytesWritten = write(self.sock, self->outBuffer, self->outBufferCount);
    char lineWritten[4096];
    memcpy(lineWritten, self->outBuffer, self->outBufferCount);
    NSLog(@"Wrote %li bytes to socket. Line was: %@", bytesWritten, [NSString stringWithUTF8String:lineWritten]);
    
    if(bytesWritten == 0) {
        // 0 means we reached "end of file" and thus the socket was closed. So let's restart it
        NSLog(@"Socket was closed. Restarting...");
        [self restart];
    } else if(bytesWritten == -1) {
        int err = errno; // Make copy before it gets nuked by something else
        
        if(err == EAGAIN || err == EWOULDBLOCK)  {
            // No free space in the OS' buffer, nothing to do here
            NSLog(@"No free space in OS buffer. Ending write.");
            return;
        } else {
            NSLog(@"Error writing to local socket: %@", [self strErr]);
            [self restart];
        }
    } else if(bytesWritten > 0) {
        memset(self->outBuffer, 0, bytesWritten);
        self->outBufferCount -= bytesWritten;
        NSLog(@"Out buffer cleared. Now count is %li bytes.", self->outBufferCount);
        
        if(self->outBufferCount == 0) {
            NSLog(@"Out buffer has been emptied, suspending write dispatch source.");
            dispatch_suspend(self.writeSource);
        } else {
            // Shift everything to beginning of buffer array
            for(long i = bytesWritten, j = 0; i < BUF_SIZE; i++, j++) {
                self->outBuffer[j] = self->outBuffer[i];
                self->outBuffer[i] = 0;
            }
        }
    }
}

- (void)askForIcon:(NSString*)path isDirectory:(BOOL)isDirectory;
{
    NSLog(@"Asking for icon.");
    
    NSString *verb;
    if(isDirectory) {
        verb = @"RETRIEVE_FOLDER_STATUS";
    } else {
        verb = @"RETRIEVE_FILE_STATUS";
    }
    
    [self askOnSocket:path query:verb];
}

- (void)readFromSocket
{
    if(![self isConnected]) {
        return;
    }
    
    NSLog(@"Reading from socket.");
    
    int bufferLength = BUF_SIZE / 2;
    char buffer[bufferLength];
    
    while(true) {
        long bytesRead = read(self.sock, buffer, bufferLength);
        
        NSLog(@"Read %li bytes from socket.", bytesRead);
        
        if(bytesRead == 0) {
            // 0 means we reached "end of file" and thus the socket was closed. So let's restart it
            NSLog(@"Socket was closed. Restarting...");
            [self restart];
            return;
        } else if(bytesRead == -1) {
            int err = errno;
            if(err == EAGAIN) {
                NSLog(@"No error and no data. Stopping.");
                return; // No error, no data, so let's stop
            } else {
                NSLog(@"Error reading from local socket: %@", [self strErr]);
                [self closeConnection];
                return;
            }
        } else {
            memcpy(self->inBuffer + self->inBufferCount, buffer, bytesRead);
            self->inBufferCount += bytesRead;
            [self processInBuffer];
        }
    }
}

- (void)processInBuffer
{
    NSLog(@"Processing in buffer.");
    UInt8 separator = 0xa; // Byte value for "\n"
    while(true) {
        int firstSeparatorIndex = -1;
        
        for(int i = 0; i < BUF_SIZE; i++) {
            if(self->inBuffer[i] == separator) {
                firstSeparatorIndex = i;
                break;
            }
        }
        
        if(firstSeparatorIndex == -1) {
            NSLog(@"No separator found. Stopping.");
            return; // No separator, nope out
        } else {
            self->inBuffer[firstSeparatorIndex] = 0; // Add NULL terminator, so we can use C string methods
            NSString *newLine = [NSString stringWithUTF8String:self->inBuffer];
            
            int numChars = firstSeparatorIndex + 1;
            memset(self->inBuffer, 0, numChars);
            self->inBufferCount -= numChars;
            // Shift everything to beginning of buffer array
            for(long i = numChars, j = 0; i < BUF_SIZE; i++, j++) {
                self->inBuffer[j] = self->inBuffer[i];
                self->inBuffer[i] = 0;
            }
            
            [self.lineProcessor process:newLine];
        }
    }
}
    
@end
