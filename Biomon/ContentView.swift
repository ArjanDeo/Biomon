//
//  ContentView.swift
//  Biomon
//
//  Created by Arjan Deo on 18/08/2026.
//

import SwiftUI
import Combine

struct ContentView: View {
    @State private var cpuUsage: Double = 0.0
    @State private var usedMemoryGB: Double = 0.0
    @State private var totalMemoryGB: Double = 0.0
    @State private var usedDiskGB: Double = 0.0
    @State private var totalDiskGB: Double = 0.0
    @State private var diskReadMBps: Double = 0.0
    @State private var diskWriteMBps: Double = 0.0
    @State private var networkDownMBps: Double = 0.0
    @State private var networkUpMBps: Double = 0.0

    let timer = Timer.publish(
        every: 1.0,
        on: .main,
        in: .common
    ).autoconnect()

    private var memoryUsage: Double {
        guard totalMemoryGB > 0 else { return 0 }
        return usedMemoryGB / totalMemoryGB
    }

    private var diskUsage: Double {
        guard totalDiskGB > 0 else { return 0 }
        return usedDiskGB / totalDiskGB
    }

    var body: some View {
        VStack(spacing: 0) {

            // MARK: Header

            HStack {
                HStack(spacing: 8) {
                    Image(systemName: "gauge.with.dots.needle.33percent")
                        .font(.system(size: 16, weight: .semibold))

                    Text("Biomon")
                        .font(.system(size: 15, weight: .semibold))
                }

                Spacer()

                Circle()
                    .fill(.green)
                    .frame(width: 7, height: 7)

                Text("Live")
                    .font(.system(size: 11, weight: .medium))
                    .foregroundStyle(.secondary)
            }
            .padding(.horizontal, 16)
            .padding(.vertical, 12)

            Divider()

            // MARK: Main Metrics

            VStack(spacing: 10) {

                HStack(spacing: 10) {
                    MetricCard(
                        title: "CPU",
                        icon: "cpu",
                        value: String(format: "%.1f%%", cpuUsage),
                        progress: min(cpuUsage / 100.0, 1.0),
                        statusColor: cpuColor(cpuUsage)
                    )

                    MetricCard(
                        title: "Memory",
                        icon: "memorychip",
                        value: String(
                            format: "%.1f / %.1f GB",
                            usedMemoryGB,
                            totalMemoryGB
                        ),
                        progress: memoryUsage,
                        statusColor: usageColor(memoryUsage)
                    )
                }

                HStack(spacing: 10) {
                    MetricCard(
                        title: "Disk",
                        icon: "internaldrive",
                        value: String(
                            format: "%.2f / %.2f TB",
                            usedDiskGB / 1024.0,
                            totalDiskGB / 1024.0
                        ),
                        progress: diskUsage,
                        statusColor: usageColor(diskUsage)
                    )

                    MetricCard(
                        title: "Network",
                        icon: "network",
                        value: String(
                            format: "↓ %.1f MB/s",
                            networkDownMBps
                        ),
                        subtitle: String(
                            format: "↑ %.1f MB/s",
                            networkUpMBps
                        )
                    )
                }
            }
            .padding(12)

            // MARK: Activity

            VStack(alignment: .leading, spacing: 10) {
                Text("Activity")
                    .font(.system(size: 12, weight: .semibold))
                    .foregroundStyle(.secondary)

                ActivityRow(
                    icon: "arrow.down.circle",
                    title: "Disk Read",
                    value: String(format: "%.1f MB/s", diskReadMBps)
                )

                ActivityRow(
                    icon: "arrow.up.circle",
                    title: "Disk Write",
                    value: String(format: "%.1f MB/s", diskWriteMBps)
                )

                ActivityRow(
                    icon: "arrow.down",
                    title: "Network Download",
                    value: String(format: "%.1f MB/s", networkDownMBps)
                )

                ActivityRow(
                    icon: "arrow.up",
                    title: "Network Upload",
                    value: String(format: "%.1f MB/s", networkUpMBps)
                )
            }
            .padding(.horizontal, 16)
            .padding(.bottom, 14)

            Divider()

            // MARK: Footer

            HStack {
                Text("Biomon v1.0.0")
                    .font(.system(size: 10))
                    .foregroundStyle(.tertiary)

                Spacer()

                Button {
                    refreshMetrics()
                } label: {
                    Image(systemName: "arrow.clockwise")
                        .font(.system(size: 11, weight: .medium))
                }
                .buttonStyle(.plain)
                .foregroundStyle(.secondary)
            }
            .padding(.horizontal, 16)
            .padding(.vertical, 9)
        }
        .frame(width: 360)
        .onReceive(timer) { _ in
            refreshMetrics()
        }
        .onAppear {
            refreshMetrics()
        }
    }

