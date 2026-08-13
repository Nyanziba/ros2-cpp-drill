// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "drill/fault_chain.hpp"

namespace
{

/// 電圧低下 → 過電流 → 通信断 の順につないだ標準の連鎖。
std::unique_ptr<FaultHandler> make_standard_chain(std::vector<std::string> * log = nullptr)
{
  auto head = make_low_voltage_handler("low_voltage", 11000, log);
  head->set_next(make_over_current_handler("over_current", 20000, log))
    .set_next(make_comm_timeout_handler("comm_timeout", 500, log));
  return head;
}

}  // namespace

TEST(ChainOfResponsibilityTest, 適切なハンドラが処理する)
{
  const auto chain = make_standard_chain();

  const auto low = chain->support(Fault{FaultKind::kLowVoltage, 10500});
  ASSERT_TRUE(low.has_value()) << "電圧低下を誰も処理していません";
  EXPECT_EQ(low->handler_name, "low_voltage");
  EXPECT_EQ(low->action, "reduce_duty");

  const auto over = chain->support(Fault{FaultKind::kOverCurrent, 25000});
  ASSERT_TRUE(over.has_value()) << "先頭が処理できないとき次に回せていません";
  EXPECT_EQ(over->handler_name, "over_current");
  EXPECT_EQ(over->action, "cut_output");

  const auto comm = chain->support(Fault{FaultKind::kCommTimeout, 800});
  ASSERT_TRUE(comm.has_value()) << "連鎖の末尾まで届いていません";
  EXPECT_EQ(comm->handler_name, "comm_timeout");
  EXPECT_EQ(comm->action, "safe_stop");
}

TEST(ChainOfResponsibilityTest, 条件を満たさない異常は素通しされる)
{
  const auto chain = make_standard_chain();

  // 種類は合っているが閾値を満たさない。担当ハンドラも「処理しない」を選ぶ。
  EXPECT_FALSE(chain->support(Fault{FaultKind::kLowVoltage, 12000}).has_value())
    << "閾値を満たさないのに electric 系ハンドラが処理してしまっています";
  EXPECT_FALSE(chain->support(Fault{FaultKind::kOverCurrent, 5000}).has_value());
  EXPECT_FALSE(chain->support(Fault{FaultKind::kCommTimeout, 100}).has_value());

  // 境界のすぐ内側は処理されること（条件が厳しすぎないことの確認）。
  EXPECT_TRUE(chain->support(Fault{FaultKind::kLowVoltage, 10999}).has_value());
  EXPECT_TRUE(chain->support(Fault{FaultKind::kOverCurrent, 20000}).has_value());
  EXPECT_TRUE(chain->support(Fault{FaultKind::kCommTimeout, 500}).has_value());
}

TEST(ChainOfResponsibilityTest, 誰も処理しなければnulloptが返る)
{
  const auto chain = make_standard_chain();

  // 同じ連鎖が、担当のいる異常はちゃんと処理すること。
  ASSERT_TRUE(chain->support(Fault{FaultKind::kCommTimeout, 800}).has_value());

  const auto result = chain->support(Fault{FaultKind::kEncoderSlip, 9999});
  EXPECT_FALSE(result.has_value())
    << "担当がいない異常は std::nullopt で返す仕様です（例外は投げません）";
}

TEST(ChainOfResponsibilityTest, 連鎖の順番を変えると処理するハンドラが変わる)
{
  // 20A で切るハンドラと 30A で切るハンドラ。35A はどちらでも処理できる。
  auto early_first = make_over_current_handler("cut_20A", 20000);
  early_first->set_next(make_over_current_handler("cut_30A", 30000));

  auto late_first = make_over_current_handler("cut_30A", 30000);
  late_first->set_next(make_over_current_handler("cut_20A", 20000));

  const Fault fault{FaultKind::kOverCurrent, 35000};

  const auto a = early_first->support(fault);
  const auto b = late_first->support(fault);
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value());

  EXPECT_EQ(a->handler_name, "cut_20A");
  EXPECT_EQ(b->handler_name, "cut_30A")
    << "先頭のハンドラが処理できるのに次に回してしまっています";
}

TEST(ChainOfResponsibilityTest, setNextは次のハンドラ自身への参照を返す)
{
  auto head = make_low_voltage_handler("head", 11000);
  auto second = make_over_current_handler("second", 20000);
  const FaultHandler * const second_address = second.get();

  FaultHandler & returned = head->set_next(std::move(second));

  EXPECT_EQ(&returned, second_address)
    << "set_next() は *this ではなく、つないだ次のハンドラへの参照を返します";
}

