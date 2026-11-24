import json

evidence = {
    "target": {
        "file_name": "SplitSecond.exe",
        "file_path": "D:\\Codes\\Win_codes\\Split.Second.Velocity\\SplitSecond.exe",
        "file_size": 14004224,
        "format": "PE32 (x86 32-bit)",
        "machine": "0x014C (IMAGE_FILE_MACHINE_I386)",
        "time_date_stamp": "0x4BE13C66 (Wed May 5 09:51:34 2010 UTC)",
        "subsystem": "IMAGE_SUBSYSTEM_WINDOWS_GUI (2)",
        "image_base": "0x00400000",
        "entry_point_rva": "0x00D8F150",
        "entry_point_va": "0x0118F150",
        "sha256": "d11fc7f48dd504bdefb16b548980bd41101f6c1a23d29c03487a81d4de42173e"
    },
    "sections": [
        {"name": ".text", "va": "0x00401000", "vsize": "0x00799F91", "raw_offset": "0x00001000", "raw_size": "0x0079A000", "characteristics": "0x60000020"},
        {"name": ".rdata", "va": "0x00BA0000", "vsize": "0x00159837", "raw_offset": "0x0079B000", "raw_size": "0x0015A000", "characteristics": "0x40000040"},
        {"name": ".data", "va": "0x00CFA000", "vsize": "0x000A793C", "raw_offset": "0x008F5000", "raw_size": "0x00070000", "characteristics": "0xC0000040"},
        {"name": ".tls", "va": "0x00DA2000", "vsize": "0x00001811", "raw_offset": "0x00965000", "raw_size": "0x00002000", "characteristics": "0xC0000040"},
        {"name": ".rsrc", "va": "0x00DA4000", "vsize": "0x003EEAB4", "raw_offset": "0x00967000", "raw_size": "0x003EF000", "characteristics": "0x40000040"},
        {"name": ".secu", "va": "0x0118E000", "vsize": "0x00004A90", "raw_offset": "0x00D56000", "raw_size": "0x00005000", "characteristics": "0xE0000020"}
    ],
    "compiler_and_toolchain": {
        "linker_version": "8.0",
        "compiler": "Microsoft Visual C++ 2005 (MSVC 8.0, _MSC_VER 1400)",
        "crt_linkage": "Statically linked MSVC CRT (libcmt.lib / libcpmt.lib)",
        "security_cookie_init": "0x0075E1EA",
        "crt_startup": "0x00756016 (__tmainCRTStartup)",
        "winmain_crt_startup": "0x007561F6"
    },
    "engine_and_middleware": {
        "engine_name": "Black Rock Studio Proprietary Engine (ATV / Pure Engine Evolution)",
        "engine_mutex": "Global\\DISNEYATVPUREPC (VA: 0x00C4AA64)",
        "developer": "Black Rock Studio (Brighton, UK)",
        "publisher": "Disney Interactive Studios",
        "physics_engine": "Havok Physics (with Havok Animation and Havok Visual Debugger, VA: 0x00BCA780)",
        "audio_middleware": "FMOD Ex 4.x + FMOD Event System (fmodex.dll, fmod_event.dll)",
        "video_middleware": "RAD Game Tools Bink Video (binkw32.dll)",
        "ui_middleware": "Scaleform GFx (Flash-based UI)",
        "network_middleware": "Quazal Net-Z / Rendez-Vous (Duplicated Objects / DDL)"
    },
    "asset_subsystem": {
        "archive_extension": ".ark",
        "header_size_bytes": 16,
        "entry_size_bytes": 20,
        "compression": "LZSS algorithm",
        "archive_classes": {
            "ArkFactory": {"vtable": "0x00BA0338", "col": "0x00C73198", "type_desc": "0x00CF7498"},
            "ArkLoader": {"vtable": "0x00BA03A4", "col": "0x00C73238", "type_desc": "0x00CF74D4"},
            "ArkFileFinder": {"vtable": "0x00BA03EC", "col": "0x00C73290", "type_desc": "0x00CF74EC"},
            "ParasiteLoader": {"vtable": "0x00BA0364", "col": "0x00C731E4", "type_desc": "0x00CF74B4"},
            "lzssUnpackFactory": {"vtable": "0x00BA0410", "col": "0x00C732E4", "type_desc": "0x00CF7508"},
            "WinFileFactory": {"vtable": "0x00BA028C", "col": "0x00C72C90", "type_desc": "0x00CF71F8"}
        },
        "streaming_architecture": "Sector split (.static.ark for geometry, .stream.ark for dynamic streaming)"
    },
    "entry_points_and_state_machine": {
        "pe_entry_point": "0x0118F150 (VA in .secu)",
        "winmain_crt_startup": "0x007561F6",
        "winmain": "0x00A172B0",
        "game_application_init": "0x00A12CE0",
        "game_run_method": "0x00A129A0",
        "game_thread_creation": "0x00405F17 (CreateThread suspended, stack 0x4000)",
        "game_thread_proc": "0x00405E90",
        "windows_message_pump": "0x0044F9E7 (PeekMessageA)",
        "direct3d9_create": "0x00B189BA (IAT thunk) / 0x0089F0FE (call)",
        "state_machine": {
            "cGameState": {"col": "0x00C742D0", "type_desc": "0x00D012A0"},
            "cBootupState": {"vtable": "0x00BA2AA4", "enter": "0x00412360", "update": "0x007D8120", "render": "0x00417740", "exit": "0x00411A30"},
            "cIntroState": {"vtable": "0x00BA32DC", "enter": "0x00414960", "update": "0x00414BE0", "render": "0x00414930", "exit": "0x00414EB0"},
            "cIntroSequence": {"vtable": "0x00BA37EC", "enter": "0x00418830"},
            "cFrontendLoadState": {"vtable": "0x00BA3398", "enter": "0x00415480", "update": "0x00415010", "render": "0x00417740", "exit": "0x004151F0"},
            "cFrontendRunState": {"vtable": "0x00BA38C0", "enter": "0x00418F40", "update": "0x004192A0", "render": "0x00418FB0", "exit": "0x00419000"},
            "cInGameLoadState": {"vtable": "0x00BA3110", "enter": "0x00414710", "update": "0x00413BB0", "render": "0x00417740", "exit": "0x00413E10"},
            "cInGameRunState": {"vtable": "0x00BA36A4", "enter": "0x004177A0", "update": "0x00417EB0", "render": "0x00417740", "exit": "0x00417940"},
            "cInGameMovieState": {"vtable": "0x00BA3518", "enter": "0x00417370", "update": "0x00417580", "render": "0x00416E70", "exit": "0x00417310"},
            "cLoadDemoState": {"vtable": "0x00BA3894", "enter": "0x007D8120", "update": "0x007D8120", "exit": "0x00418BF0"}
        }
    }
}

with open(r"D:\Codes\Win_codes\SSV\architectural_evidence.json", "w", encoding="utf-8") as f:
    json.dump(evidence, f, indent=2)

print("Saved architectural_evidence.json successfully!")
