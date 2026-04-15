#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "config.h"
#include "logger.h"

namespace {
class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mcp::logger::Logger::GetInstance().ShutDown();
        log_dir_ = std::filesystem::path(MCP_PROJECT_ROOT) / "log";
        std::filesystem::create_directories(log_dir_);
        default_log_file_ = log_dir_ / "mcp.log";
        custom_log_file_ = log_dir_ / "logger_test.log";
        configured_log_file_ = log_dir_ / "configured_logger.log";
        config_dir_ = std::filesystem::path(MCP_PROJECT_ROOT) / "tests" / "tmp_config";
        config_file_ = config_dir_ / "server.json";
        std::filesystem::create_directories(config_dir_);
        std::filesystem::remove(default_log_file_);
        std::filesystem::remove(custom_log_file_);
        std::filesystem::remove(configured_log_file_);
        std::filesystem::remove(config_file_);

        std::ofstream output(config_file_);
        output << R"({
  "server": {
    "port": 8080
  },
  "logging": {
    "log_file_path": "../../log/mcp.log",
    "log_level": "info",
    "log_file_size": 1048576,
    "log_file_count": 3,
    "log_console_output": false
  }
})";
        output.close();

        ASSERT_TRUE(mcp::Config::GetInstance().LoadFromFile(config_file_.string()));
    }

    void TearDown() override {
        mcp::logger::Logger::GetInstance().ShutDown();
        std::filesystem::remove(default_log_file_);
        std::filesystem::remove(custom_log_file_);
        std::filesystem::remove(configured_log_file_);
        std::filesystem::remove(config_file_);
        std::filesystem::remove_all(config_dir_);
    }

    std::filesystem::path log_dir_;
    std::filesystem::path default_log_file_;
    std::filesystem::path custom_log_file_;
    std::filesystem::path configured_log_file_;
    std::filesystem::path config_dir_;
    std::filesystem::path config_file_;
};
} // namespace

TEST_F(LoggerTest, GetLoggerInitializesDefaultLogger) {
    auto logger = mcp::logger::Logger::GetInstance().GetLogger();

    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->name(), "mcp");
    mcp::logger::Logger::GetInstance().Flush();
    EXPECT_TRUE(std::filesystem::exists(default_log_file_));
}

TEST_F(LoggerTest, SetLevelUpdatesLoggerLevel) {
    auto& manager = mcp::logger::Logger::GetInstance();
    auto logger = manager.GetLogger();

    manager.SetLevel(spdlog::level::debug);

    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->level(), spdlog::level::debug);
}

TEST_F(LoggerTest, InitWithFileCreatesLogFileAndWritesMessage) {
    auto& manager = mcp::logger::Logger::GetInstance();
    manager.init("logger_test", custom_log_file_.string(), 1024 * 1024, 2, false);

    auto logger = manager.GetLogger();
    ASSERT_NE(logger, nullptr);

    logger->info("logger file test message");
    manager.Flush();

    ASSERT_TRUE(std::filesystem::exists(custom_log_file_));

    std::ifstream input(custom_log_file_);
    std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("logger file test message"), std::string::npos);
}

TEST_F(LoggerTest, ShutDownAllowsReinitializationWithNewName) {
    auto& manager = mcp::logger::Logger::GetInstance();
    manager.init("first_logger", "", 1024 * 1024, 2, true);
    manager.ShutDown();
    manager.init("second_logger", "", 1024 * 1024, 2, true);

    auto logger = manager.GetLogger();
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->name(), "second_logger");
}

TEST_F(LoggerTest, InitUsesConfiguredLogFilePathWhenArgumentIsEmpty) {
    std::ofstream output(config_file_);
    output << R"({
  "server": {
    "port": 8080
  },
  "logging": {
    "log_file_path": "../../log/configured_logger.log",
    "log_level": "info",
    "log_file_size": 1048576,
    "log_file_count": 3,
    "log_console_output": false
  }
})";
    output.close();

    ASSERT_TRUE(mcp::Config::GetInstance().LoadFromFile(config_file_.string()));

    auto& manager = mcp::logger::Logger::GetInstance();
    manager.init("configured_logger", "", 1024 * 1024, 2, false);

    auto logger = manager.GetLogger();
    ASSERT_NE(logger, nullptr);

    logger->info("configured path message");
    manager.Flush();

    ASSERT_TRUE(std::filesystem::exists(configured_log_file_));

    std::ifstream input(configured_log_file_);
    std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("configured path message"), std::string::npos);
}