TEST(ChainOfResponsibilityTest, 先頭を破棄すると連鎖全体が破棄される)
{
  std::vector<std::string> destruction_log;
  {
    const auto chain = make_standard_chain(&destruction_log);
    EXPECT_TRUE(destruction_log.empty()) << "まだ誰も壊れていないはずです";
  }

  const std::vector<std::string> expected = {"low_voltage", "over_current", "comm_timeout"};
  EXPECT_EQ(destruction_log, expected)
    << "先頭から順に、連鎖全体が破棄されるはずです。next_ を unique_ptr で所有していますか";
}

TEST(ChainOfResponsibilityTest, 連鎖の途中を差し替えると古い残りは破棄される)
{
  std::vector<std::string> destruction_log;
  auto head = make_low_voltage_handler("head", 11000, &destruction_log);
  head->set_next(make_over_current_handler("old_tail", 20000, &destruction_log));

  // set_next をもう一度呼ぶと、それまでの next_ の所有権が捨てられる。
  head->set_next(make_comm_timeout_handler("new_tail", 500, &destruction_log));

  const std::vector<std::string> after_replace = {"old_tail"};
  EXPECT_EQ(destruction_log, after_replace)
    << "差し替えられた古い next_ が解放されていません（生ポインタで持っていませんか）";

  const auto result = head->support(Fault{FaultKind::kCommTimeout, 600});
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->handler_name, "new_tail");
}

TEST(ChainOfResponsibilityTest, supportAloneは次に回さない)
{
  const auto chain = make_standard_chain();

  // 先頭は電圧低下しか見ない。単体で聞けば過電流は処理されない。
  EXPECT_FALSE(chain->support_alone(Fault{FaultKind::kOverCurrent, 25000}).has_value())
    << "support_alone() が次に回してしまっています";
  EXPECT_TRUE(chain->support_alone(Fault{FaultKind::kLowVoltage, 10000}).has_value());
}

TEST(ChainOfResponsibilityTest, 配列方式でも同じ結果になる)
{
  const auto low = make_low_voltage_handler("low_voltage", 11000);
  const auto over = make_over_current_handler("over_current", 20000);
  const auto comm = make_comm_timeout_handler("comm_timeout", 500);

  FaultHandler * const handlers[] = {low.get(), over.get(), comm.get()};
  const std::size_t count = sizeof(handlers) / sizeof(handlers[0]);

  const auto over_result = dispatch(handlers, count, Fault{FaultKind::kOverCurrent, 25000});
  ASSERT_TRUE(over_result.has_value()) << "dispatch() が配列を回れていません";
  EXPECT_EQ(over_result->handler_name, "over_current");

  const auto comm_result = dispatch(handlers, count, Fault{FaultKind::kCommTimeout, 800});
  ASSERT_TRUE(comm_result.has_value());
  EXPECT_EQ(comm_result->handler_name, "comm_timeout");

  EXPECT_FALSE(dispatch(handlers, count, Fault{FaultKind::kEncoderSlip, 1}).has_value());
  EXPECT_FALSE(dispatch(handlers, 0, Fault{FaultKind::kOverCurrent, 25000}).has_value())
    << "count が 0 のときは std::nullopt です";

  // 連鎖版と配列版で答えが一致すること。
  const auto chain = make_standard_chain();
  const Fault fault{FaultKind::kOverCurrent, 25000};
  const auto from_chain = chain->support(fault);
  const auto from_array = dispatch(handlers, count, fault);
  ASSERT_TRUE(from_chain.has_value());
  ASSERT_TRUE(from_array.has_value());
  EXPECT_TRUE(*from_chain == *from_array);
}

TEST(ChainOfResponsibilityTest, ハンドラはコピーできない)
{
  static_assert(
    !std::is_copy_constructible<FaultHandler>::value,
    "連鎖のノードはコピーできてはいけません");
  static_assert(
    std::has_virtual_destructor<FaultHandler>::value,
    "基底クラスには仮想デストラクタが必要です");

  // 連鎖が 1 段だけでも動くこと（next_ が nullptr のときの経路）。
  const auto lone = make_over_current_handler("lone", 20000);
  const auto result = lone->support(Fault{FaultKind::kOverCurrent, 21000});
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->handler_name, "lone");
}
