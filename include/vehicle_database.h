#pragma once
/**
 * @file vehicle_database.h
 * @brief Split/Second Vehicle Database & Physics Parameters Parser
 *
 * Parses VehicleDatabase.xml and .params configuration files to construct
 * data-driven vehicle physics and roster entries.
 */

#include "math_types.h"
#include "vehicle_physics.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace SSV {
namespace Core {

struct VehicleEntry {
    std::string id;
    std::string modelFile;
    std::string humanPhysicsFile;
    std::string aiPhysicsFile;
    std::string chaseCamFile;
    std::string audioFile;
    int speedStat = 1;
    int driftStat = 1;
    int strengthStat = 1;
    int accelStat = 1;
    int overallStat = 1;
    std::string region = "US";
    int carSelectOrder = -1;
    bool locked = false;
};

struct VehicleParams {
    float mass = 1600.0f;
    Math::Vector3 com{0.0f, 0.2f, 0.0f};
    Math::Vector3 frontAxleHardPoint{0.8f, 0.808f, 1.36f};
    Math::Vector3 rearAxleHardPoint{0.8f, 0.808f, -1.36f};
    float downforceScale = 8.0f;
    float downforceSpeedMaxMph = 160.0f;
    float bodyRollSpringStrength = 100.0f;
    float bodyRollSpringDamping = 0.1f;
    float brakingScale = 150.0f;
    float brakingSpeedMaxMph = 140.0f;
    float driftCounterSteerTorque = 2.5f;
    float driftMinSpeedMph = 50.0f;
};

class VehicleDatabase {
public:
    VehicleDatabase() = default;

    /// Parse VehicleDatabase.xml string into vehicle roster
    bool LoadXml(const std::string& xmlContent);

    /// Parse .params text file into VehicleParams
    static bool ParseParams(const std::string& paramsText, VehicleParams& outParams);

    /// Convert VehicleParams and VehicleEntry into Physics::VehicleConfig
    static Physics::VehicleConfig BuildConfig(const VehicleParams& params, const VehicleEntry* entry = nullptr);

    const VehicleEntry* FindVehicle(const std::string& id) const;
    const std::vector<VehicleEntry>& GetVehicles() const { return m_vehicles; }
    size_t GetCount() const { return m_vehicles.size(); }

private:
    std::vector<VehicleEntry> m_vehicles;
    std::unordered_map<std::string, size_t> m_idToIndex;
};

} // namespace Core
} // namespace SSV
