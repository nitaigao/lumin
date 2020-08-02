#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <map>

#include "iplatform.h"
#include "idisplay_config.h"
#include "ios.h"
#include "server.h"
#include "seat.h"
#include "cursor.h"
#include "view.h"
#include "output.h"

#include "mocks.h"

using ::testing::Return;
using ::testing::DoDefault;
using ::testing::ReturnNull;
using ::testing::Exactly;
using ::testing::_;
using ::testing::NiceMock;

using namespace lumin;

class ServerTest : public ::testing::Test {
 public:
  std::shared_ptr<MockPlatform> platform;
  std::shared_ptr<MockOS> os;
  std::shared_ptr<MockDisplayConfig> display_config;
  std::shared_ptr<MockCursor> cursor;

  std::shared_ptr<Server> subject;

 protected:
  void SetUp() override {
    platform = std::make_shared<NiceMock<MockPlatform>>();
    os = std::make_shared<NiceMock<MockOS>>();
    display_config = std::make_shared<NiceMock<MockDisplayConfig>>();
    cursor = std::make_shared<NiceMock<MockCursor>>();

    subject = std::make_shared<Server>(platform, os, display_config, cursor);

    ON_CALL(*platform, init).WillByDefault(Return(true));
    ON_CALL(*platform, start).WillByDefault(Return(true));
    ON_CALL(*platform, cursor).WillByDefault(Return(cursor));
  }
};

TEST_F(ServerTest, InitReturnsFalseIfThePlatformFailsToInit) {
  EXPECT_CALL(*platform, init).WillOnce(Return(false));

  auto result = subject->init();

  EXPECT_FALSE(result);
}

TEST_F(ServerTest, InitReturnsFalseIfThePlatformFailsToStart) {
  EXPECT_CALL(*platform, start).WillOnce(Return(false));

  auto result = subject->init();

  EXPECT_FALSE(result);
}

TEST_F(ServerTest, InitReturnsTrueOnSuccessfulStartup) {
  auto result = subject->init();
  EXPECT_TRUE(result);
}

TEST_F(ServerTest, InitRegistersForEvents) {
  subject->init();

  EXPECT_EQ(cursor->on_button.current_id_, 1);
  EXPECT_EQ(cursor->on_move.current_id_, 1);

  EXPECT_EQ(platform->on_new_keyboard.current_id_, 1);
  EXPECT_EQ(platform->on_new_output.current_id_, 1);
  EXPECT_EQ(platform->on_new_view.current_id_, 1);

  EXPECT_EQ(platform->on_lid_switch.current_id_, 1);
}

TEST_F(ServerTest, SetsDesktopEnvVars) {
  EXPECT_CALL(*os, set_env("MOZ_ENABLE_WAYLAND", "1")).Times(Exactly(1));
  EXPECT_CALL(*os, set_env("QT_QPA_PLATFORM", "wayland")).Times(Exactly(1));
  EXPECT_CALL(*os, set_env("QT_QPA_PLATFORMTHEME", "gnome")).Times(Exactly(1));
  EXPECT_CALL(*os, set_env("XDG_CURRENT_DESKTOP", "sway")).Times(Exactly(1));
  EXPECT_CALL(*os, set_env("XDG_SESSION_TYPE", "wayland")).Times(Exactly(1));

  auto result = subject->init();

  EXPECT_TRUE(result);
}

TEST_F(ServerTest, MinimizeViewMinimizedAView) {
  MockView view;
  view.on_minimize.connect_member(subject.get(), &Server::view_minimized);
  view.minimize();

  EXPECT_TRUE(view.minimized);
}

TEST_F(ServerTest, OutputConnectedConfiguresOutputsWithALayout) {
  OutputConfig outputConfig = { .scale = 1, .primary = true };
  const char *name = "TEST";

  std::map<std::string, OutputConfig> config;
  config[name] = outputConfig;

  EXPECT_CALL(*display_config, find_layout(_)).WillOnce(Return(config));

  auto output = std::make_shared<MockOutput>();

  EXPECT_CALL(*output, is_named(name)).WillOnce(Return(true));
  EXPECT_CALL(*output, configure(outputConfig.scale, outputConfig.primary)).Times(Exactly(1));

  subject->outputs_.push_back(output);

  subject->outputs_changed(output.get());
}

TEST_F(ServerTest, LidSwitchedDisablesTheOutputWhenShut) {
  auto output = std::make_shared<MockOutput>();

  EXPECT_CALL(*output, is_named("LVDS1")).WillRepeatedly(Return(false));
  EXPECT_CALL(*output, is_named("eDP-1")).WillRepeatedly(Return(true));
  EXPECT_CALL(*output, set_connected(false));

  subject->outputs_.push_back(output);

  subject->lid_switch(false);
}

TEST_F(ServerTest, LidSwitchedEnablesTheOutputWhenOpened) {
  auto output = std::make_shared<MockOutput>();

  EXPECT_CALL(*output, is_named("LVDS1")).WillRepeatedly(Return(true));
  EXPECT_CALL(*output, is_named("eDP-1")).WillRepeatedly(Return(false));
  EXPECT_CALL(*output, set_connected(true));

  subject->outputs_.push_back(output);

  subject->lid_switch(true);
}

TEST_F(ServerTest, OutputsChangedConfiguresTheOutputWithANewLayout) {
  const char *name = "TEST";

  auto output = std::make_shared<MockOutput>();
  EXPECT_CALL(*output, is_named(name)).WillRepeatedly(Return(true));
  subject->outputs_.push_back(output);

  int scale = 2;
  bool primary = false;

  OutputConfig outputConfig { .scale = scale, .primary = primary };

  std::map<std::string, OutputConfig> config;
  config[name] = outputConfig;

  EXPECT_CALL(*display_config, find_layout(subject->outputs_)).WillOnce(Return(config));

  EXPECT_CALL(*output, configure(scale, primary));

  subject->outputs_changed(NULL);
}
