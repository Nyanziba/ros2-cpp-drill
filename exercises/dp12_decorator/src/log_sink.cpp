// I AM NOT DONE
//
// 結城本 第12章 Decorator を C++ で書きます。
// 中身を std::unique_ptr で所有し、ムーブで入れ子を組み立てます。
// 各デストラクタが DestructionLog に名前を書き込むので、
// 「外側を捨てると内側まで解放される」ことがテストで見えます。

#include "drill/log_sink.hpp"

std::string join_tag(const std::string & tag, const std::string & body)
{
  // TODO: tag と body を半角スペース 1 つでつないで返してください。
  //       join_tag("[INFO]", "moving") == "[INFO] moving"
  //
  // この関数は unique_ptr 版とテンプレート版の両方から呼ばれます。
  // ここを 1 回書けば、2 つの実装の出力が必ず一致します。
  return tag + body;
}

PlainMessage::~PlainMessage()
{
  // TODO: DestructionLog::record("PlainMessage") を呼んでください。
}

std::string PlainMessage::format(const std::string & message) const
{
  // TODO: 何も足さずに message をそのまま返してください。
  (void)message;
  return std::string{};
}

SinkDecorator::SinkDecorator(std::unique_ptr<LogSink> inner)
: inner_(nullptr)
{
  // TODO: 引数 inner をメンバ inner_ に **std::move で** 移してください。
  //       初期化子リストで書くのが本来の形です（inner_(std::move(inner))）。
  //       コピーしようとするとコンパイルエラーになります。unique_ptr はコピーできません。
  (void)inner;
}

SinkDecorator::~SinkDecorator() = default;

const LogSink & SinkDecorator::inner() const
{
  // TODO: 所有している中身への参照を返してください。
  //       ここでコピーは作れません（LogSink は抽象クラス）。参照で返します。
  return *inner_;
}

LevelTag::LevelTag(std::unique_ptr<LogSink> inner, std::string level)
: SinkDecorator(std::move(inner)), level_(std::move(level))
{
}

LevelTag::~LevelTag()
{
  // TODO: DestructionLog::record("LevelTag") を呼んでください。
}

std::string LevelTag::format(const std::string & message) const
{
  // TODO: 中身に整形させた結果の前に "[レベル]" を付けて返してください。
  //       join_tag("[" + level_ + "]", inner().format(message)) の形です。
  //
  // 「中身に先に仕事をさせて、その結果を包む」— これが Decorator の中心です。
  return level_ + message;
}

TimestampTag::TimestampTag(std::unique_ptr<LogSink> inner, std::string stamp)
: SinkDecorator(std::move(inner)), stamp_(std::move(stamp))
{
}

TimestampTag::~TimestampTag()
{
  // TODO: DestructionLog::record("TimestampTag") を呼んでください。
}

std::string TimestampTag::format(const std::string & message) const
{
  // TODO: 中身に整形させた結果の前に stamp_ を付けて返してください。
  return stamp_ + message;
}

SourceTag::SourceTag(std::unique_ptr<LogSink> inner, std::string file, int line)
: SinkDecorator(std::move(inner)), file_(std::move(file)), line_(line)
{
}

SourceTag::~SourceTag()
{
  // TODO: DestructionLog::record("SourceTag") を呼んでください。
}

std::string SourceTag::format(const std::string & message) const
{
  // TODO: 中身に整形させた結果の前に "ファイル名:行番号" を付けて返してください。
  //       タグの文字列は file_ + ":" + std::to_string(line_) です。
  return file_ + std::to_string(line_) + message;
}

std::unique_ptr<LogSink> plain()
{
  // TODO: PlainMessage を作って返してください。
  return nullptr;
}

std::unique_ptr<LogSink> with_level(std::unique_ptr<LogSink> inner, std::string level)
{
  // TODO: inner を LevelTag で包んで返してください。
  //       inner はここでも std::move で渡します。
  (void)inner;
  (void)level;
  return nullptr;
}

std::unique_ptr<LogSink> with_timestamp(std::unique_ptr<LogSink> inner, std::string stamp)
{
  // TODO: inner を TimestampTag で包んで返してください。
  (void)inner;
  (void)stamp;
  return nullptr;
}

std::unique_ptr<LogSink> with_source(std::unique_ptr<LogSink> inner, std::string file, int line)
{
  // TODO: inner を SourceTag で包んで返してください。
  (void)inner;
  (void)file;
  (void)line;
  return nullptr;
}
