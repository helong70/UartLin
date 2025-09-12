#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "UartLin.h"



static LinContext_t lin_ctx;
// 初始化

void UART_Init(void) {
	// 伪代码：根据具体平台实现Uart初始化
}
void Lin_Init(void) {
	Lin_Reset();
	// UART初始化（伪代码，根据平台实现）
	UART_Init();
}
void UART_SendByte(uint8_t byte) {
	// 伪代码：根据具体平台实现Uart发送一个字节
}

// 发送BREAK：保存当前波特率，降低波特率发送0x00以延长低电平时间，然后恢复波特率
void UART_SendBreak(void) {
	#if (BREAK_MODE==SWITCH_BAUDRATE_BREAK_MODE)
	UART_SetBaud(9600); // 13个比特时间的低电平，假设原始波特率为19200bps
	// 发送 0x00（持续为低电平比特）
	UART_SendByte(0x00);
	#elif (BREAK_MODE==HARDWARE_BREAK_MODE)
	// 硬件方式发送BREAK，具体实现依赖于MCU平台
	#endif
}


#if (BREAK_MODE==SWITCH_BAUDRATE_BREAK_MODE)
void UART_SetBaud(uint32_t baud){

}
#elif (BREAK_MODE==HARDWARE_BREAK_MODE)
// 硬件方式接收BREAK中断回调
void Uart_ReciveBreakCb(void) {
	if(lin_ctx.state == LIN_STATE_BREAK) {
		UART_SendByte(0xAA);
		lin_ctx.state = LIN_STATE_SYNC;
	}
	else
	{
		// 非预期数据，复位状态机
		Lin_Reset();
	}
}
#endif

// 超时阈值（根据实际定时器设置调整）
#define LIN_TIMEOUT_THRESHOLD 1000
void Lin_TxCallback(uint8_t id, uint8_t *data, uint8_t len) {
	
}
void Lin_RxCallback(uint8_t id, uint8_t *data, uint8_t len) {
	
}
// 状态机复位
void Lin_Reset(void) {
	lin_ctx.state = LIN_STATE_IDLE;
	lin_ctx.timeout_cnt = 0;
	lin_ctx.data_len = 0;
	lin_ctx.id = 0;
	lin_ctx.pid = 0;
	lin_ctx.checksum = 0;
	lin_ctx.is_tx = false; // 默认接收
	lin_ctx.remaining = 0;
	lin_ctx.on_tx_done = Lin_TxCallback;
	lin_ctx.on_rx_done = Lin_RxCallback;
	lin_ctx.checksum_mode = LIN_CHECKSUM_ENHANCED; // 默认增强校验
	memset(lin_ctx.data, 0, sizeof(lin_ctx.data));
	#if (BREAK_MODE==SWITCH_BAUDRATE_BREAK_MODE)
	UART_SetBaud(19200); // 默认波特率
	#endif
}


// Uart中断回调（伪代码，需根据MCU平台适配）
void UART_IRQHandler(void) {
	// 读取Uart接收数据
	uint8_t rx = UART_ReadByte();
	lin_ctx.timeout_cnt = 0; // 重置超时计数

	// 状态机转换
	switch (lin_ctx.state) {
		case LIN_STATE_IDLE:
			break;
		case LIN_STATE_BREAK:
			#if (BREAK_MODE==SWITCH_BAUDRATE_BREAK_MODE)
			if (rx == 0x00) { // SYNC
				lin_ctx.state = LIN_STATE_SYNC;
				// 收到 SYNC 后发送 0xAA（应答/占位符），具体值可根据协议调整
				UART_SetBaud(19200); // 恢复正常波特率
				UART_SendByte(0xAA);
			}
			else
			{
				// 非预期数据，复位状态机
				Lin_Reset();
			}
			#else
			break;
		case LIN_STATE_SYNC:
			if(rx==0xAA)
			{
				UART_SendByte(lin_ctx.pid); // 示例发送PID
				lin_ctx.state = LIN_STATE_PID;
				break;
			}
			else
			{
				// 非预期数据，复位状态机
				Lin_Reset();
			}
		case LIN_STATE_PID:
			if(rx==lin_ctx.pid)
			{
				if(lin_ctx.is_tx) 
				{
				UART_SendByte(lin_ctx.data[lin_ctx.data_len - lin_ctx.remaining]); // 发送数据字节
				lin_ctx.state = LIN_STATE_DATA;
				break;
				}
				else
				{
					lin_ctx.remaining = lin_ctx.data_len; // 准备接收数据
					lin_ctx.state = LIN_STATE_DATA;
					break;
				}
			}
			else
			{
				// 非预期数据，复位状态机
				Lin_Reset();
			}
			break;
		case LIN_STATE_DATA:
			if(lin_ctx.is_tx)
			{
				if(rx==lin_ctx.data[lin_ctx.data_len - lin_ctx.remaining])
				{
					lin_ctx.remaining--;
					if(lin_ctx.remaining>0)
					{
						UART_SendByte(lin_ctx.data[lin_ctx.data_len - lin_ctx.remaining]); // 发送下一个数据字节
					}
					else
					{
						UART_SendByte(lin_ctx.checksum); // 发送校验和
						lin_ctx.state = LIN_STATE_CHECK;
					}
					break;
				}
				else
				{
					// 非预期数据，复位状态机
					Lin_Reset();
				}
			}
			else
			{
				lin_ctx.data[lin_ctx.data_len - lin_ctx.remaining] = rx;
				lin_ctx.remaining--;
				if(lin_ctx.remaining==0)
				{
					// 接收完数据，准备接收校验和
					lin_ctx.state = LIN_STATE_CHECK;
				}
				break;
			}
			break;
		case LIN_STATE_CHECK:

			if(lin_ctx.is_tx)
			{
				if(rx==lin_ctx.checksum)
				{
					// 校验成功
					lin_ctx.state = LIN_STATE_DONE;
				}
				else
				{
					// 非预期数据，复位状态机
					Lin_Reset();
				}
			}
			else
			{
				// 接收时计算校验和
				uint8_t calc_checksum =CalcChecksum(lin_ctx.data, lin_ctx.data_len, lin_ctx.checksum_mode, lin_ctx.pid);

				if(rx==calc_checksum)
				{
					// 校验成功
					lin_ctx.state = LIN_STATE_DONE;
				}
				else
				{
					// 非预期数据，复位状态机

					Lin_Reset();
				}

			}
			break;
		case LIN_STATE_DONE:
			// 调用完成回调
			if (lin_ctx.is_tx) {
				if (lin_ctx.on_tx_done) lin_ctx.on_tx_done(lin_ctx.pid, lin_ctx.data, lin_ctx.data_len);
			} else {
				if (lin_ctx.on_rx_done) lin_ctx.on_rx_done(lin_ctx.pid, lin_ctx.data, lin_ctx.data_len);
			}
			Lin_Reset();
			break;
		default:
			Lin_Reset();
			break;
	}
}

