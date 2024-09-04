#import <BareKit/BareKit.h>
#import <Foundation/Foundation.h>
#import <React/RCTLog.h>

#import "BareKitModule.h"

@implementation BareKitModuleWorklet {
@public
  NSUInteger _id;
  BareKitModule *_module;
  BareWorklet *_worklet;
  BareIPC *_ipc;
}

- (_Nullable instancetype)initWithModule:(BareKitModule *)module
                                      id:(NSUInteger)id
                                filename:(NSString *)filename
                                  source:(NSString *)source {
  self = [super init];

  if (self) {
    _id = id;
    _module = module;

    _worklet = [[BareWorklet alloc] init];

    [_worklet start:filename source:[source dataUsingEncoding:NSUTF8StringEncoding]];

    _ipc = [[BareIPC alloc] initWithWorklet:_worklet];

    [self _read];
  }

  return self;
}

- (void)_read {
  [_ipc read:^(NSData *data) {
    if (data == nil) return;

    [self->_module sendEventWithName:@"BareKitIPCData"
                                body:@{
                                  @"worklet" : @(self->_id),
                                  @"data" : [data base64EncodedStringWithOptions:0],
                                }];

    [self _read];
  }];
}

@end

@implementation BareKitModule {
  NSMutableArray<BareKitModuleWorklet *> *_worklets;
}

RCT_EXPORT_MODULE(BareKit)

- (_Nullable instancetype)init {
  self = [super init];

  if (self) {
    _worklets = [[NSMutableArray alloc] init];
  }

  return self;
}

- (NSArray<NSString *> *)supportedEvents {
  return @[ @"BareKitIPCData" ];
}

RCT_EXPORT_METHOD(start : (NSString *) filename
                  source : (NSString *) source
                  resolve : (RCTPromiseResolveBlock) resolve
                  reject : (RCTPromiseRejectBlock) reject) {
  NSUInteger id = _worklets.count;

  [_worklets addObject:[[BareKitModuleWorklet alloc] initWithModule:self
                                                                 id:id
                                                           filename:filename
                                                             source:source]];

  resolve(@(id));
}

RCT_EXPORT_METHOD(write : (nonnull NSNumber *) id
                  data : (NSString *) data
                  resolve : (RCTPromiseResolveBlock) resolve
                  reject : (RCTPromiseRejectBlock) reject) {
  BareKitModuleWorklet *worklet = _worklets[id.unsignedIntegerValue];

  if (worklet == nil) return reject(@"INVALID_ID", @"No such worklet found", nil);

  [worklet->_ipc write:[[NSData alloc] initWithBase64EncodedString:data options:0]];

  resolve(nil);
}

@end
