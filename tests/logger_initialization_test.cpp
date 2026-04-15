#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "logger.h"

namespace {

class LoggerInitializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        temp_dir_ = std::filesystem::path(MCP_PROJECT_ROOT) / "tests" / "tmp_logger_initialization" /
                    test_info->test_suite_name() / test_info->name();
        custom_log_file_ = temp_dir_ / "logger_initialization.log";
        second_log_file_ = temp_dir_ / "logger_reinitialized.log";
        mcp::logger::Logger::getInstance().shutdown();
        std::error_code error;
        std::filesystem::remove_all(temp_dir_, error);
        std::filesystem::create_directories(temp_dir_, error);
        ASSERT_FALSE(error) << error.message();
    }

    void TearDown() override {
        mcp::logger::Logger::getInstance().shutdown();
        std::error_code error;
        std::filesystem::remove_all(temp_dir_, error);
    }

    std::string ReadFile(const std::filesystem::path& file_path) const {
        std::ifstream input(file_path);
        return std::string(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
    }

    std::filesystem::path temp_dir_;
    std::filesystem::path custom_log_file_;
    std::filesystem::path second_log_file_;
};

}  // namespace

TEST_F(LoggerInitializationTest, GetLoggerAutoInitializesDefaultLogger) {
    auto logger = mcp::logger::Logger::getInstance().getLogger();

    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->name(), "mcp");
    EXPECT_EQ(spdlog::default_logger_raw(), logger.get());
}

TEST_F(LoggerInitializationTest, InitWithCustomFileWritesLogMessageToDisk) {
    auto& manager = mcp::logger::Logger::getInstance();

    manager.init("file_logger", custom_log_file_.string(), 1024 * 1024, 2, false);

    auto logger = manager.getLogger();
    ASSERT_NE(logger, nullptr);

    logger->info("logger initialization test message");
    manager.flush();

    ASSERT_TRUE(std::filesystem::exists(custom_log_file_));
    EXPECT_NE(ReadFile(custom_log_file_).find("logger initialization test message"), std::string::npos);
}

TEST_F(LoggerInitializationTest, SetLevelUpdatesLoggerLevel) {
    auto& manager = mcp::logger::Logger::getInstance();
    auto logger = manager.getLogger();

    manager.setLevel(spdlog::level::debug);

    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->level(), spdlog::level::debug);
}

TEST_F(LoggerInitializationTest, ShutdownAllowsLoggerToBeReinitializedWithANewName) {
    auto& manager = mcp::logger::Logger::getInstance();

    manager.init("first_logger", custom_log_file_.string(), 1024 * 1024, 2, false);
    manager.shutdown();
    manager.init("second_logger", second_log_file_.string(), 1024 * 1024, 2, false);

    auto logger = manager.getLogger();
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->name(), "second_logger");
}

TEST_F(LoggerInitializationTest, RepeatedInitWithoutShutdownKeepsOriginalLoggerInstance) {
    auto& manager = mcp::logger::Logger::getInstance();

    manager.init("first_logger", custom_log_file_.string(), 1024 * 1024, 2, false);
    auto original_logger = manager.getLogger();

    manager.init("second_logger", second_log_file_.string(), 1024 * 1024, 2, false);
    auto logger_after_second_init = manager.getLogger();

    ASSERT_NE(original_logger, nullptr);
    ASSERT_NE(logger_after_second_init, nullptr);
    EXPECT_EQ(logger_after_second_init->name(), "first_logger");
    EXPECT_EQ(original_logger.get(), logger_after_second_init.get());
    EXPECT_FALSE(std::filesystem::exists(second_log_file_));
}
