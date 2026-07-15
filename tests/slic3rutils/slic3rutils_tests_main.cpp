#include <catch2/catch_all.hpp>

#include "libslic3r/Preset.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "slic3r/Utils/CrealityPrintAgent.hpp"
#include "slic3r/Utils/Http.hpp"
#include "slic3r/Utils/OrcaCloudServiceAgent.hpp"

namespace {

nlohmann::json flat_session_json(const nlohmann::json& fields)
{
    nlohmann::json session = {
        {"access_token", "test-token"},
        {"user_id", "test-user-id"}
    };
    session.update(fields);
    return session;
}

nlohmann::json nested_session_json(const nlohmann::json& metadata)
{
    return {
        {"access_token", "test-token"},
        {"user", {
            {"id", "test-user-id"},
            {"user_metadata", metadata}
        }}
    };
}

std::string resolved_display_name(const nlohmann::json& session)
{
    Slic3r::OrcaCloudServiceAgent agent("");
    REQUIRE(agent.set_user_session(session, false));
    return agent.get_user_nickname();
}

Slic3r::Preset& add_filament_preset(Slic3r::PresetCollection& filaments,
                                    const std::string&         name,
                                    const std::string&         type,
                                    const std::string&         filament_id,
                                    bool                       system,
                                    bool                       compatible = true)
{
    Slic3r::DynamicPrintConfig config;
    config.set_key_value("filament_type", new Slic3r::ConfigOptionStrings{type});
    auto& preset = filaments.load_preset("", name, std::move(config), false);
    preset.filament_id = filament_id;
    preset.is_system = system;
    preset.is_visible = true;
    preset.is_compatible = compatible;
    return preset;
}

} // namespace

TEST_CASE("Creality CFS exact profile names and heuristic fallback select presets", "[CrealityPrintAgent]")
{
    Slic3r::PresetCollection filaments(
        Slic3r::Preset::TYPE_FILAMENT,
        Slic3r::Preset::filament_options(),
        static_cast<const Slic3r::PrintRegionConfig&>(Slic3r::FullPrintConfig::defaults()),
        "Default Filament");
    filaments.set_default_suppressed(true);

    add_filament_preset(filaments, "Generic PLA @System", "PLA", "GFL99", true);
    add_filament_preset(filaments, "Creality Rapid PLA @K2", "PLA", "SYS01", true);
    add_filament_preset(filaments, "ELEGOO RAPID PLA @K2-all - Copy", "PLA", "USR01", false);
    add_filament_preset(filaments, "OVERTURE ASA Basic @Creality K1C 0.4 nozzle", "ASA", "ASA01", true);
    add_filament_preset(filaments, "Creality Rapid PLA @K2 (Harky)", "PLA", "USR02", false);

    // 1. Exact match should work even when query type is PLA but preset is ASA
    CHECK(Slic3r::CrealityPrintAgent::match_filament_preset(
              filaments, "", "OVERTURE ASA Basic @Creality K1C 0.4 nozzle", "PLA")
          == "OVERTURE ASA Basic @Creality K1C 0.4 nozzle");

    // 2. Exact match case-insensitive
    CHECK(Slic3r::CrealityPrintAgent::match_filament_preset(
              filaments, "ELEGOO", "elegoo rapid pla @k2-ALL - copy", "PLA")
          == "ELEGOO RAPID PLA @K2-all - Copy");

    // 3. Generic fallback
    CHECK(Slic3r::CrealityPrintAgent::match_filament_preset(
              filaments, "", "PLA", "PLA")
          == "GFL99");

    // 4. Tie-breaker favors user preset (USR02) over system preset (SYS01)
    CHECK(Slic3r::CrealityPrintAgent::match_filament_preset(
              filaments, "", "Creality Rapid", "PLA")
          == "Creality Rapid PLA @K2 (Harky)");
}

