# 23. Interpreter

> **Matches chapter 23 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep `Node` / `ProgramNode` / `CommandListNode` / `RepeatCommandNode`, and
> `Context` (the one with `nextToken()`), open next to you.
>
> **Goal of this chapter**: Interpreter is **the pattern with the fewest uses among the 23.**
> First, we start with "the decision not to use it". Then we cover the 3 points that really matter when you write it in C++:
> **ownership of the syntax tree**, **separating the parser from the AST**, and **returning syntax errors without exceptions.**
> Finally, we put it next to the `std::variant` version and connect to the Visitor discussion in chapter 13.

## 23.1 First, the decision not to use it

Chapter 23 of the book is an interesting chapter, but **there are almost no cases where you write your own parser in a team library.**
Let us get rid of that first.

Before you use Interpreter, ask these 3 questions in order.

| Question | If Yes |
| --- | --- |
| Does that "language" only write settings? | **Use a YAML / JSON parser.** Do not write your own |
| Are the kinds finite and fixed at compile time? | An `enum` and an array are enough. You do not need parsing |
| Do you really need to build behavior **from a string** at run time? | If No, just call the function directly |

Concretely, it goes like this.

- **Robot parameters** (PID gains, limit values) → YAML. In ROS 2, you have
  `declare_parameter`. There is no reason to make a mini language
- **Motion sequences** (forward → turn → stop) → First think about whether you can write a `std::vector<Motion>`
  directly in code. If you can, you do not need a language
- **You want to throw motions from a PC during a match** → Only here do you get to "interpret text".
  Even then, first think about whether a binary command sequence (`uint8_t opcode; int16_t arg;`) is enough.
  Rather than parsing text on the microcontroller, **analyzing on the PC side and sending binary** is almost always right

If someone asks "then what is the point of learning it?", there is one answer.

> **To learn the idea of evaluating a tree structure recursively.**

Syntax tree evaluation appears outside parsers too. Behavior trees, hierarchical state machines,
UI layout calculation, coordinate composition in a scene graph. All of them have the same shape:
"a node evaluates itself and asks its children to do the same."
**You write an Interpreter to write that shape by hand once.**
The part that reads strings (the parser) is a bonus.

Of the 4 checks in [0. Before you use them](00_before_you_use_them.md), the one that applies to this chapter is number 4
(is there the same thing in the standard library?). **For a configuration file, it already exists.**

## 23.2 Porting the Java version to C++ as is

This is `Node` in the book.

```java
public abstract class Node {
    public abstract void parse(Context context) throws ParseException;
}
```

If you port it to C++ in a straightforward way, you get this. ...But **it does not work in C++ unless you change 3 places.**

```cpp
class Node
{
public:
  virtual ~Node() = default;                              // change 1
  virtual void evaluate(std::vector<Motion> & out) const = 0;  // change 2, 3
};
```

### Change 1: Virtual destructor

The tree in this chapter holds its children with `std::unique_ptr<Node>`. **They are destroyed through a base pointer.**
Without a virtual destructor, the whole subtree that `RepeatNode` held leaks.
It is the same story from chapter 1 to chapter 23. It was the same to the end.

### Change 2: `parse()` was removed from the node

The book's `Node` has `parse()`. **In C++, we do not do this.**
I explain the reason in 23.4. For now, just this: "only `evaluate()` stays in `Node`."

### Change 3: Receive an output destination instead of a return value

If you write "return the evaluation result", you get this.

```cpp
virtual std::vector<Motion> evaluate() const = 0;   // straightforward but slow
```

If `RepeatNode` runs 3 times, 3 `vector`s of the children are created, and one more `vector` that joins them is created.
**Allocations and copies pile up by the depth of the tree.** The same happens in Java, but
Java only returns a reference to an `ArrayList`, so it is not noticeable. C++ returns a value, so it shows clearly.

```cpp
virtual void evaluate(std::vector<Motion> & out) const = 0;   // allocate once
```

You pass the output destination as a non-const reference. The caller creates one `vector`, and the whole tree pushes into it.
**Also watch where you put `const`.** The node itself does not change, so `evaluate` is `const`,
and `out` is written to, so it does not get `const`. Java has no such distinction.

## 23.3 Who owns it — the tree itself

