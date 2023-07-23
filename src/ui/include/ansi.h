//
// Created by marvin on 23-7-18.
//
#pragma once

#include <cstdint>

namespace ANSI {

enum SequenceStartCnt {
    HEAD_CNT = 2,
};

// reference: https://www.ecma-international.org/publications-and-standards/standards/ecma-48/
enum SequenceFirst {
    EXC = 0x1B, // Escape（转义）
};

// reference: https://www.ecma-international.org/publications-and-standards/standards/ecma-48/
enum SequenceSecond {
    PAD = 0x40, // 0x80  Padding Character（填充字符）
    HOP = 0x41, // 0x81  High Octet Preset（高字节前置）
    BPH = 0x42, // 0x82  Break Permitted Here（此处允许中断）
    NBH = 0x43, // 0x83  No Break Here（此处禁止中断）
    IND = 0x44, // 0x84  Index（索引）
    NEL = 0x45, // 0x85  Next Line（下一行）
    SSA = 0x46, // 0x86  Start of Selected Area（选择区域开始）
    ESA = 0x47, // 0x87  End of Selected Area（选择区域结束）
    HTS = 0x48, // 0x88  Horizontal Tab Set（水平制表设置）
    HTJ = 0x49, // 0x89  Horizontal Tab Justified（水平制表调整）
    VTS = 0x4A, // 0x8A  Vertical Tab Set（垂直制表设置）
    PLD = 0x4B, // 0x8B  Partial Line Forward（部分行前移）
    PLU = 0x4C, // 0x8C  Partial Line Backward（部分行后移）
    RI  = 0x4D, // 0x8D  Reverse Line Feed（逆向馈行）
    SS2 = 0x4E, // 0x8E  Single-Shift 2（单个移动2）
    SS3 = 0x4F, // 0x8F  Single-Shift 3（单个移动3）
    DCS = 0x50, // 0x90  Device Control String（设备控制串）
    PU1 = 0x51, // 0x91  Private Use 1（私用1）
    PU2 = 0x52, // 0x92  Private Use 2（私用2）
    STS = 0x53, // 0x93  Set Transmit State（发送规则设置）
    CCH = 0x54, // 0x94  Cancel Character（取消字符）
    MW  = 0x55, // 0x95  Message Waiting（消息等待）
    SPA = 0x56, // 0x96  Start of Protected Area（保护区域开始）
    EPA = 0x57, // 0x97  End of Protected Area（保护区域结束）
    SOS = 0x58, // 0x98  Start of String（串开始）
    SGC = 0x59, // 0x99  Single Graphic Char Intro（单个图形字符描述）
    SCI = 0x5A, // 0x9A  Single Char Intro（单个字符描述）
    CSI = 0x5B, // 0x9B  Control Sequence Intro（控制顺序描述）
    ST  = 0x5C, // 0x9C  String Terminator（串终止）
    OSC = 0x5D, // 0x9D  OS Command（操作系统指令）
    PM  = 0x5E, // 0x9E  Private Message（私讯）
    APC = 0x5F, // 0x9F  App Program Command（应用程序命令）

    SECOND_BYTE_BEGIN = PAD,
    SECOND_BYTE_END   = APC,
};

// reference: https://www.ecma-international.org/publications-and-standards/standards/ecma-48/
enum CSIParameterBytes {
    // ASCII: 0–9:;<=>?
    NUM_BEGIN                  = 0x30,
    NUM_END                    = 0x39,
    SUB_PARA_SEPARATOR         = 0x3A,
    PARA_SEPARATOR             = 0x3B,
    STANDARDIZATION_KEEP_BEGIN = 0x3C,
    STANDARDIZATION_KEEP_END   = 0x3F,

    CSI_PARAMETER_BEGIN = 0x30,
    CSI_PARAMETER_END   = 0x3F,
};

enum CSIIntermediateBytes {
    // ASCII: Space、!"#$%&'()*+,-./
    CSIIntermediateBegin = 0x20,
    CSIIntermediateEnd   = 0x2F,
};

enum CSIFinalBytes {
    // ASCII: @A–Z[\]^_`a–z{|}~
    SGR = 0x6D, // m

    CSIFinalBegin = 0x40,
    CSIFinalEnd   = 0x7E,

    CSIFinalExperimentalBegin = 0x70,
    CSIFinalExperimentalEnd   = CSIFinalEnd,
};

enum class Return {
    PARSE_ERROR = -1,
    PARSE_SUCC  = 0,
};

} // namespace ANSI
