/**
 * @file config_test.cpp
 * @brief Unit tests for ClientConfig validation
 */

#include <gtest/gtest.h>
#include "ascnd/client.hpp"
#include <stdexcept>

namespace ascnd {
namespace {

class ConfigTest : public ::testing::Test {
protected:
    ClientConfig valid_config;

    void SetUp() override {
        valid_config.server_address = "api.example.com:443";
        valid_config.api_key = "test-api-key";
        valid_config.use_ssl = true;
        valid_config.connection_timeout_ms = 5000;
        valid_config.request_timeout_ms = 10000;
        valid_config.max_retries = 3;
        valid_config.retry_delay_ms = 100;
    }
};

// Test that valid config passes validation
TEST_F(ConfigTest, ValidConfigPasses) {
    EXPECT_NO_THROW(valid_config.validate());
}

// Test that empty server_address fails validation
TEST_F(ConfigTest, EmptyServerAddressFails) {
    valid_config.server_address = "";

    EXPECT_THROW({
        valid_config.validate();
    }, std::invalid_argument);

    try {
        valid_config.validate();
    } catch (const std::invalid_argument& e) {
        EXPECT_STREQ(e.what(), "server_address cannot be empty");
    }
}

// Test that zero connection_timeout fails
TEST_F(ConfigTest, ZeroConnectionTimeoutFails) {
    valid_config.connection_timeout_ms = 0;

    EXPECT_THROW({
        valid_config.validate();
    }, std::invalid_argument);
}

// Test that negative connection_timeout fails
TEST_F(ConfigTest, NegativeConnectionTimeoutFails) {
    valid_config.connection_timeout_ms = -1;

    EXPECT_THROW({
        valid_config.validate();
    }, std::invalid_argument);

    try {
        valid_config.validate();
    } catch (const std::invalid_argument& e) {
        EXPECT_STREQ(e.what(), "connection_timeout_ms must be positive");
    }
}

// Test that zero request_timeout fails
TEST_F(ConfigTest, ZeroRequestTimeoutFails) {
    valid_config.request_timeout_ms = 0;

    EXPECT_THROW({
        valid_config.validate();
    }, std::invalid_argument);
}

// Test that negative request_timeout fails
TEST_F(ConfigTest, NegativeRequestTimeoutFails) {
    valid_config.request_timeout_ms = -100;

    EXPECT_THROW({
        valid_config.validate();
    }, std::invalid_argument);

    try {
        valid_config.validate();
    } catch (const std::invalid_argument& e) {
        EXPECT_STREQ(e.what(), "request_timeout_ms must be positive");
    }
}

// Test that negative max_retries fails
TEST_F(ConfigTest, NegativeMaxRetriesFails) {
    valid_config.max_retries = -1;

    EXPECT_THROW({
        valid_config.validate();
    }, std::invalid_argument);

    try {
        valid_config.validate();
    } catch (const std::invalid_argument& e) {
        EXPECT_STREQ(e.what(), "max_retries cannot be negative");
    }
}

// Test that zero max_retries is valid
TEST_F(ConfigTest, ZeroMaxRetriesValid) {
    valid_config.max_retries = 0;

    EXPECT_NO_THROW(valid_config.validate());
}

// Test that negative retry_delay fails
TEST_F(ConfigTest, NegativeRetryDelayFails) {
    valid_config.retry_delay_ms = -50;

    EXPECT_THROW({
        valid_config.validate();
    }, std::invalid_argument);

    try {
        valid_config.validate();
    } catch (const std::invalid_argument& e) {
        EXPECT_STREQ(e.what(), "retry_delay_ms cannot be negative");
    }
}

// Test that zero retry_delay is valid
TEST_F(ConfigTest, ZeroRetryDelayValid) {
    valid_config.retry_delay_ms = 0;

    EXPECT_NO_THROW(valid_config.validate());
}

// Test default config values
TEST_F(ConfigTest, DefaultConfigValues) {
    ClientConfig config;

    EXPECT_TRUE(config.server_address.empty());
    EXPECT_TRUE(config.api_key.empty());
    EXPECT_TRUE(config.use_ssl);
    EXPECT_EQ(config.connection_timeout_ms, 5000);
    EXPECT_EQ(config.request_timeout_ms, 10000);
    EXPECT_EQ(config.max_retries, 3);
    EXPECT_EQ(config.retry_delay_ms, 100);
    EXPECT_TRUE(config.user_agent.empty());
    EXPECT_FALSE(config.verbose);
}

// Test that empty api_key is allowed (optional)
TEST_F(ConfigTest, EmptyApiKeyAllowed) {
    valid_config.api_key = "";

    EXPECT_NO_THROW(valid_config.validate());
}

// Test that empty user_agent is allowed (optional)
TEST_F(ConfigTest, EmptyUserAgentAllowed) {
    valid_config.user_agent = "";

    EXPECT_NO_THROW(valid_config.validate());
}

// Test config with only required fields
TEST_F(ConfigTest, MinimalValidConfig) {
    ClientConfig config;
    config.server_address = "localhost:50051";

    EXPECT_NO_THROW(config.validate());
}

// Test config with all fields set
TEST_F(ConfigTest, FullConfig) {
    ClientConfig config;
    config.server_address = "api.ascnd.gg:443";
    config.api_key = "super-secret-key";
    config.use_ssl = true;
    config.connection_timeout_ms = 10000;
    config.request_timeout_ms = 30000;
    config.max_retries = 5;
    config.retry_delay_ms = 200;
    config.user_agent = "MyGame/1.0";
    config.verbose = true;

    EXPECT_NO_THROW(config.validate());
}

// Test that validation is called when creating client with invalid config
TEST_F(ConfigTest, ClientConstructorValidatesConfig) {
    ClientConfig invalid_config;
    invalid_config.server_address = "";  // Invalid

    EXPECT_THROW({
        AscndClient client(invalid_config);
    }, std::invalid_argument);
}

// Test that client constructor with parameters validates
TEST_F(ConfigTest, ClientSimpleConstructorValidatesConfig) {
    EXPECT_THROW({
        AscndClient client("", "api-key");  // Empty server address
    }, std::invalid_argument);
}

}  // namespace
}  // namespace ascnd
