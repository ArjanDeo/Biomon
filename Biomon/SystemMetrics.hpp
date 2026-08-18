//
//  SystemMetrics.hpp
//  Biomon
//
//  Created by Arjan Deo on 18/08/2026.
#ifndef SystemMetrics_h
#define SystemMetrics_h
#pragma once

extern "C" {
    int getTestValue();
    double getCPUUsage();
    void getMemoryUsage(unsigned long long *usedBytes, unsigned long long *totalBytes);
    void getDiskUsage(unsigned long long *usedBytes, unsigned long long *totalBytes);
    void getDiskActivity(double *readBytesPerSec, double *writeBytesPerSec);

    // Network throughput: bytes/sec down and up, summed across all interfaces
    void getNetworkActivity(double *downBytesPerSec, double *upBytesPerSec);
}

#endif /* SystemMetrics_h */
