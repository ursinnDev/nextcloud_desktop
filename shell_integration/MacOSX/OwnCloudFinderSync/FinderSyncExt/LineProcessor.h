//
//  LineProcessor.h
//  FinderSyncExt
//
//  Created by Claudio Cambra on 17/02/2022.
//

#import "SyncClient.h"

#ifndef LineProcessor_h
#define LineProcessor_h

@interface LineProcessor : NSObject
@property(nonatomic, weak)id<SyncClientDelegate> delegate;

- (instancetype)initWithDelegate:(id<SyncClientDelegate>)delegate;
- (void)process:(NSString*)line;

@end
#endif /* LineProcessor_h */
