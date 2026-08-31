#include "ayonApiGet.h"

#include "appDataFolder.h"
#include "AyonCppApi.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace {
/// Case-insensitive, bounds-safe read of the file-logging flag.
///
/// Enables file logging only on an explicit affirmative; anything else --
/// including an empty value -- leaves it off. Replaces a `switch (value[1])`
/// that read past the NUL terminator for an empty string (undefined behaviour)
/// and treated a lowercase "off" as ON, because only 'F' matched.
bool
fileLoggingIsEnabled(const char* value) {
    if (value == nullptr) {
        return false;
    }
    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return normalized == "on" || normalized == "1" || normalized == "true";
}
}   // namespace

std::unique_ptr<AyonApi>
getAyonApiFromEnv() {
    std::string AYON_SITE_ID;
    const char* AYON_API_KEY = std::getenv("AYON_API_KEY");
    const char* AYON_SERVER_URL = std::getenv("AYON_SERVER_URL");
    const char* AYON_PROJECT_NAME = std::getenv("AYON_PROJECT_NAME");

    const char* envVarFileLoggingPath = std::getenv("AYON_USD_RESOLVER_LOG_FILE");
    const char* envVarFileLogging = std::getenv("AYON_USD_RESOLVER_LOG_FILE_ENABLED");

    if (envVarFileLoggingPath == nullptr) {
        std::cout << "envVarFileLoggingPath is nullptr" << std::endl;
        envVarFileLoggingPath = "";
    }
    if (envVarFileLogging == nullptr) {
        std::cout << "envVarFileLogging is nullptr, setting to OFF" << std::endl;
        envVarFileLogging = "OFF";
    }

    std::cout << AYON_SERVER_URL << ", " << AYON_PROJECT_NAME << ", " << envVarFileLoggingPath << ", " << envVarFileLogging << std::endl;

    if (AYON_API_KEY == nullptr || AYON_SERVER_URL == nullptr || AYON_PROJECT_NAME == nullptr) {
        throw std::runtime_error(
            "Cant find 1 or more env variavbles needed to start the AyonCppApi check if the environment is correct");
    }

    // find the side ID for this system as it might be in the ENV var or in a file
    const char* AYON_SITE_ID_ENV = std::getenv("AYON_SITE_ID");
    if (AYON_SITE_ID_ENV == nullptr) {
        std::ifstream siteIdFile(getAppDataDir() + "/site_id");
        if (!siteIdFile.is_open()) {
            std::runtime_error("");
        }
        std::getline(siteIdFile, AYON_SITE_ID);
        siteIdFile.close();
    }
    else {
        AYON_SITE_ID = AYON_SITE_ID_ENV;
    }

    std::cout << "before fileLoggerFilePath - " << envVarFileLoggingPath << std::endl;
    // Stays std::nullopt unless file logging is both enabled AND given a path.
    // AyonApi takes std::optional<std::string>, so passing "" here yields an
    // *engaged* optional holding an empty string: AyonCppApi builds predating
    // ynput/ayon-cpp-api@63702e0 gate on has_value() alone, let it through, and
    // resolve it to std::filesystem::temp_directory_path() -- then try to open
    // that directory as a log file.
    std::optional<std::string> fileLoggerFilePath;
    if (fileLoggingIsEnabled(envVarFileLogging)) {
        if (envVarFileLoggingPath[0] == '\0') {
            // Without this, absolute("" + "/logFile.json") writes to /logFile.json.
            std::cout << "file logging is ON but the log path is empty; leaving it off" << std::endl;
        }
        else {
            std::cout << "file logging is ON" << std::endl;
            fileLoggerFilePath
                = std::filesystem::absolute(std::string(envVarFileLoggingPath) + "/logFile.json").string();
        }
    }
    else {
        std::cout << "file logging is OFF" << std::endl;
    }
    std::cout << "before api init" << std::endl;
    std::unique_ptr<AyonApi> api
        = std::make_unique<AyonApi>(fileLoggerFilePath, AYON_API_KEY, AYON_SERVER_URL, AYON_PROJECT_NAME, AYON_SITE_ID);
    std::cout << "before return" << std::endl;
    return api;
};
