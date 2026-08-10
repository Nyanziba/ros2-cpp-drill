// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/volatile_state.h"

TEST(VolatileStateTest, 初期化と状態確認)
{
  machine_init();
  EXPECT_EQ(machine_get_state(), STATE_IDLE);
  EXPECT_TRUE(machine_is_idle());
  EXPECT_FALSE(machine_is_running());
  EXPECT_FALSE(machine_is_stopped());
}

TEST(VolatileStateTest, IDLE_から_RUNNING_に遷移)
{
  machine_init();
  machine_start();
  EXPECT_EQ(machine_get_state(), STATE_RUNNING);
  EXPECT_FALSE(machine_is_idle());
  EXPECT_TRUE(machine_is_running());
  EXPECT_FALSE(machine_is_stopped());
}

TEST(VolatileStateTest, RUNNING_から_STOPPED_に遷移)
{
  machine_init();
  machine_start();
  machine_stop();
  EXPECT_EQ(machine_get_state(), STATE_STOPPED);
  EXPECT_FALSE(machine_is_idle());
  EXPECT_FALSE(machine_is_running());
  EXPECT_TRUE(machine_is_stopped());
}

TEST(VolatileStateTest, 複数回の状態遷移)
{
  machine_init();

  EXPECT_TRUE(machine_is_idle());
  machine_start();
  EXPECT_TRUE(machine_is_running());
  machine_stop();
  EXPECT_TRUE(machine_is_stopped());

  /* 再び RUNNING に戻す */
  machine_start();
  EXPECT_TRUE(machine_is_running());
}

TEST(VolatileStateTest, 外部から状態が変更されたことを検出できる)
{
  machine_init();
  EXPECT_TRUE(machine_is_idle());

  /* 外部から状態を変更（割り込みハンドラーや別プロセスをシミュレート） */
  machine_simulate_external_change(STATE_RUNNING);

  /* volatile がないと、コンパイラが最適化で「machine_init で STATE_IDLE で止まる」と思い込む。
   * volatile があると、毎回メモリから読むので、変更を正しく検出できる。 */
  EXPECT_FALSE(machine_is_idle());
  EXPECT_TRUE(machine_is_running());
  EXPECT_EQ(machine_get_state(), STATE_RUNNING);
}

TEST(VolatileStateTest, 外部から複数回の変更を検出できる)
{
  machine_init();

  machine_simulate_external_change(STATE_RUNNING);
  EXPECT_TRUE(machine_is_running());

  machine_simulate_external_change(STATE_STOPPED);
  EXPECT_TRUE(machine_is_stopped());

  machine_simulate_external_change(STATE_IDLE);
  EXPECT_TRUE(machine_is_idle());
}

TEST(VolatileStateTest, ポーリングループで状態を監視できる)
{
  /* 典型的なポーリングループのシミュレーション */
  machine_init();

  int state_changes = 0;
  state_t last_state = STATE_IDLE;

  for (int iteration = 0; iteration < 5; iteration++) {
    state_t current = machine_get_state();
    if (current != last_state) {
      state_changes++;
      last_state = current;
    }

    /* 外部から状態を変更 */
    if (iteration == 1) {
      machine_simulate_external_change(STATE_RUNNING);
    } else if (iteration == 3) {
      machine_simulate_external_change(STATE_STOPPED);
    }
  }

  /* 初期状態から RUNNING への変更、RUNNING から STOPPED への変更が検出できること */
  EXPECT_GE(state_changes, 2);
}

TEST(VolatileStateTest, get_state_は毎回読み込みをしている)
{
  /* volatile 変数を読む関数を複数回呼び出して、毎回新しい値を取得できることを確認 */
  machine_init();
  EXPECT_EQ(machine_get_state(), STATE_IDLE);

  machine_simulate_external_change(STATE_RUNNING);
  EXPECT_EQ(machine_get_state(), STATE_RUNNING);

  machine_simulate_external_change(STATE_STOPPED);
  EXPECT_EQ(machine_get_state(), STATE_STOPPED);

  /* 同じ関数を複数回呼び出した時、毎回異なる値が返ることが保証される */
  machine_simulate_external_change(STATE_IDLE);
  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(machine_get_state(), STATE_IDLE);
  }
}
