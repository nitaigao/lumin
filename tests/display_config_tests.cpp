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

#include "display_config.h"

#include "mocks.h"

using ::testing::Return;
using ::testing::DoDefault;
using ::testing::ReturnNull;
using ::testing::Exactly;
using ::testing::_;
using ::testing::NiceMock;

using namespace lumin;

class DisplayConfigTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockOS> os_;

  std::shared_ptr<MockOutput> output1_;
  std::shared_ptr<MockOutput> output2_;
  std::shared_ptr<MockOutput> output3_;

  std::shared_ptr<DisplayConfig> subject_;

  void SetUp() override {
    os_ = std::make_shared<MockOS>();

    output1_.reset();
    output2_.reset();
    output3_.reset();

    output1_ = std::make_shared<MockOutput>();
    EXPECT_CALL(*output1_, id).WillRepeatedly(Return("OUTPUT1"));
    EXPECT_CALL(*output1_, connected).WillRepeatedly(Return(true));

    output2_ = std::make_shared<MockOutput>();
    EXPECT_CALL(*output2_, id).WillRepeatedly(Return("OUTPUT2"));
    EXPECT_CALL(*output2_, connected).WillRepeatedly(Return(true));

    output3_ = std::make_shared<MockOutput>();
    EXPECT_CALL(*output3_, id).WillRepeatedly(Return("OUTPUT3"));
    EXPECT_CALL(*output3_, connected).WillRepeatedly(Return(false));

    subject_ = std::make_shared<DisplayConfig>(os_);
  }
};

TEST_F(DisplayConfigTest, GivesADefaultLayoutWhenTheConfigFileDoesntExist) {
  std::vector<std::shared_ptr<IOutput>> outputs;

  outputs.push_back(output1_);

  std::stringstream data;
  EXPECT_CALL(*os_, file_exists("$HOME/.config/monitors")).WillOnce(Return(false));

  auto result = subject_->find_layout(outputs);
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result.begin()->first, "OUTPUT1");

  EXPECT_EQ(result["OUTPUT1"].primary, true);
  EXPECT_EQ(result["OUTPUT1"].scale, 1);
}

TEST_F(DisplayConfigTest, GivesADefaultLayoutWhenNoMatches) {
  std::vector<std::shared_ptr<IOutput>> outputs;

  outputs.push_back(output1_);
  outputs.push_back(output2_);

  std::stringstream data;
  EXPECT_CALL(*os_, file_exists).WillOnce(Return(true));
  EXPECT_CALL(*os_, open_file).WillOnce(Return(data.str()));

  auto result = subject_->find_layout(outputs);
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result.begin()->first, "OUTPUT1");
  EXPECT_EQ(result.rbegin()->first, "OUTPUT2");

  EXPECT_EQ(result["OUTPUT1"].primary, true);
  EXPECT_EQ(result["OUTPUT1"].scale, 1);

  EXPECT_EQ(result["OUTPUT2"].primary, false);
  EXPECT_EQ(result["OUTPUT2"].scale, 1);
}

TEST_F(DisplayConfigTest, MatchesASingleConfig) {
  std::vector<std::shared_ptr<IOutput>> outputs;

  outputs.push_back(output1_);

  std::stringstream data;
  data << R"(
    - - name: OUTPUT1
        scale: 1
        primary: true
    - - name: OUTPUT2
        scale: 2
        primary: false
  )";

  EXPECT_CALL(*os_, file_exists).WillOnce(Return(true));
  EXPECT_CALL(*os_, open_file).WillOnce(Return(data.str()));

  auto result = subject_->find_layout(outputs);
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result.begin()->first, "OUTPUT1");
}

TEST_F(DisplayConfigTest, MatchesADoubleConfig) {
  std::vector<std::shared_ptr<IOutput>> outputs;

  outputs.push_back(output1_);
  outputs.push_back(output2_);

  std::stringstream data;
  data << R"(
    - - name: OUTPUT3
        scale: 1
        primary: true
      - name: OUTPUT4
        scale: 2
        primary: false
    - - name: OUTPUT1
        scale: 1
        primary: true
      - name: OUTPUT2
        scale: 2
        primary: false
  )";

  EXPECT_CALL(*os_, file_exists).WillOnce(Return(true));
  EXPECT_CALL(*os_, open_file).WillOnce(Return(data.str()));

  auto result = subject_->find_layout(outputs);
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result.begin()->first, "OUTPUT1");
  EXPECT_EQ(result.rbegin()->first, "OUTPUT2");
}


TEST_F(DisplayConfigTest, MatchesADoubleConfigEvenWithADisconnectedScreen) {
  std::vector<std::shared_ptr<IOutput>> outputs;

  outputs.push_back(output1_);
  outputs.push_back(output2_);
  outputs.push_back(output3_);

  std::stringstream data;
  data << R"(
    - - name: OUTPUT1
        scale: 1
        primary: true
      - name: OUTPUT2
        scale: 2
        primary: false
    - - name: OUTPUT1
        scale: 1
        primary: true
      - name: OUTPUT2
        scale: 2
        primary: false
      - name: OUTPUT3
        scale: 1
        primary: true
  )";

  EXPECT_CALL(*os_, file_exists).WillOnce(Return(true));
  EXPECT_CALL(*os_, open_file).WillOnce(Return(data.str()));

  auto result = subject_->find_layout(outputs);
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result.begin()->first, "OUTPUT1");
  EXPECT_EQ(result.rbegin()->first, "OUTPUT2");
}
