#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "config.h"

namespace {

class ConfigLoadFromFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        temp_dir_ = std::filesystem::path(MCP_PROJECT_ROOT) / "tests" / "tmp_config_load_from_file" /
                    test_info->test_suite_name() / test_info->name();
        config_file_ = temp_dir_ / "server.json";
        std::error_code error;
        std::filesystem::remove_all(temp_dir_, error);
        std::filesystem::create_directories(temp_dir_, error);
        ASSERT_FALSE(error) << error.message();
    }

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(temp_dir_, error);
    }

    void WriteConfig(const std::string& content) const {
        std::ofstream output(config_file_);
        ASSERT_TRUE(output.is_open());
        output << content;
    }

    std::filesystem::path temp_dir_;
    std::filesystem::path config_file_;
};

}  // namespace

TEST_F(ConfigLoadFromFileTest, LoadsMinimalServerConfigAndAppliesLoggingDefaults) {
    WriteConfig(R"({
  "server": {
    "port": 9090
  }
})");

    auto& config = mcp::Config::GetInstance();

    ASSERT_TRUE(config.LoadFromFile(config_file_.string()));
    EXPECT_TRUE(config.IsLoaded());
    EXPECT_EQ(config.GetServerPort(), 9090);
    EXPECT_EQ(config.GetLogFilePath(), "../../logs/server.log");
    EXPECT_EQ(config.GetLogLevel(), "info");
    EXPECT_EQ(config.GetLogFileSize(), 10 * 1024 * 1024);
    EXPECT_EQ(config.GetLogFileCount(), 5);
    EXPECT_TRUE(config.GetLogConsoleOutput());
}

TEST_F(ConfigLoadFromFileTest, LoadsConfigWithoutServerSectionAndUsesDefaultPort) {
    WriteConfig(R"({
  "logging": {
    "log_level": "debug"
  }
})");

    auto& config = mcp::Config::GetInstance();

    ASSERT_TRUE(config.LoadFromFile(config_file_.string()));
    EXPECT_TRUE(config.IsLoaded());
    EXPECT_EQ(config.GetServerPort(), 8080);
    EXPECT_EQ(config.GetLogLevel(), "debug");
}

TEST_F(ConfigLoadFromFileTest, RejectsConfigWhenPortIsOutOfRange) {
    WriteConfig(R"({
  "server": {
    "port": 70000
  }
})");

    auto& config = mcp::Config::GetInstance();

    EXPECT_FALSE(config.LoadFromFile(config_file_.string()));
}

TEST_F(ConfigLoadFromFileTest, RejectsConfigWhenLogLevelIsInvalid) {
    WriteConfig(R"({
  "server": {
    "port": 8080
  },
  "logging": {
    "log_level": "verbose"
  }
})");

    auto& config = mcp::Config::GetInstance();

    EXPECT_FALSE(config.LoadFromFile(config_file_.string()));
}

TEST_F(ConfigLoadFromFileTest, RejectsConfigWhenLogFileSizeIsZero) {
    WriteConfig(R"({
  "server": {
    "port": 8080
  },
  "logging": {
    "log_file_size": 0
  }
})");

    auto& config = mcp::Config::GetInstance();

    EXPECT_FALSE(config.LoadFromFile(config_file_.string()));
}

TEST_F(ConfigLoadFromFileTest, ReturnsFalseWhenConfigFileDoesNotExist) {
    auto missing_file = temp_dir_ / "missing_server.json";

    auto& config = mcp::Config::GetInstance();

    EXPECT_FALSE(config.LoadFromFile(missing_file.string()));
}
