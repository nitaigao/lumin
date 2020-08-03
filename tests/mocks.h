#ifndef MOCKS_H_
#define MOCKS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "idisplay_config.h"
#include "ios.h"
#include "iplatform.h"
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

class MockCursor : public ICursor {
 public:
  MOCK_METHOD(void, load_scale, (int));
  MOCK_METHOD(int, x, (), (const));
  MOCK_METHOD(int, y, (), (const));
  MOCK_METHOD(void, set_surface, (wlr_surface*, int, int));
  MOCK_METHOD(void, add_device, (wlr_input_device*));
  MOCK_METHOD(void, set_image, (const std::string&));
  MOCK_METHOD(void, begin_interactive, (View*, CursorMode, unsigned int));
};

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
  MOCK_METHOD(std::shared_ptr<ICursor>, cursor, (), (const));

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
  MOCK_METHOD(void, configure, (int, bool, bool enabled, int x, int y));
  MOCK_METHOD(void, take_damage, (const View *));
  MOCK_METHOD(void, take_whole_damage, ());
  MOCK_METHOD(bool, is_named, (const std::string&), (const));
  MOCK_METHOD(bool, connected, (), (const));
  MOCK_METHOD(void, set_connected, (bool));
  MOCK_METHOD(bool, deleted, () , (const));
  MOCK_METHOD(void, mark_deleted, (), ());
  MOCK_METHOD(bool, primary, (), (const));
  MOCK_METHOD(int, width, (), (const));
};

class MockOS : public IOS {
 public:
  MOCK_METHOD(void, set_env, (const std::string&, const std::string&), ());
  MOCK_METHOD(std::string, open_file, (const std::string&), ());
  MOCK_METHOD(bool, file_exists, (const std::string&), ());
  MOCK_METHOD(void, execute, (const std::string&), ());
};

#endif  // MOCKS_H_