TEST_CASE("Creality CFS boxsInfo parse: remaining percent, CFS-mini, external holder", "[CrealityPrintAgent]")
{
    // Two active boxes: id 0 is the external spool holder (type 1, must be
    // skipped), id 1 is a CFS-mini (type 2, must be accepted and renumbered
    // to AMS index 0). Slot A carries a remaining percent; slot B an
    // out-of-range one (must fold to unknown = -1).
    const std::string response = R"({
        "boxsInfo": { "materialBoxs": [
            { "id": 0, "state": 1, "type": 1, "materials": [
                { "id": 0, "state": 1, "vendor": "Creality", "type": "PLA", "name": "Ext", "color": "#0FFFFFF" }
            ]},
            { "id": 1, "state": 1, "type": 2, "materials": [
                { "id": 0, "state": 1, "vendor": "Creality", "type": "ABS", "name": "Hyper ABS", "color": "#0FFFFFF", "percent": 80 },
                { "id": 1, "state": 2, "vendor": "Generic",  "type": "PLA", "name": "PLA",       "color": "#0FF1E1E", "percent": 150 },
                { "id": 2, "state": 0, "vendor": "", "type": "", "name": "", "color": "-1" }
            ]}
        ]}
    })";

    std::vector<Slic3r::CrealityPrintAgent::CFSSlot> slots;
    int         box_count = 0;
    std::string error;
    REQUIRE(Slic3r::CrealityPrintAgent::parse_cfs_response(response, slots, box_count, error));

    CHECK(box_count == 1);          // external holder skipped, CFS-mini renumbered to 0
    REQUIRE(slots.size() == 2);     // empty slot skipped
    CHECK(slots[0].box_id == 0);
    CHECK(slots[0].slot_id == 0);
    CHECK(slots[0].filament_type == "ABS");
    CHECK(slots[0].percent == 80);
    CHECK(slots[0].color_hex == "#FFFFFF");  // "#0RRGGBB" normalized
    CHECK(slots[1].slot_id == 1);
    CHECK(slots[1].percent == -1);  // 150 is out of range -> unknown
}

TEST_CASE("Check SSL certificates paths", "[Http][NotWorking]") {
    
    Slic3r::Http g = Slic3r::Http::get("https://github.com/");
    
    unsigned status = 0;
    g.on_error([&status](std::string, std::string, unsigned http_status) {
        status = http_status;
    });
    
    g.on_complete([&status](std::string /* body */, unsigned http_status){
        status = http_status;
    });
    
    g.perform_sync();
    
    REQUIRE(status == 200);
}

TEST_CASE("Orca cloud flat session resolves display name consistently", "[OrcaCloudServiceAgent]")
{
    CHECK(resolved_display_name(flat_session_json({
        {"username", "orca_username"},
        {"display_name", "Display Name"},
        {"nickname", "Nickname"}
    })) == "Display Name");

    CHECK(resolved_display_name(flat_session_json({
        {"username", "orca_username"},
        {"nickname", "Nickname"}
    })) == "Nickname");

    CHECK(resolved_display_name(flat_session_json({
        {"username", "orca_username"},
        {"full_name", "Full Name"}
    })) == "Full Name");

    CHECK(resolved_display_name(flat_session_json({
        {"username", "orca_username"},
        {"name", "Provider Name"}
    })) == "Provider Name");

    CHECK(resolved_display_name(flat_session_json({
        {"username", "orca_username"}
    })) == "orca_username");
}

TEST_CASE("Orca cloud nested session resolves display name consistently", "[OrcaCloudServiceAgent]")
{
    CHECK(resolved_display_name(nested_session_json({
        {"username", "orca_username"},
        {"display_name", "Display Name"},
        {"nickname", "Nickname"}
    })) == "Display Name");

    CHECK(resolved_display_name(nested_session_json({
        {"username", "orca_username"},
        {"nickname", "Nickname"}
    })) == "Nickname");

    CHECK(resolved_display_name(nested_session_json({
        {"username", "orca_username"},
        {"full_name", "Full Name"}
    })) == "Full Name");

    CHECK(resolved_display_name(nested_session_json({
        {"username", "orca_username"},
        {"name", "Provider Name"}
    })) == "Provider Name");

    CHECK(resolved_display_name(nested_session_json({
        {"username", "orca_username"}
    })) == "orca_username");
}

TEST_CASE("Http digest authentication", "[Http][NotWorking]") {
    Slic3r::Http g = Slic3r::Http::get("https://httpbingo.org/digest-auth/auth/guest/guest");

    g.auth_digest("guest", "guest");

    unsigned status = 0;
    g.on_error([&status](std::string, std::string, unsigned http_status) {
        status = http_status;
    });

    g.on_complete([&status](std::string /* body */, unsigned http_status){
        status = http_status;
    });

    g.perform_sync();

    REQUIRE(status == 200);
}

TEST_CASE("Http basic authentication", "[Http][NotWorking]") {
    Slic3r::Http g = Slic3r::Http::get("https://httpbingo.org/basic-auth/guest/guest");

    g.auth_basic("guest", "guest");

    unsigned status = 0;
    g.on_error([&status](std::string, std::string, unsigned http_status) {
        status = http_status;
    });

    g.on_complete([&status](std::string /* body */, unsigned http_status){
        status = http_status;
    });

    g.perform_sync();

    REQUIRE(status == 200);
}
