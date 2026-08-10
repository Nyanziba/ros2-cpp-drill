/* このファイルは編集しません（インタフェースの提示）。 */

#ifndef DRILL_BIT_OPS_H
#define DRILL_BIT_OPS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===== CAN ID の仕様 =====
 * 11 bit の ID は次の 3 つの フィールドを含みます：
 *   [10:8] ReceiverType (3 bit)  — 受信対象
 *   [7:4]  DeviceId (4 bit)      — デバイス番号
 *   [3:0]  Command (4 bit)       — コマンド
 *
 * ReceiverType の値：
 *   0=ALL, 1=MD, 2=BLDC, 3=OTHER_ACTUATOR,
 *   4=SENSOR, 5=MICROCONTROLLER, 6=MAINBOARD_PC, 7=UNDEFINED
 */

/* CAN ID を組み立てる。3 つのフィールドから 11 bit ID を作る */
uint16_t can_id_create(uint8_t type, uint8_t device, uint8_t cmd);

/* CAN ID からフィールドを抽出する */
uint8_t can_id_extract_type(uint16_t id);
uint8_t can_id_extract_device(uint16_t id);
uint8_t can_id_extract_cmd(uint16_t id);

/* Read-Modify-Write：既存の ID を保持しつつ、Device フィールドだけ変更する */
uint16_t can_id_set_device(uint16_t id, uint8_t new_device);

/* ===== レジスタ操作 =====
 * 16 bit のレジスタを想定し、特定ビットを立てる/落とす/読む関数
 */

/* 指定したビット位置を 1 にする（他のビットは変えない） */
uint16_t register_set_bit(uint16_t reg, uint8_t bit_pos);

/* 指定したビット位置を 0 にする（他のビットは変えない） */
uint16_t register_clear_bit(uint16_t reg, uint8_t bit_pos);

/* 指定したビット位置を読む（0 または 1） */
uint8_t register_read_bit(uint16_t reg, uint8_t bit_pos);

/* 指定したビット範囲を設定する [msb_pos:lsb_pos] (msb_pos >= lsb_pos) */
uint16_t register_set_bits(uint16_t reg, uint8_t msb_pos, uint8_t lsb_pos, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* DRILL_BIT_OPS_H */
