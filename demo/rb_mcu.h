#pragma once

#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <math.h>

/* 投标之和长度标注 */
#define RB_MCU_PACKAGE_SIZE     (12)
#define RB_MCU_PROTO_HEAD       (0xe8)

/* 串口缓存数组下标 */
#define SERIAL_CMD_HEADER            (0)    ///< 串口发送/接收命令头位
#define SERIAL_CMD                   (1)    ///< 串口发送/接收控制/获取命令位
#define SERIAL_DATA_1                (2)    ///< 串口发送/接收数据位1
#define SERIAL_DATA_2                (3)    ///< 串口发送/接收数据位2
#define SERIAL_DATA_3                (4)    ///< 串口发送/接收数据位3
#define SERIAL_DATA_4                (5)    ///< 串口发送/接收数据位4
#define SERIAL_DATA_5                (6)    ///< 串口发送/接收数据位5
#define SERIAL_DATA_6                (7)    ///< 串口发送/接收数据位6
#define SERIAL_DATA_7                (8)    ///< 串口发送/接收数据位7
#define SERIAL_DATA_8                (9)    ///< 串口发送/接收数据位8
#define SERIAL_DATA_9               (10)    ///< 串口发送/接收数据位9
#define SERIAL_CHECK                (11)    ///< 串口发送/接收缓存数据校验位
#define UART232_ERR_TRY_CNT			(10)    ///< 允许的最大错误次数

/****************************APP&MCU串口交互命令************************************/
/*1. 获取类命令 */
#define GET_TRACK_MCU_VERSION_CMD		(0x01) ///< 获取轨道机运动单片机版本号
#define GET_TRACK_XY_POS_CMD			(0x02) ///< 获取轨道机XY坐标
#define GET_TRACK_SELFCHECK_INFO_CMD	(0x03) ///< 获取轨道机自检信息
#define GET_TRACK_XY_SPEED_CMD			(0x04) ///< 获取轨道机XY速度
#define GET_TRACK_PID_PARAM_CMD			(0x05) ///< 获取PID参数
#define GET_TRACK_MCU_ERROR_CODE_CMD	(0x06) ///< 获取单片机错误信息
#define GET_TRACK_MCU_STATE_INFO_CMD	(0x07) ///< 获取单片机状态信息
#define GET_TRACK_X_MAX_VALUE_CMD		(0x08) ///< 获取轨道机最大值信息
#define GET_TRACK_NFC_INFO_CMD			(0x09) ///< 获取轨道机NFC信息
#define GET_TRACK_TOTAL_MILEAGE_CMD		(0x0A) ///< 获取轨道机总里程信息
#define GET_MOTOR_MAINTAIN_INFO_CMD		(0x0D) ///< 获取电机维护信息
#define GET_CURVE_TRACK_POS_CMD			(0x0E) ///< 获取某一段弯轨线段起始点位置信息
#define GET_TRACK_MAINTAIN_INFO_CMD_NEW (0x0F) ///< 获取轨道可维护信息主动上报

/*2. 设置类命令 */
#define SET_KEY_XY_CMD					(0x30) ///< 键控XY
#define SET_XY_CMD						(0x31) ///< 绝对位置
#define SET_XY_STOP						(0x32) ///< 停止
#define SET_TRACK_AUTO_SCHECK_CMD		(0x33) ///< 设置轨道机上电自动自检使能
#define SET_X_SELF_CHECK_CMD			(0x34) ///< 水平自检（简化）
#define SET_X_WHOLE_CHECK_CMD			(0x35) ///< 水平自检（全检模式或首次标定）
#define SET_Y_SELF_CHECK_CMD			(0x36) ///< 垂直自检
#define SET_TRACK_PID_PARAM_CMD			(0x37) ///< 设置PID参数
#define SET_KEY_LIMIT_X_CMD				(0x38) ///< 设置水平键控限位
#define SET_KEY_LIMIT_Y_CMD				(0x39) ///< 设置垂直键控限位
#define SET_MCU_REBOOT_CMD				(0x3A) ///< 设置单片机重启
#define SET_DIREC_Y_LIFTING_LENGTH_CMD	(0x3B) ///< 设置垂直升降杆长度
#define SET_CURVE_TRACK_POS_CMD			(0x3C) ///< 设置下发弯轨线段起始点位置，用于单片机底层做弯轨限速保护逻辑
#define SET_WHOLE_INVALID				(0x3D) ///< 设置首次标定无效，清除电子地图
#define SET_REBOOT_CALLBACK_CMD			(0x3E) ///< 设置重启前回调	

/* 串口数据拼接 */
#define DATA_SPLICING_BYTE_2(a,b)     (((a) << 8) | (b))  ///< 2个字节数据拼接
#define DATA_SPLICING_BYTE_4(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))  ///< 4个字节数据拼接
#define DATA_SPLICING_BYTE_1(a,b) 	  (((a) & 0x0f) | ((b) & 0xf0))  ///< 1个字节数据拼接


#define GET_HIGH_24_31_BIT_FROM_4_BYTE_DATA(a) (((a) >> 24) & 0x000000ff)
#define GET_HIGH_16_23_BIT_FROM_4_BYTE_DATA(a) (((a) >> 16) & 0x000000ff)
#define GET_LOW_8_15_BIT_FROM_4_BYTE_DATA(a)  (((a) >> 8) & 0x000000ff)
#define GET_LOW_0_7_BIT_FROM_4_BYTE_DATA(a)   ((a) & 0x000000ff)

#define GET_HIGH_8_BIT_FROM_2_BYTE_DATA(a) (((a) >> 8) & 0x00ff)
#define GET_LOW_8_BIT_FROM_2_BYTE_DATA(a)  ((a)  & 0x00ff)

#define TRACK_KEY_XY_SPEED_NEGATIVE_PROC(speed)  (0x8000 | (abs(speed) & 0x7fff))  ///< 轨道机键控速度负方向处理
