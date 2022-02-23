//
//  LineProcessor.m
//  FinderSyncExt
//
//  Created by Claudio Cambra on 17/02/2022.
//

#import <Foundation/Foundation.h>
#import "LineProcessor.h"

@implementation LineProcessor

-(instancetype)initWithDelegate:(id<SyncClientDelegate>)delegate
{
    NSLog(@"Init line processor with delegate.");
    self.delegate = delegate;
    return self;
}

-(void)process:(NSString*)line
{
    NSLog(@"Processing line: %@", line);
    NSArray *split = [line componentsSeparatedByString:@":"];
    NSString *command = [split objectAtIndex:0];
    
    NSLog(@"Command: %@", command);
    
    if([command isEqualToString:@"STATUS"]) {
        NSString *result = [split objectAtIndex:1];
        NSArray *pathSplit = [split subarrayWithRange:NSMakeRange(2, [split count] - 1)];
        NSString *path = [pathSplit componentsJoinedByString:@":"];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"Setting result %@ for path %@", result, path);
            [self.delegate setResultForPath:path result:result];
        });
    } else if([command isEqualToString:@"UPDATE_VIEW"]) {
        NSString *path = [split objectAtIndex:1];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"Re-fetching filename cache for path %@", path);
            [self.delegate reFetchFileNameCacheForPath:path];
        });
    } else if([command isEqualToString:@"REGISTER_PATH"]) {
        NSString *path = [split objectAtIndex:1];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"Registering path %@", path);
            [self.delegate registerPath:path];
        });
    } else if([command isEqualToString:@"UNREGISTER_PATH"]) {
        NSString *path = [split objectAtIndex:1];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"Unregistering path %@", path);
            [self.delegate unregisterPath:path];
        });
    } else if([command isEqualToString:@"GET_STRINGS"]) {
        // BEGIN and END messages, do nothing.
        return;
    } else if([command isEqualToString:@"STRING"]) {
        NSString *key = [split objectAtIndex:1];
        NSString *value = [split objectAtIndex:2];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"Setting string %@ to value %@", key, value);
            [self.delegate setString:key value:value];
        });
    } else if([command isEqualToString:@"GET_MENU_ITEMS"]) {
        if([[split objectAtIndex:1] isEqualToString:@"BEGIN"]) {
            dispatch_async(dispatch_get_main_queue(), ^{
                NSLog(@"Resetting menu items.");
                [self.delegate resetMenuItems];
            });
        } else {
            NSLog(@"Emitting menu has completed signal.");
            [self.delegate menuHasCompleted];
        }
    } else if([command isEqualToString:@"MENU_ITEM"]) {
        NSDictionary *item = @{@"command": [split objectAtIndex:1], @"flags": [split objectAtIndex:2], @"text": [split objectAtIndex:3]};
        
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"Adding menu item with command %@, flags %@, and text %@", [split objectAtIndex:1], [split objectAtIndex:2], [split objectAtIndex:3]);
            [self.delegate addMenuItem:item];
        });
    } else {
        // LOG UNKOWN COMMAND
        NSLog(@"Unkown command: %@", command);
    }
}

@end
