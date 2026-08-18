//
//  BiomonApp.swift
//  Biomon
//
//  Created by Arjan Deo on 18/08/2026.
//

import SwiftUI

@main
struct BiomonApp: App {
    var body: some Scene {
        MenuBarExtra("Biomon", systemImage: "heart.fill") {
            ContentView()
        }
        .menuBarExtraStyle(.window)
    }
}
