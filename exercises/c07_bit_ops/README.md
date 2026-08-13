# c07 ビット演算とレジスタ操作 〔C言語編〕

ビット演算を使って CAN ID の組み立て・分解と、レジスタの read-modify-write を実装する課題です。

## ヘッダは既に完成しています

`include/drill/bit_ops.h` は**編集しません**。CAN ID とレジスタ操作の関数インタフェースが定義されています。

## CAN ID の仕様

11 bit の ID は 3 つのフィールドで構成されます：
- `[10:8]` **ReceiverType** (3 bit) — 受信対象
  - 0=ALL, 1=MD, 2=BLDC, 3=OTHER_ACTUATOR, 4=SENSOR, 5=MICROCONTROLLER, 6=MAINBOARD_PC, 7=UNDEFINED
- `[7:4]` **DeviceId** (4 bit) — デバイス番号 (0-15)
- `[3:0]` **Command** (4 bit) — コマンド (0-15)

## やること

編集するのは `src/bit_ops.c` だけです。以下の関数を実装してください：

### CAN ID 関数
- `can_id_create(type, device, cmd)` — 3 つのフィールドから 11 bit ID を組み立てる
- `can_id_extract_type(id)` — ID から Type を抽出
- `can_id_extract_device(id)` — ID から Device を抽出
- `can_id_extract_cmd(id)` — ID から Cmd を抽出
- `can_id_set_device(id, new_device)` — **Read-Modify-Write** で Device だけ変更（他を壊さない）

### レジスタ操作関数
- `register_set_bit(reg, bit_pos)` — 指定ビットを 1 にする
- `register_clear_bit(reg, bit_pos)` — 指定ビットを 0 にする
- `register_read_bit(reg, bit_pos)` — 指定ビットを読む
- `register_set_bits(reg, msb, lsb, value)` — ビット範囲 [msb:lsb] を value に設定

## 動かしてみる

```bash
./drill run c07
```

## つまずきポイント

- **シフト演算** — `(value << n)` で n ビット左シフト。左シフトでビット幅が伸びることに注意（`uint8_t` を `uint16_t` へ）
- **マスク** — ビット範囲を取り出すときは `(value >> shift) & mask` でフィルタ
- **Read-Modify-Write** — `can_id_set_device` では Device フィールド以外を保存する必要があります
  - 不正な実装：`(new_device << 4)` — Type と Cmd が失われます
  - 正しい実装：`(id & ~0xF0) | (new_device << 4)` — Device をクリアして新値を追加
- **ビット範囲の設定** — `register_set_bits` ではマスク計算が必要です

## テスト

| テスト | 見ているところ |
| --- | --- |
| `組み立てと抽出が往復する` | ビット操作の基本（シフト・マスク） |
| `ReadModifyWrite_他のビットを壊さない` | read-modify-write パターン |
| `ビットを立てる/落とす` | レジスタ操作の単一ビット |
| `ビット範囲を設定` | 複数ビットの read-modify-write |

## 参考

- [7. ビット演算とレジスタ操作](../../docs/c/07_ビット演算とレジスタ操作.md)