In the Java version, you `new ProgramNode()`, references are linked, and the GC collects it.
In C++, you write ownership in the type. **The answer is exactly the same as Composite in chapter 11.**

```cpp
class SequenceNode final : public Node
{
public:
  void append(std::unique_ptr<Node> child) { children_.push_back(std::move(child)); }
  void evaluate(std::vector<Motion> & out) const override;

private:
  std::vector<std::unique_ptr<Node>> children_;   // the parent solely owns the children
};

class RepeatNode final : public Node
{
public:
  RepeatNode(int count, std::unique_ptr<Node> body)
  : count_(count), body_(std::move(body)) {}

private:
  int count_;
  std::unique_ptr<Node> body_;
};
```

**A syntax tree is the tree with the simplest ownership.** The parent holds each child alone, and nothing is shared.
You do not need `shared_ptr`. If you drop one root, everything disappears.

```cpp
{
  ParseResult result = parse(source);
  // ...
}   // the whole tree is freed
```

**`final`** is here to say in the type that no more classes derive from it.
When you add it, the compiler can sometimes skip the virtual call of the destructor. It is free.

> **Note**: When you drop a deep tree, **the destructor recurses too.**
> `~SequenceNode` → `~unique_ptr` → the child's `~Node` → ... and it dives down.
> The depth limit in 23.6 actually protects not only evaluation but also destruction.

## 23.4 Separate the parser from the AST

The book gives `Node` a `Node::parse(Context)`. **This is a slightly old way even in Java**, and
in C++ it is more natural to separate them. There are 2 reasons.

**Reason 1: There are 2 responsibilities.** `Node` knows both "how to read a string" and "how to evaluate".
If you change the grammar of the language, all the files that have the evaluation code change. Conversely,
when you want to change only the evaluation (to emit a log string instead of a motion sequence), you have to read the parser.

**Reason 2: You cannot write tests.** If they are separate, you can build a tree directly without the parser and test only the evaluation.

```cpp
auto body = std::make_unique<SequenceNode>();
body->append(std::make_unique<CommandNode>(Motion{MotionKind::Forward, 50}));
RepeatNode repeat{3, std::move(body)};

std::vector<Motion> out;
repeat.evaluate(out);          // you can check only the evaluation, without the parser at all
```

Conversely, you can also test only the parser by "is the shape of the tree correct?"
**The fact that the exercise tests can compare results with the `std::variant` version is also thanks to this separation.**

After separating, you get this shape.

```
string ──tokenize──> token list ──parse──> AST ──evaluate──> motion list
```

Each stage is independent, and you can test each one alone.
What corresponds to `Context` (the current position) is held **by the parser**, not by `Node`.

## 23.5 Return syntax errors without exceptions

The Java version has `throws ParseException`. If you `throw` as is in C++,
**it does not work on a microcontroller** (`-fno-exceptions`). Return it as a value.

```cpp
struct ParseError
{
  std::string message;
  std::size_t position = 0;   // at which byte it noticed. Without this it is not usable
};

class ParseResult
{
public:
  static ParseResult success(std::unique_ptr<Node> ast);
  static ParseResult failure(ParseError error);

  bool ok() const { return ast_ != nullptr; }
  const Node * ast() const { return ast_.get(); }
  const ParseError & error() const { return error_; }

private:
  std::unique_ptr<Node> ast_;
  ParseError error_;
};
```

You can also write it with `std::optional<std::unique_ptr<Node>>`, but **the reason for the failure disappears.**
A syntax error cannot be fixed unless it tells you "where and what". `std::optional` is for
"when there is only one reason for having no value". Here, make your own type.

> In C++23, `std::expected<std::unique_ptr<Node>, ParseError>` is exactly this.
> Within C++17, you write it yourself. **If you keep the form close to `expected`**,
> you can replace it later.

### How to pass the failure upward

A recursive descent parser has functions that call each other in a nested way. If you do not use `throw`, you need a way to pass it up.
The standard way is this.

```cpp
class Parser
{
private:
  std::unique_ptr<Node> parse_statement(std::size_t depth);   // nullptr on failure
  std::nullptr_t fail(std::string message, std::size_t position)
  {
    error_ = ParseError{std::move(message), position};
    return nullptr;                                            // you can return it as is
  }

  ParseError error_;
};
```

