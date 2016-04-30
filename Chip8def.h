/************************************************************
  **** chip8def.h (header)
   ***
    ** Author:
     *   Tommy Hellstrom
     *
     * Description:
     *   Definitions for CHIP-8
     *
     * Revision history:
     *   When         Who       What
     *   20090519     me        created
     *
     * License information:
     *   GPLv3
     *
     ********************************************************/

#pragma once
#ifndef _CHIP8DEF_H_
#define _CHIP8DEF_H_

#define C8_MEMSIZE          4096
#define C8_PC_START         0x200
#define C8_GPREG_COUNT      16
#define C8_FLAG_REG         15
#define C8_OPCODE_SIZE      2
#define C8_STACK_DEPTH      16
#define C8_GPREG_SIZE       1
#define C8_ADDRESSREG_SIZE  2
#define C8_KEY_COUNT        16
#define C8_RES_WIDTH        64
#define C8_RES_HEIGHT       32
#define C8_PIXEL_ON         1
#define C8_PIXEL_OFF        0

#endif //_CHIP8DEF_H_
