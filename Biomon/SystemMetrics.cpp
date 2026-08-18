//
//  SystemMetrics.cpp
//  Biomon
//
//  Created by Arjan Deo on 18/08/2026.
//

#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/processor_info.h>
#include <vector>
#include "SystemMetrics.hpp"
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDriver.h>
#include <CoreFoundation/CoreFoundation.h>
#include <chrono>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>

static unsigned long long prevBytesRead = 0;
static unsigned long long prevBytesWritten = 0;
static std::chrono::steady_clock::time_point prevTimestamp;
static bool hasPrevReading = false;

static unsigned long long prevBytesDown = 0;
static unsigned long long prevBytesUp = 0;
static std::chrono::steady_clock::time_point prevNetTimestamp;
static bool hasPrevNetReading = false;

static void sumInterfaceBytes(
    unsigned long long *totalIn,
    unsigned long long *totalOut
) {
    *totalIn = 0;
    *totalOut = 0;

    struct ifaddrs *addrs;

    if (getifaddrs(&addrs) != 0) {
        return;
    }

    for (
        struct ifaddrs *addr = addrs;
        addr != NULL;
        addr = addr->ifa_next
    ) {
        if (
            addr->ifa_addr == NULL ||
            addr->ifa_addr->sa_family != AF_LINK
        ) {
            continue;
        }

        if (strncmp(addr->ifa_name, "lo", 2) == 0) {
            continue;
        }

        struct if_data *ifData = (struct if_data *)addr->ifa_data;

        if (ifData != NULL) {
            *totalIn += ifData->ifi_ibytes;
            *totalOut += ifData->ifi_obytes;
        }
    }

    freeifaddrs(addrs);
}

void getNetworkActivity(
    double *downBytesPerSec,
    double *upBytesPerSec
) {
    unsigned long long currentIn = 0;
    unsigned long long currentOut = 0;

    sumInterfaceBytes(&currentIn, &currentOut);

    auto now = std::chrono::steady_clock::now();

    if (!hasPrevNetReading) {
        *downBytesPerSec = 0.0;
        *upBytesPerSec = 0.0;
    } else {
        double elapsedSeconds =
            std::chrono::duration<double>(
                now - prevNetTimestamp
            ).count();

        if (elapsedSeconds > 0) {
            *downBytesPerSec =
                (double)(currentIn - prevBytesDown) /
                elapsedSeconds;

            *upBytesPerSec =
                (double)(currentOut - prevBytesUp) /
                elapsedSeconds;
        } else {
            *downBytesPerSec = 0.0;
            *upBytesPerSec = 0.0;
        }
    }

    prevBytesDown = currentIn;
    prevBytesUp = currentOut;
    prevNetTimestamp = now;
    hasPrevNetReading = true;
}

static void sumBlockStorageBytes(
    unsigned long long *totalRead,
    unsigned long long *totalWritten
) {
    *totalRead = 0;
    *totalWritten = 0;

    CFMutableDictionaryRef matching =
        IOServiceMatching("IOBlockStorageDriver");

    io_iterator_t iter;

    if (
        IOServiceGetMatchingServices(
            kIOMainPortDefault,
            matching,
            &iter
        ) != KERN_SUCCESS
    ) {
        return;
    }

    io_object_t service;

    while ((service = IOIteratorNext(iter))) {
        CFTypeRef statsRef =
            IORegistryEntryCreateCFProperty(
                service,
                CFSTR("Statistics"),
                kCFAllocatorDefault,
                0
            );

        if (
            statsRef &&
            CFGetTypeID(statsRef) == CFDictionaryGetTypeID()
        ) {
            CFDictionaryRef stats =
                (CFDictionaryRef)statsRef;

            CFNumberRef readNum =
                (CFNumberRef)CFDictionaryGetValue(
                    stats,
                    CFSTR("Bytes (Read)")
                );

            CFNumberRef writeNum =
                (CFNumberRef)CFDictionaryGetValue(
                    stats,
                    CFSTR("Bytes (Write)")
                );

            long long readVal = 0;
            long long writeVal = 0;

            if (readNum) {
                CFNumberGetValue(
                    readNum,
                    kCFNumberSInt64Type,
                    &readVal
                );
            }

            if (writeNum) {
                CFNumberGetValue(
                    writeNum,
                    kCFNumberSInt64Type,
                    &writeVal
                );
            }

            *totalRead +=
                (unsigned long long)readVal;

            *totalWritten +=
                (unsigned long long)writeVal;
        }

        if (statsRef) {
            CFRelease(statsRef);
        }

        IOObjectRelease(service);
    }

    IOObjectRelease(iter);
}

