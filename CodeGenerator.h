/************************************************************
  **** CodeGenerator_x86.cpp (implementation of .h)
   ***
    ** Author:
     *   Tommy Hellstrom
     *
     * Description:
     *  Dynamic machinecode generator for x86
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
#ifndef _CODEGENERATOR_X86_H_
#define _CODEGENERATOR_X86_H_

#include <list>
#include <vector>
#include <stdint.h>
#include <stddef.h>

#include "x86def.h"

#define CG_BLOCK_SIZE 10240

#define CG_INT8_MIN -128
#define CG_INT8_MAX  127

//must be power of 2
#define CG_ALIGNMENT 16

typedef int Label_t;

class CodeGenerator
{
    private:

        typedef void (CodeGenerator::*CodeGeneratorMemberFn_t)(int32_t);

        /**
         * Stores information about jumps.
         */
        struct Jump
        {
            int                     index;
            int                     label;
            CodeGeneratorMemberFn_t pfnJmp;

            Jump(const int i, const int lbl, const CodeGeneratorMemberFn_t pfn)
            {index = i; label = lbl; pfnJmp = pfn;}
         // Jump(const Jump&);
         // Jump& Jump=(const Jump&);
         // ~Jump()
        };

        /**
         * Stores information about labels
         */
        struct Label
        {
            int     index;
            bool    inserted;

            Label() {inserted = false;}
         // Label(const Label&);
         // Label& Label=(const Label&);
         // ~Label()
        };

        std::vector<Label *> mLabels;
        std::list<Jump *>    mJumps;
        uint8_t              mMachineCode[CG_BLOCK_SIZE];
        int                  mIndex;

        /**
         * Insert all jumps into the code.
         */
        void insertJumps();

        /**
         * Jump If Not Zero.
         * Insert a jump into the code
         *
         * PARAMS
         *   rel    distance between label and jump
         */
        void insert_jnz(const int32_t rel);

        /**
         * Jump If Zero.
         * Insert a jump into the code
         *
         * PARAMS
         *   rel    distance between label and jump
         */
        void insert_jz(const int32_t rel);

        /**
         * Jump If Not Carry.
         * Insert a jump into the code
         *
         * PARAMS
         *   rel    distance between label and jump
         */
        void insert_jnc(const int32_t rel);

        /**
         * Jump If Carry.
         * Insert a jump into the code
         *
         * PARAMS
         *   rel    distance between label and jump
         */
        void insert_jc(const int32_t rel);

        /**
         * Jump.
         * Insert a jump into the code
         *
         * PARAMS
         *   rel    distance between label and jump
         */
        void insert_jmp(const int32_t rel);

        /**
         * Destroy all data in the object.
         */
        void destroy();

    public:

        /**
         * Insert a label at current position
         *
         * PARAMS
         * id    Label id
         */
        void insertLabel(const Label_t id);

        /**
         * Creates a new label
         *
         * RETURNS
         * a unique label id
         */
        Label_t newLabel();

        /**
         * Copies the code to the heap and return a pointer to it
         * The code is guaranteed to be aligned
         * RETURNS
         * void pointer to generated code
         */
        void* getAlignedCodePointer(void **const pBlock, size_t *size);

        /**
         * Copies the code to the heap and return a pointer to it
         * The code is NOT guaranteed to be aligned
         *
         * RETURNS
         * void pointer to generated code
         */
        void* getCodePointer(size_t *size);

        /**
         * Force alignment of current position (inserts nops)
         */
        void align16();

        /**
         * Request alignment of current position (inserts nops)
         */
        void align();

        /**
         * Reset
         */
        void reset();

        /**
         * Constructor
         */
        CodeGenerator();

        /**
         * Destructor
         */
        ~CodeGenerator();

        /**
         * MOV r32,imm32
         *
         * PARAMS
         * reg32   32 bit registernumber
         * imm32   32 bit immediate
         */
        void mov_r32i32(const int reg32, const uint32_t imm32);

        /**
         * MOV m32,imm32
         *
         * PARAMS
         * reg32   32 bit register
         * imm32   32 bit immediate
         */
        void mov_m32i32(const int reg32, const uint32_t imm32);

        /**
         * MOV m32,imm32
         *
         * PARAMS
         * reg32    32 bit register
         * imm32    32 bit immediate
         * disp8    8 bit memory displacement
         */
        void mov_m32i32_d8(const int reg32, const uint32_t imm32, const uint8_t disp8);

        /**
         * MOV r16,imm16
         *
         * PARAMS
         * reg16    16 bit register
         * imm16    16 bit immediate
         */
        void mov_r16i16(const int reg16, const uint16_t imm16);

        /**
         * MOV r32,r32
         *
         * PARAMS
         * reg32d    32 bit destination register
         * reg32s    32 bit source register
         */
        void mov_r32r32(const int reg32d, const int reg32s);

        /**
         * MOV r16,r16
         *
         * PARAMS
         * reg16d    16 bit destination register
         * reg16s    16 bit source register
         */
        void mov_r16r16(const int reg16d, const int reg16s);

        /**
         * MOVZX r32,r16
         *
         * PARAMS
         * reg32    32 bit destination register
         * reg16    16 bit source register
         */
        void movzx_r32r16(const int reg32, const int reg16);

        /**
         * MOVZX r32,m16
         *
         * PARAMS
         * reg32d    32 bit destination register
         * reg32s    32 bit memory pointer
         */
        void movzx_r32m16(const int reg32d, const int reg32s);

        /**
         * NOP
         */
        void nop();

        /**
         * MOV r8,i8
         *
         * PARAMS
         * reg8    8 bit destination register
         * imm8    8 bit immediate
         */
        void mov_r8i8(const int reg8, const uint8_t imm8);

        /**
         * MOV r8,r8
         *
         * PARAMS
         * reg8d    8 bit destination register
         * reg8s    8 bit source register
         */
        void mov_r8r8(const int reg8d, const int reg8s);

        /**
         * MOV r8,m8
         *
         * PARAMS
         * reg8d     8 bit destination register
         * reg32s    32 bit memory pointer
         */
        void mov_r8m8(const int reg8d, const int reg32s);

        /**
         * MOV r8,m8
         *
         * PARAMS
         * reg8d     8 bit destination register
         * reg32s    32 bit memory pointer
         * disp8     8 bit memory dispacement
         */
        void mov_r8m8_d8(const int reg8d, const int reg32s, const uint8_t disp8);

        /**
         * MOV r32,m32
         *
         * PARAMS
         * reg32d    32 bit destination register
         * reg32s    32 bit memory pointer
         */
        void mov_r32m32(const int reg32d, const int reg32s);

        /**
         * MOV r16,m16
         *
         * PARAMS
         * reg16d    16 bit destination register
         * reg32s    32 bit memory pointer
         */
        void mov_r16m16(const int reg16d, const int reg32s);

        /**
         * MOV m8,r8
         *
         * PARAMS
         * reg32d    32 bit memory pointer
         * reg32s    8 bit source register
         */
        void mov_m8r8(const int reg32d, const int reg8s);

        /**
         * MOV m8,r8
         *
         * PARAMS
         * reg32d    32 bit memory pointer
         * reg8s     8 bit source register
         * disp8     8 bit memory displacement
         */
        void mov_m8r8_d8(const int reg32d, const int reg8s, const uint8_t disp8);

        /**
         * MOV m32,r32
         *
         * PARAMS
         * reg32d    32 bit memory pointer
         * reg32s    32 bit source register
         */
        void mov_m32r32(const int reg32d, const int reg32s);

        /**
         * MOV m16,r16
         *
         * PARAMS
         * reg32d    32 bit memory pointer
         * reg16s    16 bit source register
         */
        void mov_m16r16(const int reg32d, const int reg16s);

        /**
         * MOVZX m32,r8
         *
         * PARAMS
         * reg32    32 bit destination register
         * reg8     8 bit source register
         */
        void movzx_r32r8(const int reg32, const int reg8);

        /**
         * MOVZX r32,m8
         *
         * PARAMS
         * reg32d    32 bit destination register
         * reg32s    32 bit memory pointer
         */
        void movzx_r32m8(const int reg32d, const int reg32s);

        /**
         * MOVZX r16,r8
         *
         * PARAMS
         * reg16    16 bit destination register
         * reg8     8 bit source register
         */
        void movzx_r16r8(const int reg16, const int reg8);

        /**
         * RET
         */
        void ret();

        /**
         * PUSHAD
         */
        void pushad();

        /**
         * POPAD
         */
        void popad();

        /**
         * POP r32
         *
         * PARAMS
         * reg32    32 bit register
         */
        void pop_r32(const int reg32);

        /**
         * POP r16
         *
         * PARAMS
         * reg16    16 bit register
         */
        void pop_r16(const int reg16);

        /**
         * CALL r32
         *
         * PARAMS
         * reg32    32 bit memory pointer
         */
        void call_r32(const int reg32);

        /**
         * CMP r8,i8
         *
         * PARAMS
         * reg8    8 bit register
         * imm8    8 bit immediate
         */
        void cmp_r8i8(const int reg8, const uint8_t imm8);

        /**
         * OR r8,i8
         *
         * PARAMS
         * reg8    8 bit register
         * imm8    8 bit immediate
         */
        void or_r8i8(const int reg8, const uint8_t imm8);

        /**
         * CMP m8,i8
         *
         * PARAMS
         * reg32   32 bit memory pointer
         * imm8    8 bit immediate
         */
        void cmp_m8i8(const int reg32, const uint8_t imm8);

        /**
         * CMP m8,i8
         *
         * PARAMS
         * reg32   32 bit memory pointer
         * imm8    8 bit immediate
         * disp8   8 bit memory displacement
         */
        void cmp_m8i8_d8(const int reg32, const uint8_t imm8, const uint8_t disp8);

        /**
         * CMP r8,r8
         *
         * PARAMS
         * reg8d   8 bit register
         * reg8s   8 bit register
         */
        void cmp_r8r8(const int reg8d, const int reg8s);

        /**
         * OR r8,r8
         *
         * PARAMS
         * reg8d   8 bit destination register
         * reg8s   8 bit source register
         */
        void or_r8r8(const int reg8d, const int reg8s);

        /**
         * XOR r8,r8
         *
         * PARAMS
         * reg8d   8 bit destination register
         * reg8s   8 bit source register
         */
        void xor_r8r8(const int reg8d, const int reg8s);

        /**
         * XOR r32,r32
         *
         * PARAMS
         * reg32d   32 bit destination register
         * reg32s   32 bit source register
         */
        void xor_r32r32(const int reg32d, const int reg32s);

        /**
         * XOR r16,r16
         *
         * PARAMS
         * reg16d   16 bit destination register
         * reg16s   16 bit source register
         */
        void xor_r16r16(const int reg16d, const int reg16s);

        /**
         * AND r8,r8
         *
         * PARAMS
         * reg8d   8 bit destination register
         * reg8s   8 bit source register
         */
        void and_r8r8(const int reg8d, const int reg8s);

        /**
         * AND r8,i8
         *
         * PARAMS
         * reg8d   8 bit destination register
         * imm8    8 bit immediate
         */
        void and_r8i8(const int reg8, const uint8_t imm8);

        /**
         * NOT r8
         *
         * PARAMS
         * reg8   8 bit register
         */
        void not_r8(const int reg8);

        /**
         * ADD r8,i8
         *
         * PARAMS
         * reg8   8 bit destination register
         * imm8    8 bit immediate
         */
        void add_r8i8(const int reg8, const uint8_t imm8);

        /**
         * ADD r8,r8
         *
         * PARAMS
         * reg8d   8 bit destination register
         * reg8s   8 bit source register
         */
        void add_r8r8(const int reg8d, const int reg8s);

        /**
         * ADD r16,r16
         *
         * PARAMS
         * reg16d   16 bit destination register
         * reg16s   16 bit source register
         */
        void add_r16r16(const int reg16d, const int reg16s);

        /**
         * ADD r32,r32
         *
         * PARAMS
         * reg32d   32 bit destination register
         * reg32s   32 bit source register
         */
        void add_r32r32(const int reg32d, const int reg32s);

        /**
         * ADD r16,i16
         *
         * PARAMS
         * reg16   16 bit destination register
         * imm16   16 bit immediate
         */
        void add_r16i16(const int reg16, const uint16_t imm16);

        /**
         * ADD r32,i32
         *
         * PARAMS
         * reg32   32 bit destination register
         * imm32   32 bit immediate
         */
        void add_r32i32(const int reg32, const uint32_t imm32);

        /**
         * SUB r8,r8
         *
         * PARAMS
         * reg8d   8 bit destination register
         * reg8s   8 bit source register
         */
        void sub_r8r8(const int reg8d, const int reg8s);

        /**
         * SUB r32,i32
         *
         * PARAMS
         * reg32   32 bit destination register
         * imm32   32 bit immediate
         */
        void sub_r32i32(const int reg32, const uint32_t imm32);

        /**
         * SUB r8,i8
         *
         * PARAMS
         * reg8   8 bit destination register
         * imm8   8 bit immediate
         */
        void sub_r8i8(const int reg8, const uint8_t imm8);

        /**
         * INC r8
         *
         * PARAMS
         * reg8   8 bit register
         */
        void inc_r8(const int reg8);

        /**
         * INC r16
         *
         * PARAMS
         * reg16   16 bit register
         */
        void inc_r16(const int reg16);

        /**
         * INC r32
         *
         * PARAMS
         * reg32   32 bit register
         */
        void inc_r32(const int reg32);

        /**
         * DEC r32
         *
         * PARAMS
         * reg32   32 bit register
         */
        void dec_r32(const int reg32);

        /**
         * SHL r8, 1
         *
         * PARAMS
         * reg8   8 bit register
         */
        void shl1_r8(const int reg8);

        /**
         * SHR r8, 1
         *
         * PARAMS
         * reg8   8 bit register
         */
        void shr1_r8(const int reg8);

        /**
         * SETNC r8
         *
         * PARAMS
         * reg8   8 bit register
         */
        void setnc_r8(const int reg8);

        /**
         * SETC r8
         *
         * PARAMS
         * reg8   8 bit register
         */
        void setc_r8(const int reg8);

        /**
         * PUSH i8
         *
         * PARAMS
         * imm8   8 bit immediate
         */
        void push_i8(const uint8_t imm8);

        /**
         * PUSH i16
         *
         * PARAMS
         * imm16   16 bit immediate
         */
        void push_i16(const uint16_t imm16);

        /**
         * PUSH i32
         *
         * PARAMS
         * imm32   32 bit immediate
         */
        void push_i32(const uint32_t imm32);

        /**
         * PUSH r32
         *
         * PARAMS
         * reg32   32 bit register
         */
        void push_r32(const int reg32);

        /**
         * PUSH r16
         *
         * PARAMS
         * re16   16 bit register
         */
        void push_r16(const int reg16);

        /**
         * JZ i8
         *
         * PARAMS
         * rel8   8 bit immediate, relative distance
         */
        void jz_i8(const int8_t rel8);

        /**
         * JZ i16
         *
         * PARAMS
         * rel16   16 bit immediate, relative distance
         */
        void jz_i16(const int16_t rel16);

        /**
         * JZ i32
         *
         * PARAMS
         * rel32   32 bit immediate, relative distance
         */
        void jz_i32(const int32_t rel32);

        /**
         * JMP i8
         *
         * PARAMS
         * rel8   8 bit immediate, relative distance
         */
        void jmp_i8(const int8_t rel8);

        /**
         * JMP i16
         *
         * PARAMS
         * rel16   16 bit immediate, relative distance
         */
        void jmp_i16(const int16_t rel16);

        /**
         * JMP i32
         *
         * PARAMS
         * rel32   32 bit immediate, relative distance
         */
        void jmp_i32(const int32_t rel32);

        /**
         * JC i8
         *
         * PARAMS
         * rel8   8 bit immediate, relative distance
         */
        void jc_i8(const int8_t rel8);

        /**
         * JC i16
         *
         * PARAMS
         * rel16   16 bit immediate, relative distance
         */
        void jc_i16(const int16_t rel16);

        /**
         * JC i32
         *
         * PARAMS
         * rel32   32 bit immediate, relative distance
         */
        void jc_i32(const int32_t rel32);

        /**
         * Insert JMP to label
         *
         * PARAMS
         * label    destination
         */
        void jmp(const Label_t label);

        /**
         * Insert JZ to label
         *
         * PARAMS
         * label    destination
         */
        void jz(const Label_t label);

        /**
         * Insert JNZ to label
         *
         * PARAMS
         * label    destination
         */
        void jnz(const Label_t label);

        /**
         * Insert JC to label
         *
         * PARAMS
         * label    destination
         */
        void jc(const Label_t label);

        /**
         * Insert JNC to label
         *
         * PARAMS
         * label    destination
         */
        void jnc(const Label_t label);

        /**
         * JNZ i8
         *
         * PARAMS
         * rel8   8 bit immediate, relative distance
         */
        void jnz_i8(const int8_t rel8);

        /**
         * JNZ i16
         *
         * PARAMS
         * rel16   16 bit immediate, relative distance
         */
        void jnz_i16(const int16_t rel16);

        /**
         * JNZ i32
         *
         * PARAMS
         * rel32   32 bit immediate, relative distance
         */
        void jnz_i32(const int32_t rel32);

        /**
         * JNC i8
         *
         * PARAMS
         * rel8   8 bit immediate, relative distance
         */
        void jnc_i8(const int8_t rel8);

        /**
         * JNC i16
         *
         * PARAMS
         * rel16   16 bit immediate, relative distance
         */
        void jnc_i16(const int16_t rel16);

        /**
         * JNC i32
         *
         * PARAMS
         * rel32   32 bit immediate, relative distance
         */
        void jnc_i32(const int32_t rel32);

        /**
         * RDTSC
         */
        void rdtsc();

        /**
         * MUL r8
         * AX = AL * r8
         *
         * PARAMS
         * reg8     8 bit register
         */
        void mul_r8(const int reg8);

        /**
         * MUL m32
         * EDX:EAX = EAX * m32
         *
         * PARAMS
         * reg32     32 bit memory pointer
         */
        void mul_m32(const int reg32);

        /**
         * XCHG r8,r8
         *
         * PARAMS
         * reg8d     8 bit destination register
         * reg8s     8 bit source register
         */
        void xchg_r8r8(const int reg8d, int reg8s);

        /**
         * DIV r8
         * AL = AX / r8, AH = reminder
         *
         * PARAMS
         * reg8     8 bit register
         */
        void div_r8(const int reg8);

        /**
         * SHR r8,i8
         *
         * PARAMS
         * reg8     8 bit destination register
         * imm8     8 bit immediate
         */
        void shr_r8i8(const int reg8, const uint8_t imm8);

        /**
         * SHR r32,i8
         *
         * PARAMS
         * reg8     32 bit destination register
         * imm8     8 bit immediate
         */
        void shr_r32i8(const int reg32, const uint8_t imm8);

        /**
         * SHR r16,i8
         *
         * PARAMS
         * reg16     16 bit destination register
         * imm8      8 bit immediate
         */
        void shr_r16i8(const int reg16, const uint8_t imm8);

        /**
         * SHL r8,i8
         *
         * PARAMS
         * reg8      8 bit destination register
         * imm8      8 bit immediate
         */
        void shl_r8i8(const int reg8, const uint8_t imm8);

        /**
         * SHL r16,i8
         *
         * PARAMS
         * reg16     16 bit destination register
         * imm8      8 bit immediate
         */
        void shl_r16i8(const int reg16, const uint8_t imm8);

        /**
         * SHL r32,i8
         *
         * PARAMS
         * reg32     32 bit destination register
         * imm8      8 bit immediate
         */
        void shl_r32i8(const int reg32, const uint8_t imm8);

        /**
         * MOV m8,i8
         *
         * PARAMS
         * reg32     32 bit destination register
         * imm8      8 bit immediate
         */
        void mov_m8i8(const int reg32, const uint8_t imm8);

        /**
         * CMP r32,i32
         *
         * PARAMS
         * reg32     32 bit register
         * imm32     32 bit immediate
         */
        void cmp_r32i32(const int reg32, const uint32_t imm32);

        /**
         * CMP r16,i16
         *
         * PARAMS
         * reg16     16 bit register
         * imm16     16 bit immediate
         */
        void cmp_r16i16(const int reg16, const uint16_t imm16);

        /**
         * XOR m8,i8
         *
         * PARAMS
         * reg32     32 bit memory pointer
         * imm8      8 bit immediate
         */
        void xor_m8i8(const int reg32, const uint8_t imm8);
        /**
         * BSWAP r32
         *
         * PARAMS
         * reg32     32 bit register
         */
        void bswap_r32(const int reg32);

        /**
         * AND r16,i16
         *
         * PARAMS
         * reg16     16 bit register
         * imm16     16 bit immediate
         */
        void and_r16i16(const int reg16, const uint16_t imm16);

        /**
         * AND r32,i32
         *
         * PARAMS
         * reg32     32 bit register
         * imm32     32 bit immediate
         */
        void and_r32i32(const int reg32, const uint32_t imm32);

        /**
         * TEST r32,r32
         *
         * PARAMS
         * reg32d    32 bit destination register
         * reg32s    32 bit source register
         */
        void test_r32r32(const int reg32d, const int reg32s);

        /**
         * TEST r16,r16
         *
         * PARAMS
         * reg16d    16 bit destination register
         * reg16s    16 bit source register
         */
        void test_r16r16(const int reg16d, const int reg16s);

        /**
         * TEST r8,r8
         *
         * PARAMS
         * reg8d    8 bit destination register
         * reg8s    8 bit source register
         */
        void test_r8r8(const int reg8d, const int reg8s);

};



#endif //_CODEGENERATOR_X86_H_