// 超时处理（需在定时器中调用）
void Lin_TimeoutHandler(void) {
	if (lin_ctx.state != LIN_STATE_IDLE) {
		lin_ctx.timeout_cnt++;
		if (lin_ctx.timeout_cnt > LIN_TIMEOUT_THRESHOLD) {
			Lin_Reset();
		}
	}
}



void Lin_SetTxCallback(LinTxCallback_t cb) {
	lin_ctx.on_tx_done = cb;
}

void Lin_SetRxCallback(LinRxCallback_t cb) {
	lin_ctx.on_rx_done = cb;
}

// 发送LIN帧（伪代码）
void Lin_SendFrame(uint8_t id, uint8_t *data, uint8_t len, LinChecksumMode_t checksum_mode) {
	// 限制长度到 8
	if (len > 8) len = 8;
	uint8_t pid = Lin_IdToPid(id);
	lin_ctx.pid = pid;
	lin_ctx.data_len = len;
	lin_ctx.remaining = len;
	memcpy(lin_ctx.data, data, len);
	lin_ctx.checksum = CalcChecksum(data, len, checksum_mode, lin_ctx.pid);
	lin_ctx.checksum_mode = checksum_mode;
	lin_ctx.state = LIN_STATE_BREAK;
	lin_ctx.is_tx = true; // 发送方向
	// 通过修改 UART 波特率发送 BREAK
	UART_SendBreak();
	}
void Lin_ReceiveFrame(uint8_t id, uint8_t *data, uint8_t len, LinChecksumMode_t checksum_mode) {
	// 限制长度到 8
	if (len > 8) len = 8;
	uint8_t pid = Lin_IdToPid(id);
	lin_ctx.pid = pid;
	lin_ctx.data_len = len;
	lin_ctx.remaining = len;
	memset(lin_ctx.data, 0, sizeof(lin_ctx.data));
	lin_ctx.checksum = 0; // 接收时校验和由接收数据计算
	lin_ctx.checksum_mode = checksum_mode;
	lin_ctx.state = LIN_STATE_BREAK;
	lin_ctx.is_tx = false; // 接收方向
	// 通过修改 UART 波特率发送 BREAK
	UART_SendBreak();
}

// 计算校验和（简单反码和）
uint8_t CalcChecksum(uint8_t *data, uint8_t len, LinChecksumMode_t mode, uint8_t pid) {
	uint8_t sum = 0;
	if (mode == LIN_CHECKSUM_ENHANCED) {
		// 在增强校验中先把 PID 加入
		sum += pid;
	}
	for (uint8_t i = 0; i < len; i++) {
		sum += data[i];
	}
	return (uint8_t)(~sum);
}

// 将 6-bit ID 转换为包含奇偶校验位的 PID
// LIN parity 计算：
// P0 = ID0 ^ ID1 ^ ID2 ^ ID4
// P1 = !(ID1 ^ ID3 ^ ID4 ^ ID5)
uint8_t Lin_IdToPid(uint8_t id) {
	id &= 0x3F; // 仅保留低6位
	uint8_t id0 = (id >> 0) & 1;
	uint8_t id1 = (id >> 1) & 1;
	uint8_t id2 = (id >> 2) & 1;
	uint8_t id3 = (id >> 3) & 1;
	uint8_t id4 = (id >> 4) & 1;
	uint8_t id5 = (id >> 5) & 1;

	uint8_t p0 = id0 ^ id1 ^ id2 ^ id4;
	uint8_t p1 = (uint8_t)(~(id1 ^ id3 ^ id4 ^ id5)) & 1;

	uint8_t pid = id | (p0 << 6) | (p1 << 7);
	return pid;
}



