/************************************************************
  **** x86def.h (header)
   ***
    ** Author:
     *   Tommy Hellstrom
     *
     * Description:
     *   Definitions for x86 ISA
     *
     * Revision history:
     *   When         Who       What
     *   20090519     me        created
     *
     * License information:
     *   To be decided... possibly GPL
     *
     ********************************************************/

#pragma once
#ifndef _X86DEF_H_
#define _X86DEF_H_

//mod field in ModR/M byte
#define X86_MOD_MEM         0   //00
#define X86_MOD_MEM_DISPB   1   //01
#define X86_MOD_MEM_DISPDW  2   //10
#define X86_MOD_REG         3   //11

#define X86_MODRM_BYTE(mod, reg, rm) (((mod) << 6) | ((reg) << 3) | (rm))

//use 16 bit registers
#define X86_PREFIX_REG16 0x66
//16 bit indirect addresses
#define X86_PREFIX_MEM16 0x67

#define X86_COUNT_REGS_8BIT  8
#define X86_COUNT_REGS_16BIT 8
#define X86_COUNT_REGS_32BIT 8

//register 8bit
#define X86_REG_AL 0
#define X86_REG_CL 1
#define X86_REG_DL 2
#define X86_REG_BL 3
#define X86_REG_AH 4
#define X86_REG_CH 5
#define X86_REG_DH 6
#define X86_REG_BH 7

//register 16bit
#define X86_REG_AX 0
#define X86_REG_CX 1
#define X86_REG_DX 2
#define X86_REG_BX 3
#define X86_REG_SP 4
#define X86_REG_BP 5
#define X86_REG_SI 6
#define X86_REG_DI 7

//register 32bit
#define X86_REG_EAX 0
#define X86_REG_ECX 1
#define X86_REG_EDX 2
#define X86_REG_EBX 3
#define X86_REG_ESP 4
#define X86_REG_EBP 5
#define X86_REG_ESI 6
#define X86_REG_EDI 7

#endif //_X86DEF_H_