**"The return value is only success or failure, and the details go in a member."** The idea is the same as `errno`,
but it is a member of the parser, not a global, so thread problems do not happen.
Each caller just writes `if (child == nullptr) { return nullptr; }`.

**Build the error message on the spot.** If you try to build it after returning upward,
you no longer know which token failed.

## 23.6 A recursive descent parser eats the stack

This is not so much about C++ as about something that is **fatal on a microcontroller.**

A recursive descent parser pushes one stack frame for each nesting level.
If someone writes `repeat 1 { repeat 1 { ... } }` 200,000 levels deep, **it crashes even if the input is correct.**

In 23.9, we actually crash it. Here is only the remedy.

```cpp
inline constexpr std::size_t kMaxNestingDepth = 16;

std::unique_ptr<Node> Parser::parse_sequence(std::size_t depth)
{
  if (depth > kMaxNestingDepth) {                 // ★ check at the "entrance" of the recursion
    return fail("repeat is nested too deeply", peek().position);
  }
  // ...
  parse_sequence(depth + 1);
}
```

**Check at the entrance.** If you check at the exit or in cleanup, the stack runs out before you get there.
And **pass the depth as an argument.** If you make it a member counter,
you forget to subtract it on every early return and it drifts.

Cut the upper limit at "a depth that cannot happen in practice". A motion description will not come with 16 levels of nesting.
**Having a limit that is too low hurts much less than having no limit and crashing.**

For the same reason, put a limit on the `repeat` count too (in the exercise, `kMaxRepeatCount = 1000`).
`repeat 1000000 { forward 1; }` is correct as syntax, and when expanded it has 1 million elements.
**Check the validity of input not only by syntax but also by size.**

## 23.7 Write it with `std::variant` — the sequel to Visitor in chapter 13

If the kinds of nodes are **fixed** (in this exercise, 3: forward / turn / repeat),
you need neither inheritance nor a vtable. What we did in chapter 13 applies as it is.

```cpp
struct Repeat;

struct Command
{
  Motion motion;
};

using VNode = std::variant<Command, std::unique_ptr<Repeat>>;

struct Repeat
{
  int count = 0;
  std::vector<VNode> body;
};
```

**Notice that `unique_ptr` has not disappeared.**
You tend to think "the heap disappears if I use variant", but **for a recursive type, it does not disappear.**
`Repeat` may contain itself, so you need indirection somewhere.
Also, the elements of `std::variant` must be complete types, so
you cannot write `std::variant<Command, Repeat>` (you need the size of `Repeat` while defining `Repeat`).
If you put in a `unique_ptr`, `Repeat` can stay an incomplete type and it compiles.

You write the evaluation like this.

```cpp
struct VariantEvaluator
{
  std::vector<Motion> * out;

  void operator()(const Command & command) const { out->push_back(command.motion); }

  void operator()(const std::unique_ptr<Repeat> & repeat) const
  {
    for (int i = 0; i < repeat->count; ++i) {
      for (const VNode & child : repeat->body) {
        std::visit(*this, child);      // pass itself and recurse
      }
    }
  }
};
```

Comparison with the inheritance version.

| | Inheritance + `unique_ptr<Node>` | `std::variant` |
| --- | --- | --- |
| Adding a kind | Just add one node. Existing code is untouched | Fix the `variant` and **all visitors** |
| Adding an operation (formatting besides evaluation) | Add a virtual function to all nodes | Just add one visitor |
| Exhaustiveness check | None (a forgotten implementation shows up at run time) | **It becomes a compile error** |
| Heap | Always needed | Still needed for a recursive type |
| vtable | Yes | No (branching by index instead) |

**If kinds grow, use inheritance. If operations grow, use variant.** This was the conclusion of chapter 13.
The syntax of a mini language does not usually grow so often, so **Interpreter suits variant.**
In the exercise, you write both and check that they give the same result.

## 23.8 Is there the same thing in the standard library / language?

In the sense of "interpreting a string", **yes. If you choose the use case, you do not need to write your own.**

