//
//  SystemMetricsBridge.m
//  Biomon
//
//  Created by Arjan Deo on 18/08/2026.
//
#import "SystemMetricsBridge.h"
#import "SystemMetrics.hpp"

@implementation SystemMetricsBridge

+ (int)testValue {
    return getTestValue();
}

+ (double)cpuUsage {
    return getCPUUsage();
}
+ (void)memoryUsedBytes:(unsigned long long *)used totalBytes:(unsigned long long *)total {
    getMemoryUsage(used, total);
}
+ (void)diskUsedBytes:(unsigned long long *)used totalBytes:(unsigned long long *)total {
    getDiskUsage(used, total);
}
+ (void)diskReadBytesPerSec:(double *)read writeBytesPerSec:(double *)write {
    getDiskActivity(read, write);
}
+ (void)networkDownBytesPerSec:(double *)down upBytesPerSec:(double *)up {
    getNetworkActivity(down, up);
}
@end
