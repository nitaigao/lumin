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

using ::testing::Return;
using ::testing::DoDefault;
using ::testing::ReturnNull;
using ::testing::Exactly;
using ::testing::_;
using ::testing::NiceMock;

using namespace lumin;

typedef std::map<std::string, OutputConfig> OutputConfigReturn;

class MockDisplayConfig : public IDisplayConfig {
 public:
  MOCK_METHOD(OutputConfigReturn, find_layout,
    (const std::vector<std::shared_ptr<IOutput>>& outputs));
};

class MockPlatform : public IPlatform {
 public:
  MOCK_METHOD(bool, init, ());
  MOCK_METHOD(bool, start, ());
  MOCK_METHOD(void, run, ());
  MOCK_METHOD(void, destroy, ());
  MOCK_METHOD(void, terminate, ());

  MOCK_METHOD(std::shared_ptr<Seat>, seat, (), (const));
  MOCK_METHOD(std::shared_ptr<Cursor>, cursor, (), (const));

  MOCK_METHOD(void, add_idle, (idle_func, void*));
  MOCK_METHOD(Output*, output_at, (int, int), (const));
};

class MockView : public View {
 public:
  MockView() : View(nullptr, nullptr, nullptr) { }

  MOCK_METHOD(std::string, id, (), (const));
  MOCK_METHOD(std::string, title, (), (const));

  MOCK_METHOD(void, enter, (const Output* output), ());

  MOCK_METHOD(void, geometry, (wlr_box *box), (const));
  MOCK_METHOD(void, extents, (wlr_box *box), (const));

  MOCK_METHOD(void, move, (int x, int y), ());
  MOCK_METHOD(void, resize, (double width, double height), ());

  MOCK_METHOD(uint, min_width, (), (const));
  MOCK_METHOD(uint, min_height, (), (const));

  MOCK_METHOD(bool, is_root, (), (const));
  MOCK_METHOD(View*, parent, (), (const));

  MOCK_METHOD(const View*, root, (), (const));

  MOCK_METHOD(void, set_tiled, (int edges), ());
  MOCK_METHOD(void, set_maximized, (bool maximized), ());
  MOCK_METHOD(void, set_size, (int width, int height), ());

  MOCK_METHOD(bool, has_surface, (const wlr_surface *surface), (const));
  MOCK_METHOD(void, for_each_surface, (wlr_surface_iterator_func_t iterator, void *data), (const));
  MOCK_METHOD(wlr_surface*, surface_at, (double sx, double sy, double *sub_x, double *sub_y), ());

  MOCK_METHOD(void, activate, (), ());
  MOCK_METHOD(void, deactivate, (), ());

  MOCK_METHOD(wlr_surface*, surface, (), (const));
  MOCK_METHOD(bool, steals_focus, (), (const));
};

class MockOutput : public IOutput {
 public:
  MOCK_METHOD(std::string, id, (), (const));
  MOCK_METHOD(void, configure, (int, bool));
  MOCK_METHOD(void, take_damage, (const View *));
  MOCK_METHOD(void, take_whole_damage, ());
  MOCK_METHOD(bool, is_named, (const std::string&), (const));
  MOCK_METHOD(void, set_connected, (bool));
  MOCK_METHOD(bool, deleted, () , (const));
  MOCK_METHOD(void, mark_deleted, (), ());
  MOCK_METHOD(bool, primary, (), (const));
};

class MockOS : public IOS {
 public:
  MOCK_METHOD(void, set_env, (const std::string&, const std::string&), ());
};

class ServerTest : public ::testing::Test {
 public:
  std::unique_ptr<MockPlatform> mockedPlatform;
  std::unique_ptr<MockOS> mockedOS;
  std::unique_ptr<MockDisplayConfig> mockedDisplayConfig;

 protected:
  void SetUp() override {
    mockedPlatform = std::make_unique<NiceMock<MockPlatform>>();
    mockedOS = std::make_unique<NiceMock<MockOS>>();
    mockedDisplayConfig = std::make_unique<NiceMock<MockDisplayConfig>>();

    ON_CALL(*mockedPlatform, init).WillByDefault(Return(true));
    ON_CALL(*mockedPlatform, start).WillByDefault(Return(true));
  }
};

TEST_F(ServerTest, InitReturnsFalseIfThePlatformFailsToInit) {
  EXPECT_CALL(*mockedPlatform, init).WillOnce(Return(false));

  std::unique_ptr<IPlatform> platform = std::move(mockedPlatform);
  std::unique_ptr<IOS> os = std::move(mockedOS);
  std::unique_ptr<IDisplayConfig> display_config = std::move(mockedDisplayConfig);

  Server server(platform, os, display_config);
  auto result = server.init();

  EXPECT_FALSE(result);
}