    // MARK: - Status Colors

    private func cpuColor(_ value: Double) -> Color {
        switch value {
        case 0..<60:
            return .green
        case 60..<85:
            return .yellow
        default:
            return .red
        }
    }

    private func usageColor(_ value: Double) -> Color {
        switch value {
        case 0..<0.60:
            return .green
        case 0.60..<0.85:
            return .yellow
        default:
            return .red
        }
    }

    // MARK: - Metrics

    private func refreshMetrics() {
        cpuUsage = SystemMetricsBridge.cpuUsage()

        var usedMem: UInt64 = 0
        var totalMem: UInt64 = 0

        SystemMetricsBridge.memoryUsedBytes(
            &usedMem,
            totalBytes: &totalMem
        )

        let gb = 1024.0 * 1024.0 * 1024.0

        usedMemoryGB = Double(usedMem) / gb
        totalMemoryGB = Double(totalMem) / gb

        var usedDisk: UInt64 = 0
        var totalDisk: UInt64 = 0

        SystemMetricsBridge.diskUsedBytes(
            &usedDisk,
            totalBytes: &totalDisk
        )

        usedDiskGB = Double(usedDisk) / gb
        totalDiskGB = Double(totalDisk) / gb

        var readRate: Double = 0.0
        var writeRate: Double = 0.0

        SystemMetricsBridge.diskReadBytesPerSec(
            &readRate,
            writeBytesPerSec: &writeRate
        )

        diskReadMBps = readRate / (1024.0 * 1024.0)
        diskWriteMBps = writeRate / (1024.0 * 1024.0)

        var netDown: Double = 0.0
        var netUp: Double = 0.0

        SystemMetricsBridge.networkDownBytesPerSec(
            &netDown,
            upBytesPerSec: &netUp
        )

        networkDownMBps = netDown / (1024.0 * 1024.0)
        networkUpMBps = netUp / (1024.0 * 1024.0)
    }
}

// MARK: - Metric Card

struct MetricCard: View {
    let title: String
    let icon: String
    let value: String
    var subtitle: String? = nil
    var progress: Double? = nil
    var statusColor: Color = .primary

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {

            HStack(spacing: 6) {
                Image(systemName: icon)
                    .font(.system(size: 11, weight: .medium))
                    .foregroundStyle(.secondary)

                Text(title)
                    .font(.system(size: 11, weight: .medium))
                    .foregroundStyle(.secondary)
            }

            Text(value)
                .font(
                    .system(
                        size: 16,
                        weight: .semibold,
                        design: .monospaced
                    )
                )
                .foregroundStyle(statusColor)
                .lineLimit(1)
                .minimumScaleFactor(0.8)

            if let subtitle {
                Text(subtitle)
                    .font(
                        .system(
                            size: 12,
                            weight: .medium,
                            design: .monospaced
                        )
                    )
                    .foregroundStyle(.secondary)
            }

            if let progress {
                ProgressView(value: progress)
                    .progressViewStyle(.linear)
                    .tint(statusColor)
                    .controlSize(.small)
            }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(12)
        .background(
            RoundedRectangle(cornerRadius: 10)
                .fill(.quaternary.opacity(0.45))
        )
    }
}

// MARK: - Activity Row

struct ActivityRow: View {
    let icon: String
    let title: String
    let value: String

    var body: some View {
        HStack(spacing: 10) {

            Image(systemName: icon)
                .font(.system(size: 13, weight: .medium))
                .foregroundStyle(.secondary)
                .frame(width: 18)

            Text(title)
                .font(.system(size: 12))

            Spacer()

            Text(value)
                .font(
                    .system(
                        size: 12,
                        weight: .medium,
                        design: .monospaced
                    )
                )
                .foregroundStyle(.secondary)
        }
    }
}

#Preview {
    ContentView()
        .padding()
}
