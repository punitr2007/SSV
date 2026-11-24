#include "vehicle_database.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace SSV::Core;

void TestVehicleDatabaseAndParams() {
    std::cout << "[TEST] Running Vehicle Database XML and .params Parsing Tests..." << std::endl;

    // 1. Test parsing VehicleDatabase.xml
    std::ifstream xmlFile("D:\\Codes\\Win_codes\\SSV\\assets\\extracted\\VehicleDatabase.xml");
    if (xmlFile.is_open()) {
        std::stringstream buffer;
        buffer << xmlFile.rdbuf();
        std::string xmlContent = buffer.str();

        VehicleDatabase db;
        bool ok = db.LoadXml(xmlContent);
        assert(ok);
        assert(db.GetCount() == 84);

        const VehicleEntry* m01 = db.FindVehicle("Musclecar_01");
        assert(m01 != nullptr);
        assert(m01->modelFile == "Musclecar_01");
        assert(m01->humanPhysicsFile == "Musclecar_01");
        assert(m01->speedStat == 1);
        assert(m01->driftStat == 8);
        assert(m01->strengthStat == 5);
        assert(m01->carSelectOrder == 1);

        const VehicleEntry* s01 = db.FindVehicle("Supercar_01");
        assert(s01 != nullptr);
        assert(s01->speedStat == 8);
        assert(s01->carSelectOrder == 23);

        std::cout << "  -> VehicleDatabase.xml verified (" << db.GetCount() << " vehicles parsed, Musclecar_01 & Supercar_01 validated)." << std::endl;
    } else {
        std::cout << "  (Note: VehicleDatabase.xml not found, skipping disk test)" << std::endl;
    }

    // 2. Test parsing Musclecar_01.params
    std::ifstream paramsFile("D:\\Codes\\Win_codes\\SSV\\assets\\extracted\\Musclecar_01\\Musclecar_01.params");
    if (paramsFile.is_open()) {
        std::stringstream buffer;
        buffer << paramsFile.rdbuf();
        std::string paramsContent = buffer.str();

        VehicleParams params;
        bool ok = VehicleDatabase::ParseParams(paramsContent, params);
        assert(ok);
        assert(params.mass == 1600.0f);
        assert(params.com.y == 0.2f);
        assert(params.frontAxleHardPoint.x == 0.8f);
        assert(params.frontAxleHardPoint.y == 0.808f);
        assert(params.frontAxleHardPoint.z == 1.36f);
        assert(params.rearAxleHardPoint.z == -1.36f);
        assert(params.downforceScale == 8.0f);
        assert(params.bodyRollSpringStrength == 100.0f);

        // Build config
        auto config = VehicleDatabase::BuildConfig(params);
        assert(config.mass == 1600.0f);
        assert(config.hardPointFL.x == -0.8f);
        assert(config.hardPointFR.x == 0.8f);
        assert(config.hardPointFL.z == 1.36f);
        assert(config.hardPointRL.z == -1.36f);

        std::cout << "  -> Musclecar_01.params verified (Mass=1600kg, COM.y=0.2, AxleZ=+/-1.36m, Downforce=8.0)." << std::endl;
    } else {
        std::cout << "  (Note: Musclecar_01.params not found, skipping disk test)" << std::endl;
    }
}
