#include "vehicle_database.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace SSV {
namespace Core {

static std::string ExtractAttr(const std::string& line, const std::string& attrName) {
    std::string pattern = attrName + "=\"";
    size_t pos = line.find(pattern);
    if (pos == std::string::npos) {
        return "";
    }
    pos += pattern.length();
    size_t endPos = line.find('"', pos);
    if (endPos == std::string::npos) {
        return "";
    }
    return line.substr(pos, endPos - pos);
}

bool VehicleDatabase::LoadXml(const std::string& xmlContent) {
    m_vehicles.clear();
    m_idToIndex.clear();

    std::istringstream stream(xmlContent);
    std::string line;

    while (std::getline(stream, line)) {
        size_t carTag = line.find("<Car ");
        if (carTag == std::string::npos) {
            continue;
        }

        VehicleEntry entry;
        entry.id = ExtractAttr(line, "ID");
        if (entry.id.empty()) {
            continue;
        }

        entry.modelFile = ExtractAttr(line, "ModelFile");
        entry.humanPhysicsFile = ExtractAttr(line, "HumanPhysicsFile");
        entry.aiPhysicsFile = ExtractAttr(line, "AIPhysicsFile");
        entry.chaseCamFile = ExtractAttr(line, "ChaseCamFile");
        entry.audioFile = ExtractAttr(line, "AudioFile");
        entry.region = ExtractAttr(line, "Region");

        std::string speedStr = ExtractAttr(line, "Speed");
        if (!speedStr.empty()) entry.speedStat = std::stoi(speedStr);

        std::string driftStr = ExtractAttr(line, "Drift");
        if (!driftStr.empty()) entry.driftStat = std::stoi(driftStr);

        std::string strengthStr = ExtractAttr(line, "Strength");
        if (!strengthStr.empty()) entry.strengthStat = std::stoi(strengthStr);

        std::string accelStr = ExtractAttr(line, "Accel");
        if (!accelStr.empty()) entry.accelStat = std::stoi(accelStr);

        std::string overallStr = ExtractAttr(line, "OverallStat");
        if (!overallStr.empty()) entry.overallStat = std::stoi(overallStr);

        std::string orderStr = ExtractAttr(line, "CarSelectOrder");
        if (!orderStr.empty()) entry.carSelectOrder = std::stoi(orderStr);

        std::string lockedStr = ExtractAttr(line, "Locked");
        if (lockedStr == "true") entry.locked = true;

        m_idToIndex[entry.id] = m_vehicles.size();
        m_vehicles.push_back(entry);
    }

    return !m_vehicles.empty();
}

static float ParseFirstFloat(const std::string& valStr) {
    std::string clean = valStr;
    // Strip leading brackets or spaces
    size_t start = clean.find_first_of("-0123456789.");
    if (start == std::string::npos) return 0.0f;
    return std::stof(clean.substr(start));
}

bool VehicleDatabase::ParseParams(const std::string& paramsText, VehicleParams& outParams) {
    std::istringstream stream(paramsText);
    std::string line;
    std::string curSection = "";

    while (std::getline(stream, line)) {
        // Strip trailing carriage return and whitespace
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }
        size_t lead = line.find_first_not_of(" \t");
        if (lead == std::string::npos) continue;
        line = line.substr(lead);

        if (line.rfind("/Vehicles/Config/", 0) == 0 && line.back() == ':') {
            curSection = line.substr(0, line.length() - 1);
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = line.substr(0, eqPos);
        std::string val = line.substr(eqPos + 1);

        if (curSection.find("Chassis") != std::string::npos && key.find(";Mass") != std::string::npos) {
            outParams.mass = ParseFirstFloat(val);
        } else if (key.find("COM.x") != std::string::npos) {
            outParams.com.x = ParseFirstFloat(val);
        } else if (key.find("COM.y") != std::string::npos) {
            outParams.com.y = ParseFirstFloat(val);
        } else if (key.find("COM.z") != std::string::npos) {
            outParams.com.z = ParseFirstFloat(val);
        } else if (curSection.find("Suspension/FrontAxle") != std::string::npos) {
            if (key.find("HardPoint.x") != std::string::npos) outParams.frontAxleHardPoint.x = ParseFirstFloat(val);
            if (key.find("HardPoint.y") != std::string::npos) outParams.frontAxleHardPoint.y = ParseFirstFloat(val);
            if (key.find("HardPoint.z") != std::string::npos) outParams.frontAxleHardPoint.z = ParseFirstFloat(val);
        } else if (curSection.find("Suspension/RearAxle") != std::string::npos) {
            if (key.find("HardPoint.x") != std::string::npos) outParams.rearAxleHardPoint.x = ParseFirstFloat(val);
            if (key.find("HardPoint.y") != std::string::npos) outParams.rearAxleHardPoint.y = ParseFirstFloat(val);
            if (key.find("HardPoint.z") != std::string::npos) outParams.rearAxleHardPoint.z = ParseFirstFloat(val);
        } else if (key.find("Downforce acceleration MPSScale") != std::string::npos) {
            outParams.downforceScale = ParseFirstFloat(val);
        } else if (curSection.find("BodyRoll/Pitch") != std::string::npos || curSection.find("BodyRoll/Roll") != std::string::npos) {
            if (key.find("SpringStrength") != std::string::npos) outParams.bodyRollSpringStrength = ParseFirstFloat(val);
            if (key.find("SpringDampening") != std::string::npos) outParams.bodyRollSpringDamping = ParseFirstFloat(val);
        } else if (key.find("Braking(mph per sec)Scale") != std::string::npos) {
            outParams.brakingScale = ParseFirstFloat(val);
        } else if (key.find("SkidAngleToCounterSteeringTorqueCurve_AccelerationScale") != std::string::npos) {
            outParams.driftCounterSteerTorque = ParseFirstFloat(val);
        }
    }

    return true;
}

Physics::VehicleConfig VehicleDatabase::BuildConfig(const VehicleParams& params, const VehicleEntry* entry) {
    Physics::VehicleConfig config;
    config.mass = params.mass;

    // Set hardpoints from exact front and rear axles
    config.hardPointFL = {-params.frontAxleHardPoint.x, params.frontAxleHardPoint.y - 0.55f,  params.frontAxleHardPoint.z};
    config.hardPointFR = { params.frontAxleHardPoint.x, params.frontAxleHardPoint.y - 0.55f,  params.frontAxleHardPoint.z};
    config.hardPointRL = {-params.rearAxleHardPoint.x,  params.rearAxleHardPoint.y  - 0.55f,  params.rearAxleHardPoint.z};
    config.hardPointRR = { params.rearAxleHardPoint.x,  params.rearAxleHardPoint.y  - 0.55f,  params.rearAxleHardPoint.z};

    // Tune top speed from speed stat if provided (Speed stat 1 = ~145 MPH, 10 = ~215 MPH)
    if (entry) {
        config.topSpeedMph = 140.0f + (entry->speedStat * 7.5f);
        config.engineForce = 14000.0f + (entry->accelStat * 1200.0f);
        config.handbrakeDriftSlip = 0.45f - (entry->driftStat * 0.02f);
    }

    return config;
}

const VehicleEntry* VehicleDatabase::FindVehicle(const std::string& id) const {
    auto it = m_idToIndex.find(id);
    if (it != m_idToIndex.end()) {
        return &m_vehicles[it->second];
    }
    return nullptr;
}

} // namespace Core
} // namespace SSV
