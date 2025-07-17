
#pragma once

#include <cstdint>
#include <random>

// Using a 64-bit integer as our UUID for simplicity.
// For true universal uniqueness, a 128-bit UUID library would be better,
// but this is robust enough for most single-project needs.
using UUID = uint64_t;

namespace UUIDGenerator {
    // A simple utility to generate random UUIDs.
    inline UUID generate() {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<UUID> dis;
        return dis(gen);
    }
}

// This component is attached to every actor to give it a persistent identity.
struct IDComponent {
    UUID uuid;

    IDComponent() = default;
    IDComponent(UUID id) : uuid(id) {}
};