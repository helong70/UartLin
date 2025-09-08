# UartLin

简单的基于 UART 模拟 LIN 协议的实现（样例/框架）。

## 目标
通过 MCU 的 UART 和外接收发器模拟 LIN 总线：发送时通过改变 UART 波特率生成 BREAK，并用状态机在 UART 中断中完成 SYNC / PID / DATA / CHECKSUM 的发送接收流程；接收时回环数据由中断解析并做校验。

## 特性（当前实现）
- 状态机实现 LIN 帧的发送/接收（在 `UART_IRQHandler` 中运行）。
- 发送函数 `Lin_SendFrame` / 接收函数 `Lin_ReceiveFrame`：调用时先保存 PID/data/len，发送 BREAK 后由状态机继续完成帧事务。
- 通过修改 UART 波特率来产生 BREAK（在 `UART_SendBreak` 中保存原波特率并切换到较低波特率，然后等待回环字节在中断中恢复）。
- 超时机制：`Lin_TimeoutHandler` 用于复位状态机。
- 支持发送完成和接收完成回调。

## 代码结构
- `UartLin.h`：对外 API、类型和回调 typedef。
- `UartLin.c`：状态机实现、API 具体实现、BREAK 生成逻辑（平台无关代码）。
- `task_template.md`：任务模版（用于描述需求）。

## 主要 API
（在 `UartLin.h` 中声明）

- `void Lin_Init(void);` 初始化。
- `void Lin_Reset(void);` 复位状态机。
- `void Lin_SendFrame(uint8_t id, uint8_t *data, uint8_t len);` 发送（传入 6-bit ID，内部转换为 PID）。
- `void Lin_ReceiveFrame(uint8_t id, uint8_t *data, uint8_t *len);` 接收（以 ID 启动监听/回环）。
- `void Lin_TimeoutHandler(void);` 超时处理（需定时器定期调用）。
- `void Lin_SetTxCallback(LinTxCallback_t cb);` 设置发送完成回调。
- `void Lin_SetRxCallback(LinRxCallback_t cb);` 设置接收完成回调。
- `uint8_t Lin_IdToPid(uint8_t id);` 将 6-bit ID 转为含校验位的 PID。
- `uint8_t CalcChecksum(uint8_t *data, uint8_t len);` 计算校验和（简单反码和）。

## 平台适配（必须实现的底层函数）
在你的 MCU 平台上需要实现以下 UART 相关函数（`UartLin.c` 中有声明）：
- `void UART_Init(void);`
- `void UART_SendByte(uint8_t byte);`
- `uint8_t UART_ReadByte(void);`
- `uint32_t UART_GetBaud(void);`
- `void UART_SetBaud(uint32_t baud);`
- `void UART_WaitForTxComplete(void);`

注意：BREAK 生成逻辑依赖于外接收发器的 TX->RX 回环（硬件连线），UART 波特率在 `UART_SendBreak` 中被降低并设置 `break_active` 标记，收到回环 0x00 后将在 `UART_IRQHandler` 中恢复原波特率。

## 使用说明（快速）
1. 在 MCU 启动时调用 `Lin_Init()`。
2. 注册回调（可选）：
   - `Lin_SetTxCallback(cb);`
   - `Lin_SetRxCallback(cb);`
3. 发送帧：`Lin_SendFrame(id, data, len);`（len 最大 8）
4. 在定时器中调用 `Lin_TimeoutHandler()` 来处理超时复位。
5. 在 UART 中断里应调用或者保持 `UART_IRQHandler` 为中断入口（框架内实现了状态机）。

## 测试建议
- 在开发板上先实现 UART 平台函数并通过硬件回环（TX->收发器->RX）验证：调用 `Lin_SendFrame` 并观察回调或在示波器上查看帧结构（BREAK/SYNC/PID/DATA/CHECK）。
- 可以在 PC 端写一个简单的 UART 模拟器替代底层函数，以便在桌面环境编译/单元测试逻辑。

## 后续改进（建议）
- 支持可配置的帧超时时间和帧长度动态支持。
- 更健壮的错误处理和日志接口。
- 对接 RTOS 或中断优先级安全处理（避免在中断中做长时间阻塞调用）。