| What you want to do | What to use |
| --- | --- |
| Read a number | `std::stoi` / `std::from_chars` (no exceptions, no allocation) |
| Read whitespace-separated values | `std::istringstream` |
| Regular expressions | `std::regex` (**but it is slow, and it allocates and throws exceptions**. Do not use it on microcontrollers) |
| Configuration files | A YAML / JSON library. Do not write your own |
| Command line | Various argument parsers |

For "evaluating a syntax tree", **the standard has nothing.**
`std::visit` is a tool for branching, not a tree. This is the part you write yourself.

You can also combine `std::function` and use it instead of a tree
(the same idea as chapter 22 Command: stack closures instead of nodes).
**For a simple language, this is shorter.** But you step on the heap.

## 23.9 Try it yourself

You check "does a recursive descent parser that does not look at the depth really crash?" by actually crashing it.
**Before you run it, predict what happens.**

```cpp
// try.cpp
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Node
{
public:
  virtual ~Node() = default;
  virtual void evaluate(std::vector<int> & out) const = 0;
};

class Leaf final : public Node
{
public:
  explicit Leaf(int value) : value_(value) {}
  void evaluate(std::vector<int> & out) const override { out.push_back(value_); }

private:
  int value_;
};

class Repeat final : public Node
{
public:
  Repeat(int count, std::unique_ptr<Node> body)
  : count_(count), body_(std::move(body)) {}

  void evaluate(std::vector<int> & out) const override
  {
    for (int i = 0; i < count_; ++i) {
      body_->evaluate(out);
    }
  }

private:
  int count_;
  std::unique_ptr<Node> body_;
};

// A recursive descent parser that never looks at the depth. It recurses when "(" comes.
std::unique_ptr<Node> parse(const std::string & source, std::size_t & index)
{
  if (index < source.size() && source[index] == '(') {
    ++index;
    std::unique_ptr<Node> body = parse(source, index);
    if (index < source.size() && source[index] == ')') {
      ++index;
    }
    return std::make_unique<Repeat>(2, std::move(body));
  }
  return std::make_unique<Leaf>(1);
}

std::string nested(std::size_t depth)
{
  return std::string(depth, '(') + std::string(depth, ')');
}

int main()
{
  std::size_t index = 0;
  const std::string small = nested(3);
  const std::unique_ptr<Node> tree = parse(small, index);

  std::vector<int> out;
  tree->evaluate(out);
  std::cout << "depth 3 -> " << out.size() << " leaves\n";

  std::cout << "parsing depth 200000 ..." << std::endl;
  std::size_t deep_index = 0;
  const std::string deep = nested(200000);
  const std::unique_ptr<Node> deep_tree = parse(deep, deep_index);
  std::cout << "parsed. (you are lucky if you get here)" << std::endl;
  return deep_tree == nullptr ? 1 : 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: what does the first line print? And what happens at 200000 levels?</summary>

This is the actual output on my machine (macOS / Apple clang).

<!-- measure: env=clang filter="head -n 2" -->
```
depth 3 -> 8 leaves
parsing depth 200000 ...
```

And the exit code is **139** (`128 + 11`, that is, SIGSEGV).
`parsed.` is not printed. **With an input that is completely correct as syntax, the parser overran the stack.**

The first line is 8 because of `2^3`. `Repeat{2}` is nested 3 levels, and there is one leaf.
You can also see that **the length of the input is proportional to the depth, but the expanded result grows exponentially.**
This is why `repeat` needs a limit on both the count and the depth.

Even if `parse` had survived, next the **destructor of `deep_tree` would recurse** and crash in the same place.
A tree is recursive both to build and to destroy.

</details>

## 23.10 Conclusion for microcontrollers

**Do not parse text at run time.** This is the conclusion. The reasons are 23.6, allocation, and exceptions.

- Recursive descent = eats the stack. **In a world with tens of KB of RAM, stack usage that depends on the input is out of the question**
- AST = a tree of `unique_ptr` = heap. Allocations happen in the loop
- An error message as `std::string` = more allocation
- You cannot `throw`

Settings are **decided at compile time** or **held in a binary table.**

```cpp
// Hold the motion sequence in a constexpr array. Zero parsing.
struct Step
{
  MotionKind kind;
  int16_t value;
};

