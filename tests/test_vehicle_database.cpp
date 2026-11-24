#include "vehicle_database.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace SSV::Core;

void TestVehicleDatabaseAndParams() {
    std::cout << "[TEST] Running Vehicle Database XML and .params Parsing Tests..." << std::endl;

    // 1. Synthetic fallback test ensuring XML parser logic works 100% in CI without disk assets
    {
        const std::string syntheticXml =
            "<VehicleDatabase>\n"
            "  <Car ID=\"TestVehicle_01\" ModelFile=\"TestModel\" HumanPhysicsFile=\"TestPhys\" Speed=\"7\" Drift=\"4\" Strength=\"6\" Accel=\"8\" CarSelectOrder=\"10\"/>\n"
            "</VehicleDatabase>";
        VehicleDatabase synDb;
        bool ok = synDb.LoadXml(syntheticXml);
        assert(ok);
        assert(synDb.GetCount() == 1);
        const VehicleEntry* v = synDb.FindVehicle("TestVehicle_01");
        assert(v != nullptr);
        assert(v->modelFile == "TestModel");
        assert(v->humanPhysicsFile == "TestPhys");
        assert(v->speedStat == 7);
        assert(v->driftStat == 4);
        assert(v->strengthStat == 6);
        assert(v->accelStat == 8);
        assert(v->carSelectOrder == 10);
        std::cout << "  -> Synthetic VehicleDatabase XML unit test passed." << std::endl;
    }

    // 2. Synthetic fallback test ensuring .params parser logic works 100% in CI
    {
        const std::string syntheticParams =
            "Section Chassis\n"
            "1600.0;Mass (kg)\n"
            "0.0 0.2 0.0;Centre of mass (m)\n"
            "0.8 0.808 1.36;Front axle hardpoint (m)\n"
            "-0.8 0.808 -1.36;Rear axle hardpoint (m)\n"
            "Section Aero\n"
            "8.0;Downforce scale\n"
            "Section Suspension\n"
            "100.0;Body roll spring strength\n";
        VehicleParams synParams;
        bool ok = VehicleDatabase::ParseParams(syntheticParams, synParams);
        assert(ok);
        assert(synParams.mass == 1600.0f);
        assert(synParams.com.y == 0.2f);
        assert(synParams.frontAxleHardPoint.x == 0.8f);
        assert(synParams.downforceScale == 8.0f);
        assert(synParams.bodyRollSpringStrength == 100.0f);
        std::cout << "  -> Synthetic VehicleParams unit test passed." << std::endl;
    }

    // 3. Optional real asset disk test: test parsing VehicleDatabase.xml if present
    const char* xmlPaths[] = {
        "assets/extracted/VehicleDatabase.xml",
        "D:\\Codes\\Win_codes\\SSV\\assets\\extracted\\VehicleDatabase.xml"
    };
    std::ifstream xmlFile;
    for (const char* p : xmlPaths) {
        xmlFile.open(p);
        if (xmlFile.is_open()) break;
    }

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

        std::cout << "  -> Real VehicleDatabase.xml verified (" << db.GetCount() << " vehicles parsed, Musclecar_01 & Supercar_01 validated)." << std::endl;
    } else {
        std::cout << "  (Note: Real VehicleDatabase.xml not found on disk, skipping disk test)" << std::endl;
    }

    // 4. Optional real asset disk test: test parsing Musclecar_01.params if present
    const char* paramsPaths[] = {
        "assets/extracted/Musclecar_01/Musclecar_01.params",
        "D:\\Codes\\Win_codes\\SSV\\assets\\extracted\\Musclecar_01\\Musclecar_01.params"
    };
    std::ifstream paramsFile;
    for (const char* p : paramsPaths) {
        paramsFile.open(p);
        if (paramsFile.is_open()) break;
    }

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

        std::cout << "  -> Real Musclecar_01.params verified (Mass=1600kg, COM.y=0.2, AxleZ=+/-1.36m, Downforce=8.0)." << std::endl;
    } else {
        std::cout << "  (Note: Real Musclecar_01.params not found on disk, skipping disk test)" << std::endl;
    }
}
