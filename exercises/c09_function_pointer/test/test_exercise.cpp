// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/function_pointer.h"

static int g_called_led_id = -1;
static int g_called_state = -1;
static int g_call_count = 0;

static void handler_on_off(int led_id, int state)
{
  g_called_led_id = led_id;
  g_called_state = state;
  g_call_count++;
}

static void handler_toggle(int led_id, int state)
{
  /* 別のハンドラー */
  g_called_led_id = -led_id - 1;  /* 呼び出し元と区別するため負値にする */
  g_called_state = state;
  g_call_count++;
}

TEST(FunctionPointerTest, コントローラー作成と破棄)
{
  struct LedController * ctrl = led_controller_create();
  ASSERT_NE(ctrl, nullptr);

  /* 作成直後はすべてのハンドラーが NULL */
  for (int i = 0; i < LED_COUNT; i++) {
    EXPECT_TRUE(led_handler_is_null(ctrl, i));
  }

  led_controller_destroy(ctrl);
}

TEST(FunctionPointerTest, ハンドラーを登録できる)
{
  struct LedController * ctrl = led_controller_create();
  ASSERT_NE(ctrl, nullptr);

  led_register_handler(ctrl, 0, handler_on_off);
  EXPECT_FALSE(led_handler_is_null(ctrl, 0));
  EXPECT_TRUE(led_handler_is_null(ctrl, 1));

  led_register_handler(ctrl, 2, handler_toggle);
  EXPECT_FALSE(led_handler_is_null(ctrl, 2));

  led_controller_destroy(ctrl);
}

TEST(FunctionPointerTest, 登録されたハンドラーが呼ばれる)
{
  struct LedController * ctrl = led_controller_create();
  ASSERT_NE(ctrl, nullptr);

  led_register_handler(ctrl, 0, handler_on_off);

  g_call_count = 0;
  g_called_led_id = -1;
  g_called_state = -1;

  led_set(ctrl, 0, 1);
  EXPECT_EQ(g_call_count, 1);
  EXPECT_EQ(g_called_led_id, 0);
  EXPECT_EQ(g_called_state, 1);

  led_set(ctrl, 0, 0);
  EXPECT_EQ(g_call_count, 2);
  EXPECT_EQ(g_called_led_id, 0);
  EXPECT_EQ(g_called_state, 0);

  led_controller_destroy(ctrl);
}

TEST(FunctionPointerTest, 未登録のスロットを呼んでも落ちない)
{
  struct LedController * ctrl = led_controller_create();
  ASSERT_NE(ctrl, nullptr);

  /* ハンドラーを登録していない状態 */
  g_call_count = 0;

  /* 未登録のスロットへの led_set は何もしない（安全） */
  led_set(ctrl, 0, 1);
  EXPECT_EQ(g_call_count, 0);  /* ハンドラーが呼ばれていない */

  led_set(ctrl, 3, 0);
  EXPECT_EQ(g_call_count, 0);  /* ハンドラーが呼ばれていない */

  led_controller_destroy(ctrl);
}

TEST(FunctionPointerTest, ハンドラーを複数登録して正しく呼び分ける)
{
  struct LedController * ctrl = led_controller_create();
  ASSERT_NE(ctrl, nullptr);

  led_register_handler(ctrl, 0, handler_on_off);
  led_register_handler(ctrl, 1, handler_toggle);

  g_call_count = 0;

  led_set(ctrl, 0, 1);
  EXPECT_EQ(g_called_led_id, 0);
  EXPECT_EQ(g_called_state, 1);
  EXPECT_EQ(g_call_count, 1);

  led_set(ctrl, 1, 1);
  EXPECT_EQ(g_called_led_id, -2);  /* handler_toggle が負値にする */
  EXPECT_EQ(g_called_state, 1);
  EXPECT_EQ(g_call_count, 2);

  led_controller_destroy(ctrl);
}

TEST(FunctionPointerTest, ハンドラーをNULLで削除できる)
{
  struct LedController * ctrl = led_controller_create();
  ASSERT_NE(ctrl, nullptr);

  led_register_handler(ctrl, 0, handler_on_off);
  EXPECT_FALSE(led_handler_is_null(ctrl, 0));

  g_call_count = 0;
  led_set(ctrl, 0, 1);
  EXPECT_EQ(g_call_count, 1);

  /* NULL を登録して削除 */
  led_register_handler(ctrl, 0, NULL);
  EXPECT_TRUE(led_handler_is_null(ctrl, 0));

  g_call_count = 0;
  led_set(ctrl, 0, 1);
  EXPECT_EQ(g_call_count, 0);  /* 呼ばれていない */

  led_controller_destroy(ctrl);
}

TEST(FunctionPointerTest, ハンドラーを上書きできる)
{
  struct LedController * ctrl = led_controller_create();
  ASSERT_NE(ctrl, nullptr);

  led_register_handler(ctrl, 0, handler_on_off);
  g_call_count = 0;
  led_set(ctrl, 0, 1);
  EXPECT_EQ(g_called_led_id, 0);
  EXPECT_EQ(g_call_count, 1);

  /* ハンドラーを上書き */
  led_register_handler(ctrl, 0, handler_toggle);
  g_call_count = 0;
  led_set(ctrl, 0, 1);
  EXPECT_EQ(g_called_led_id, -1);  /* handler_toggle が呼ばれた */
  EXPECT_EQ(g_call_count, 1);

  led_controller_destroy(ctrl);
}
