#include "commutils/logger_.h"
#include "gtest/gtest.h"
#ifdef _WITH_SPDLOG
#include "spdlog/sinks/ostream_sink.h"
#endif

TEST(Logger, FormatsBothApisAndPreservesBracesInStreamMessages) {
#ifdef _WITH_SPDLOG
  std::ostringstream output;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(output);
  auto previous = spdlog::default_logger();
  auto logger = std::make_shared<spdlog::logger>("regression", sink);
  spdlog::set_default_logger(logger);
  INITIALIZE_DEFAULT_LOGGER("%v", TRACE);
  EXPECT_EQ(spdlog::get_level(), spdlog::level::trace);
#else
  testing::internal::CaptureStdout();
#endif
  LOG(INFO) << "literal {braces}";
  LOG_INFO("formatted {}", 42);
  LOG_DEBUG("debug {}", 7);
  LOG_WARNING("warning {}", 8);
#ifdef _WITH_SPDLOG
  const std::string text = output.str();
  spdlog::set_default_logger(previous);
#else
  const std::string text = testing::internal::GetCapturedStdout();
#endif
  EXPECT_NE(text.find("literal {braces}"), std::string::npos);
  EXPECT_NE(text.find("formatted 42"), std::string::npos);
  EXPECT_NE(text.find("debug 7"), std::string::npos);
  EXPECT_NE(text.find("warning 8"), std::string::npos);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
