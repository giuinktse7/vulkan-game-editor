#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "../vendor/result/result.h"

class Config
{
  public:
    struct Error
    {
        enum class Kind
        {
            NoConfigFile,
            MissingConfigJsonKey
        };

        std::string show() const;

        Error(Kind kind, std::string message)
            : kind(kind), message(message) {}

        Kind kind;
        std::string message;
    };

    static Result<Config, Config::Error> create(const std::string version);

    std::optional<Config::Error> load();
    void loadOrTerminate();

  private:
    Config(const std::string version);

    std::string _version;
    std::filesystem::path _dataFolder;
    std::filesystem::path _configFile;
    std::filesystem::path _assetFolder;

    bool _loaded;
};

enum class BorderBrushVariationType
{
    Detailed,
    General
};

struct Settings
{
    static BorderBrushVariationType BORDER_BRUSH_VARIATION;
    static bool AUTO_BORDER;
    static int UI_CHANGE_TIME_DELAY_MILLIS;
};