constexpr Step kApproachSequence[] = {
  {MotionKind::Forward, 100},
  {MotionKind::Turn, 90},
  {MotionKind::Forward, 50},
};
// It is placed in ROM. It uses no RAM and no allocation
```

If you want to throw motions from the PC side, use **a binary command sequence, not text.**

```cpp
// Fixed length, no recursion. Just look at the opcode and run one instruction at a time
struct Instruction
{
  uint8_t opcode;   // 0: forward, 1: turn, 2: repeat_begin, 3: repeat_end
  int16_t arg;
};

// repeat is handled with a "stack", not a "tree". The depth limit is decided by the array size
class Machine
{
public:
  bool run(const Instruction * program, std::size_t count)
  {
    std::size_t loop_depth = 0;
    for (std::size_t pc = 0; pc < count; ++pc) {
      switch (program[pc].opcode) {
        case 2:
          if (loop_depth >= kMaxLoopDepth) {
            return false;                       // too deep. Do not throw an exception
          }
          loop_stack_[loop_depth].start = pc;
          loop_stack_[loop_depth].remaining = program[pc].arg;
          ++loop_depth;
          break;
        case 3:
          // decrease the remaining count and go back to start (omitted)
          break;
        default:
          execute(program[pc]);
          break;
      }
    }
    return true;
  }

private:
  static constexpr std::size_t kMaxLoopDepth = 4;
  struct Loop { std::size_t start; int remaining; };
  Loop loop_stack_[kMaxLoopDepth] = {};