TEST_F(ServerTest, InitReturnsFalseIfThePlatformFailsToStart) {
  EXPECT_CALL(*mockedPlatform, start).WillOnce(Return(false));

  std::unique_ptr<IPlatform> platform = std::move(mockedPlatform);
  std::unique_ptr<IOS> os = std::move(mockedOS);
  std::unique_ptr<IDisplayConfig> display_config = std::move(mockedDisplayConfig);

  Server server(platform, os, display_config);
  auto result = server.init();

  EXPECT_FALSE(result);
}

TEST_F(ServerTest, InitReturnsTrueOnSuccessfulStartup) {
  std::unique_ptr<IPlatform> platform = std::move(mockedPlatform);
  std::unique_ptr<IOS> os = std::move(mockedOS);
  std::unique_ptr<IDisplayConfig> display_config = std::move(mockedDisplayConfig);

  Server server(platform, os, display_config);
  auto result = server.init();

  EXPECT_TRUE(result);
}

TEST_F(ServerTest, InitRegistersForEvents) {
  std::unique_ptr<IPlatform> platform = std::move(mockedPlatform);
  std::unique_ptr<IOS> os = std::move(mockedOS);
  std::unique_ptr<IDisplayConfig> display_config = std::move(mockedDisplayConfig);

  Server server(platform, os, display_config);
  server.init();

  EXPECT_EQ(server.platform_->on_cursor_button.current_id_, 1);
  EXPECT_EQ(server.platform_->on_cursor_motion.current_id_, 1);

  EXPECT_EQ(server.platform_->on_new_keyboard.current_id_, 1);
  EXPECT_EQ(server.platform_->on_new_output.current_id_, 1);
  EXPECT_EQ(server.platform_->on_new_view.current_id_, 1);

  EXPECT_EQ(server.platform_->on_lid_switch.current_id_, 1);
}

TEST_F(ServerTest, SetsDesktopEnvVars) {
  EXPECT_CALL(*mockedOS, set_env("MOZ_ENABLE_WAYLAND", "1")).Times(Exactly(1));
  EXPECT_CALL(*mockedOS, set_env("QT_QPA_PLATFORM", "wayland")).Times(Exactly(1));
  EXPECT_CALL(*mockedOS, set_env("QT_QPA_PLATFORMTHEME", "gnome")).Times(Exactly(1));
  EXPECT_CALL(*mockedOS, set_env("XDG_CURRENT_DESKTOP", "sway")).Times(Exactly(1));
  EXPECT_CALL(*mockedOS, set_env("XDG_SESSION_TYPE", "wayland")).Times(Exactly(1));

  std::unique_ptr<IPlatform> platform = std::move(mockedPlatform);
  std::unique_ptr<IOS> os = std::move(mockedOS);
  std::unique_ptr<IDisplayConfig> display_config = std::move(mockedDisplayConfig);

  Server server(platform, os, display_config);
  auto result = server.init();

  EXPECT_TRUE(result);
}


TEST_F(ServerTest, MinimizeViewMinimizedAView) {
  std::unique_ptr<IPlatform> platform = std::move(mockedPlatform);
  std::unique_ptr<IOS> os = std::move(mockedOS);
  std::unique_ptr<IDisplayConfig> display_config = std::move(mockedDisplayConfig);

  Server server(platform, os, display_config);

  MockView view;
  view.on_minimize.connect_member(&server, &Server::view_minimized);
  view.minimize();

  EXPECT_TRUE(view.minimized);
}

TEST_F(ServerTest, OutputConnectedConfiguresOutputsWithALayout) {
  OutputConfig outputConfig = { .scale = 1, .primary = true };
  const char *name = "TEST";

  std::map<std::string, OutputConfig> config;
  config[name] = outputConfig;

  EXPECT_CALL(*mockedDisplayConfig, find_layout(_)).WillOnce(Return(config));

  std::unique_ptr<IPlatform> platform = std::move(mockedPlatform);
  std::unique_ptr<IOS> os = std::move(mockedOS);
  std::unique_ptr<IDisplayConfig> display_config = std::move(mockedDisplayConfig);

  auto output = std::make_shared<MockOutput>();

  EXPECT_CALL(*output, id()).WillOnce(Return(name));
  EXPECT_CALL(*output, configure(outputConfig.scale, outputConfig.primary)).Times(Exactly(1));

  Server server(platform, os, display_config);
  server.outputs_.push_back(output);

  server.output_connected(output.get());
}
