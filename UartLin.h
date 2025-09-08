// UartLin.h - 公共接口与类型
#ifndef UARTLIN_H
#define UARTLIN_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LIN_STATE_IDLE = 0,
    LIN_STATE_BREAK,
    LIN_STATE_SYNC,
    LIN_STATE_PID,
    LIN_STATE_DATA,
    LIN_STATE_CHECK,
    LIN_STATE_DONE,
    LIN_STATE_ERROR
} LinState_t;

typedef void (*LinTxCallback_t)(uint8_t pid, uint8_t *data, uint8_t len);
typedef void (*LinRxCallback_t)(uint8_t pid, uint8_t *data, uint8_t len);

// 校验方式
typedef enum {
	LIN_CHECKSUM_CLASSIC = 0, // 经典校验（仅数据）
	LIN_CHECKSUM_ENHANCED     // 增强校验（包含PID）
} LinChecksumMode_t;

typedef struct {
	LinState_t state;
	uint32_t timeout_cnt;
	uint8_t data[8];
	uint8_t data_len;
	uint8_t id;
	uint8_t pid;
	uint8_t checksum;
	// 当前方向：接收(RX) 或 发送(TX)
	bool is_tx;
	// 剩余未发送/未接收的字节数
	uint8_t remaining;
	// 校验方式
	LinChecksumMode_t checksum_mode;
	// 回调：发送完成和接收完成
	LinTxCallback_t on_tx_done;
	LinRxCallback_t on_rx_done;
} LinContext_t;

// API
void Lin_Init(void);
void Lin_Reset(void);
void Lin_SendFrame(uint8_t id, uint8_t *data, uint8_t len);
void Lin_ReceiveFrame(uint8_t id, uint8_t *data, uint8_t len);
void Lin_TimeoutHandler(void);

void Lin_SetTxCallback(LinTxCallback_t cb);
void Lin_SetRxCallback(LinRxCallback_t cb);

// Helper
uint8_t Lin_IdToPid(uint8_t id);
uint8_t CalcChecksum(uint8_t *data, uint8_t len, LinChecksumMode_t mode, uint8_t pid);

#endif // UARTLIN_H