  void execute(const Instruction & instruction);
};
```

There are 3 points.

1. **Do not build a tree.** You only scan the instruction sequence from the front. Zero heap
2. **Do not recurse.** Express nesting with a loop stack (a fixed-length array). **The depth limit is written in the type**
3. **Errors are `bool` / error codes.** Use neither strings nor exceptions

If you need to analyze text, **do it on the PC side and send this `Instruction` array.**
Do not put a parser on the microcontroller.

## 23.11 Conclusion for ROS 2 (supplement)

On the ROS 2 side, you can freely write run-time parsing. But **look for existing tools before that.**

- Parameters → `declare_parameter` + YAML. This is not a reason to make a mini language
- Describing behavior → BehaviorTree.CPP (used by Nav2). You write a tree in XML and
  register nodes in C++. **It is exactly an Interpreter, but it already exists**
- Message definitions → The IDL of `.msg` / `.srv` and its compiler already exist

Before you write your own parser, always ask "can't I do this with BehaviorTree.CPP?"
This is a matter of adding a new library, so consult a person before you decide.

## 23.12 Common pitfalls

| Symptom | Cause |
| --- | --- |
| SIGSEGV (exit code 139) with deep nesting | You do not check the depth at the entrance of the recursion. 23.6 |
| It crashes when you drop the tree | The destructor recurses too. The depth limit also protects destruction |
| The expanded result becomes huge and eats memory | There is no limit on the `repeat` count. Validate input by size too |
| You cannot fix it because you do not know the error position | `ParseError` has no `position`. A message alone is not enough |
| The body of `repeat` eats statements outside it | `parse_sequence` does not stop at both `}` and the end of input |
| You added a node and forgot to write its evaluation | The inheritance version cannot detect it. With `variant` + `visit`, it is a **compile error** |
| `std::variant<Command, Repeat>` does not compile | It is recursive and an incomplete type. Put in a `unique_ptr` |
| `std::visit` gives dozens of lines of template errors | The visitor lacks an `operator()` for one of the candidate types |
| Evaluation is slow / allocates a lot | `evaluate()` returns a `std::vector`. Pass the output destination |
| The tree leaks | `Node` has no virtual destructor |

## 23.13 Matching exercise

```bash
./drill run dp23
```

In `exercises/dp23_interpreter/src/motion_script.cpp`, you implement the interpretation of the motion description mini language
`forward 100; turn 90; repeat 3 { forward 50; }`.

1. `evaluate()` of `CommandNode` / `SequenceNode` / `RepeatNode`
2. `parse()` — a recursive descent parser. **It does not throw exceptions** and returns a `ParseResult`. **With a depth limit**
3. `run_variant()` — the evaluation with `std::variant` + `std::visit`

The tests check the expanded result of nesting, that syntax errors are returned as error values,
that the `variant` version matches the class version, and even that
**a 1000-level nesting does not crash and becomes an error.**

## 23.14 Summary of this chapter

- **First, the decision not to use it.** For settings, use YAML/JSON. There are almost no cases to write your own parser in a team
- What is worth learning is the shape "**evaluate a tree recursively.**" Behavior trees and scene graphs have the same shape
- A syntax tree is a tree of `std::unique_ptr`. It is **the tree with the simplest ownership**, and you do not need `shared_ptr`
- **Do not give `parse()` to the node.** If you separate the parser and the AST, the responsibilities split and you can test
- Return syntax errors **as values, not exceptions.** Always include `position`. In C++23, use `std::expected`
- For recursive descent, **check the depth at the entrance.** If you do not, correct input overruns the stack (measured: exit code 139)
- If the kinds are fixed, use `std::variant`. But **for a recursive type, the indirection does not disappear**
- On microcontrollers, **do not parse at run time.** With a fixed-length instruction sequence + a loop stack, there is zero recursion and zero allocation

---

## After finishing this course

Thank you for your hard work through the 23 chapters. Finally, let me sum up what we saw across the whole course.

### 4 points kept coming back

There were 23 patterns, but **what C++ asked every time was 4 things.**

1. **Who owns it?** `unique_ptr` / `shared_ptr` / reference / value.
   Where the Java version did `new` and returned it, almost every chapter ended up returning a `unique_ptr`.
   Sharing was really needed only in about Flyweight and Observer
2. **Did you write a virtual destructor?** Every chapter where you wrote a pure virtual function needed this.
   If you forget it, it leaks silently
3. **Value, copy, or reference?** This is a decision Java does not have. Where you returned `Object`,
   whether you make it `const T &` or `T` changed both performance and lifetime
4. **Can you decide it at compile time?** If you use templates, the vtable and the heap disappear.
   This option always came up in Strategy, State, Visitor, and Interpreter

**When you start a new chapter, ask these 4 in order.** That settles 80%.

### The conclusion for microcontrollers was almost the same in every chapter

- **Do not use the heap** (if you allocate, do it only once at startup)
- **Do not use exceptions** (errors are return values: `bool` / error codes / your own Result type)
- **Ask whether you really need run-time polymorphism** (if there is only one implementation, you do not need virtual functions)
- **Move things to compile time** (templates, `constexpr`, fixed-length arrays)

After writing 23 chapters, you may not have expected the microcontroller conclusions to line up this much.
Put the other way, **on a microcontroller, many of the GoF patterns do not fit in their original form.**
The job is not "do not use patterns". It is **to achieve the same intent in a form without allocation or exceptions.**

### Of the 23 GoF patterns, only a few are "written by yourself" in C++

| Pattern | In C++ |
| --- | --- |
| Iterator | `begin()` / `end()` and `<algorithm>` already exist |
| Command | `std::function` is almost it |
| Proxy | Smart pointers are Proxies themselves |
| Flyweight | `shared_ptr` and `string_view` |
| Visitor | `std::variant` + `std::visit` |
| Strategy | A template argument or `std::function` |
| Prototype | The copy constructor already exists (`clone()` only for polymorphism) |
| Singleton | A function-local static (Meyers Singleton). The language guarantees it |

Even so, we implemented each one by hand once, **to understand why the standard has the form it has.**
If you write the GoF `Iterator` and then write `begin()` / `end()`,
you see what the design of "expressing the end with another iterator" made possible.
If you had skipped writing them yourself, `std::function` would have stayed just a convenient box.

### What to do next

When you finish reading, do a **design review of your team's library.** Look at 2 points.

1. **Which of these 23 does the current code use?** It has no name, but
   it is actually Strategy or Observer. Once it has a name, reviews get faster
2. **Does it use something it should not?** An abstraction with only one implementation,
   a Singleton that is just a renamed global variable, 3-level creation,
   a `shared_ptr` where a `unique_ptr` would do

And go back to the 4 checks in [0. Before you use them](00_before_you_use_them.md).

1. Are there 2 or more implementations now?
2. If you remove this abstraction, which changes become hard?
3. Can you say in one line who frees this object?
4. Is there the same thing in the standard library?

**Having read 23 chapters, you should now feel the weight of those 4 more.**

---

Previous: [22. Command](22_Command.md) / Next: [After finishing this course](README.md)
