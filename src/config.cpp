#include "config.h"

#include "graphics/appearances.h"
#include "items.h"
#include "util.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

BorderBrushVariationType Settings::BORDER_BRUSH_VARIATION = BorderBrushVariationType::Detailed;
bool Settings::AUTO_BORDER = true;
int Settings::UI_CHANGE_TIME_DELAY_MILLIS = 100;
int Settings::DEFAULT_CREATURE_SPAWN_INTERVAL = 60;

bool Settings::HIGHLIGHT_BRUSH_IN_PALETTE_ON_SELECT = false;

namespace
{
    constexpr auto DataFolder = "data/clients";

    constexpr auto ConfigFile = "config.json";
    constexpr auto CatalogContentFile = "catalog-content.json";
    constexpr auto AppearancesFile = "appearances.dat";
} // namespace

Config::Config(const std::string version)
    : _version(version),
      _dataFolder(DataFolder / std::filesystem::path(version)),
      _configFile(_dataFolder / ConfigFile),
      _loaded(false)
{
}

Result<Config, Config::Error> Config::create(const std::string version)
{
    Config config(version);

    if (!std::filesystem::exists(config._configFile))
    {
        std::stringstream s;
        s << "Could not locate config file for client version '" << version << "'." << std::endl;
        s << "You need to add a '" << ConfigFile << "' file at: '" << std::filesystem::absolute(config._configFile).string() << "'.";
        return Err(Error(Error::Kind::NoConfigFile, s.str()));
    }

    return Ok(config);
}

std::optional<Config::Error> Config::load()
{
    std::optional<Config::Error> result;

    if (_loaded)
    {
        std::ostringstream s;
        s << "Warning: Tried to load files for config with client version " << _version << " that is already loaded.";
        VME_LOG(s.str());
        return result;
    }

    std::ifstream fileStream(_configFile);
    nlohmann::json config;
    fileStream >> config;
    fileStream.close();

    auto assetFolderEntry = config.find("assetFolder");
    if (assetFolderEntry == config.end())
    {
        std::stringstream s;
        s << "key 'assetFolder' is required in config.json. You can add it here: " << util::unicodePath(_configFile);
        result.emplace(Error::Kind::MissingConfigJsonKey, s.str());
        return result;
    }

    _assetFolder = assetFolderEntry.value().get<std::string>();

    if (!std::filesystem::exists(_assetFolder) || !std::filesystem::is_directory(_assetFolder))
    {
        std::stringstream s;
        s << "Could not locate config file for client version " << _version << ". You need to add a config.json file at: " + std::filesystem::absolute(_configFile).string() << std::endl;
        result.emplace(Error::Kind::NoConfigFile, s.str());
        return result;
    }

    // Paths are okay, begin loading files

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    Appearances::loadTextureAtlases(_assetFolder / CatalogContentFile, _assetFolder);
    Appearances::loadAppearanceData(_dataFolder / AppearancesFile);

    Items::loadFromOtb(_dataFolder / "items.otb");
    Items::loadMissingItemTypes();
    Items::loadFromXml(_dataFolder / "items.xml");

    VME_LOG_D(std::format("Items: {} (highest server ID: {})", Items::items.size(), Items::items.highestServerId));
    VME_LOG_D("Client object count: " << Appearances::objectCount());

    _loaded = true;
    return result;
}

void Config::loadOrTerminate()
{
    auto error = load();
    if (error)
    {
        ABORT_PROGRAM(error.value().show());
    }
}

std::string Config::Error::show() const
{
    std::ostringstream s;
    s << "Configuration error (";
    switch (kind)
    {
        case Kind::NoConfigFile:
            s << "NoConfigFile";
            break;
        default:
            s << "Unknown";
            break;
    }

    s << "):" << std::endl;
    s << message;

    return s.str();
}
