//
//  SystemMetricsBridge.hpp
//  Biomon
//
//  Created by Arjan Deo on 18/08/2026.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SystemMetricsBridge : NSObject
+ (int)testValue;
+ (double)cpuUsage;
+ (void)memoryUsedBytes:(unsigned long long *)used totalBytes:(unsigned long long *)total;
+ (void)diskUsedBytes:(unsigned long long *)used totalBytes:(unsigned long long *)total;
+ (void)diskReadBytesPerSec:(double *)read writeBytesPerSec:(double *)write;
+ (void)networkDownBytesPerSec:(double *)down upBytesPerSec:(double *)up;
@end

NS_ASSUME_NONNULL_END