void getDiskActivity(
    double *readBytesPerSec,
    double *writeBytesPerSec
) {
    unsigned long long currentRead = 0;
    unsigned long long currentWritten = 0;

    sumBlockStorageBytes(
        &currentRead,
        &currentWritten
    );

    auto now = std::chrono::steady_clock::now();

    if (!hasPrevReading) {
        *readBytesPerSec = 0.0;
        *writeBytesPerSec = 0.0;
    } else {
        double elapsedSeconds =
            std::chrono::duration<double>(
                now - prevTimestamp
            ).count();

        if (elapsedSeconds > 0) {
            *readBytesPerSec =
                (double)(currentRead - prevBytesRead) /
                elapsedSeconds;

            *writeBytesPerSec =
                (double)(currentWritten - prevBytesWritten) /
                elapsedSeconds;
        } else {
            *readBytesPerSec = 0.0;
            *writeBytesPerSec = 0.0;
        }
    }

    prevBytesRead = currentRead;
    prevBytesWritten = currentWritten;
    prevTimestamp = now;
    hasPrevReading = true;
}

void getDiskUsage(
    unsigned long long *usedBytes,
    unsigned long long *totalBytes
) {
    struct statfs stats;

    if (statfs("/", &stats) != 0) {
        *usedBytes = 0;
        *totalBytes = 0;
        return;
    }

    unsigned long long blockSize =
        (unsigned long long)stats.f_bsize;

    unsigned long long total =
        (unsigned long long)stats.f_blocks *
        blockSize;

    unsigned long long free =
        (unsigned long long)stats.f_bavail *
        blockSize;

    *totalBytes = total;
    *usedBytes = total - free;
}

void getMemoryUsage(
    unsigned long long *usedBytes,
    unsigned long long *totalBytes
) {
    uint64_t physicalMemory = 0;
    size_t size = sizeof(physicalMemory);

    sysctlbyname(
        "hw.memsize",
        &physicalMemory,
        &size,
        NULL,
        0
    );

    *totalBytes = physicalMemory;

    mach_msg_type_number_t count =
        HOST_VM_INFO64_COUNT;

    vm_statistics64_data_t vmStats;

    kern_return_t err =
        host_statistics64(
            mach_host_self(),
            HOST_VM_INFO64,
            (host_info64_t)&vmStats,
            &count
        );

    if (err != KERN_SUCCESS) {
        *usedBytes = 0;
        return;
    }

    vm_size_t pageSize;

    host_page_size(
        mach_host_self(),
        &pageSize
    );

    unsigned long long used =
        (unsigned long long)(
            vmStats.active_count +
            vmStats.wire_count +
            vmStats.compressor_page_count
        ) * pageSize;

    *usedBytes = used;
}

int getTestValue() {
    return 42;
}

static unsigned long long prevTotalTicks = 0;
static unsigned long long prevIdleTicks = 0;

double getCPUUsage() {
    natural_t cpuCount;
    processor_info_array_t cpuInfo;
    mach_msg_type_number_t numCpuInfo;

    kern_return_t err =
        host_processor_info(
            mach_host_self(),
            PROCESSOR_CPU_LOAD_INFO,
            &cpuCount,
            &cpuInfo,
            &numCpuInfo
        );

    if (err != KERN_SUCCESS) {
        return -1.0;
    }

    unsigned long long totalTicks = 0;
    unsigned long long idleTicks = 0;

    for (natural_t i = 0; i < cpuCount; i++) {
        processor_cpu_load_info_t coreInfo =
            (processor_cpu_load_info_t)(
                cpuInfo + (CPU_STATE_MAX * i)
            );

        for (
            int state = 0;
            state < CPU_STATE_MAX;
            state++
        ) {
            totalTicks +=
                coreInfo->cpu_ticks[state];
        }

        idleTicks +=
            coreInfo->cpu_ticks[CPU_STATE_IDLE];
    }

    vm_deallocate(
        mach_task_self(),
        (vm_address_t)cpuInfo,
        numCpuInfo * sizeof(int)
    );

    double usage = 0.0;

    if (prevTotalTicks != 0) {
        unsigned long long totalDelta =
            totalTicks - prevTotalTicks;

        unsigned long long idleDelta =
            idleTicks - prevIdleTicks;

        if (totalDelta > 0) {
            usage =
                100.0 *
                (
                    1.0 -
                    (
                        (double)idleDelta /
                        (double)totalDelta
                    )
                );
        }
    }

    prevTotalTicks = totalTicks;
    prevIdleTicks = idleTicks;

    return usage;
}
