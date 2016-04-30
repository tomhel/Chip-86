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
     *   To be decided... possibly GPL
     *
     ********************************************************/

#include <cstdlib>
#include <cstring>

#include "CodeGenerator.h"

/**
 * Insert all jumps into the code.
 */
void CodeGenerator::insertJumps()
{

    const int tmp = mIndex;

    while(!mJumps.empty())
    {
        const Jump *const pJmp = mJumps.back();
        const int id = pJmp->label;

        if(mLabels[id]->inserted)
        {
            mIndex = pJmp->index;
            const int32_t rel = mLabels[id]->index - mIndex;
            (this->*pJmp->pfnJmp)(rel);
        }

        mJumps.pop_back();
        delete pJmp;
    }

    mIndex = tmp;
}

/**
 * Jump If Not Zero.
 * Insert a jump into the code
 *
 * PARAMS
 *   rel    distance between label and jump
 */
void CodeGenerator::insert_jnz(const int32_t rel)
{

    if((rel - 2) >= CG_INT8_MIN && (rel - 2) <= CG_INT8_MAX)
        jnz_i8(rel - 2);
    else
        jnz_i32(rel - 6);
}

/**
 * Jump If Zero.
 * Insert a jump into the code
 *
 * PARAMS
 *   rel    distance between label and jump
 */
void CodeGenerator::insert_jz(const int32_t rel)
{
    if((rel - 2) >= CG_INT8_MIN && (rel - 2) <= CG_INT8_MAX)
        jz_i8(rel - 2);
    else
        jz_i32(rel - 6);
}

/**
 * Jump If Not Carry.
 * Insert a jump into the code
 *
 * PARAMS
 *   rel    distance between label and jump
 */
void CodeGenerator::insert_jnc(const int32_t rel)
{
    if((rel - 2) >= CG_INT8_MIN && (rel - 2) <= CG_INT8_MAX)
        jnc_i8(rel - 2);
    else
        jnc_i32(rel - 6);
}

/**
 * Jump If Carry.
 * Insert a jump into the code
 *
 * PARAMS
 *   rel    distance between label and jump
 */
void CodeGenerator::insert_jc(const int32_t rel)
{
    if((rel - 2) >= CG_INT8_MIN && (rel - 2) <= CG_INT8_MAX)
        jc_i8(rel - 2);
    else
        jc_i32(rel - 6);
}

/**
 * Jump.
 * Insert a jump into the code
 *
 * PARAMS
 *   rel    distance between label and jump
 */
void CodeGenerator::insert_jmp(const int32_t rel)
{
    if((rel - 2) >= CG_INT8_MIN && (rel - 2) <= CG_INT8_MAX)
        jmp_i8(rel - 2);
    else
        jmp_i32(rel - 6);
}

/**
 * Insert a label at current position
 *
 * PARAMS
 * id    Label id
 */
void CodeGenerator::insertLabel(const Label_t id)
{
    mLabels[id]->inserted = true;
    mLabels[id]->index = mIndex;
}

/**
 * Creates a new label
 *
 * RETURNS
 * a unique label id
 */
Label_t CodeGenerator::newLabel()
{
    mLabels.push_back(new Label);
    return mLabels.size() - 1;
}

/**
 * Copies the code to the heap and return a pointer to it
 * The code is guaranteed to be aligned
 * RETURNS
 * void pointer to generated code
 */
void* CodeGenerator::getAlignedCodePointer(void **const pBlock)
{
    void *pCode;

    //om det finns kod
    if(mIndex > 0)
    {
        //beräkna och lägg in alla hopp
        insertJumps();
        //allokera nytt minne och kopiera
        *pBlock = malloc(mIndex + CG_ALIGNMENT);
        const uintptr_t blockAddr = (uintptr_t)*pBlock;
        pCode = (void*)(blockAddr + (CG_ALIGNMENT - (blockAddr & (CG_ALIGNMENT - 1))));
        //pCode = *pBlock;
        memcpy(pCode, mMachineCode, mIndex);
        reset();
    }
    else
    {
        *pBlock = NULL;
        pCode = NULL;
    }

    return pCode;
}

/**
 * Copies the code to the heap and return a pointer to it
 * The code is NOT guaranteed to be aligned
 *
 * RETURNS
 * void pointer to generated code
 */
void* CodeGenerator::getCodePointer()
{
    void *pCode;

    //om det finns kod
    if(mIndex > 0)
    {
        //beräkna och lägg in alla hopp
        insertJumps();
        //allokera nytt minne och kopiera
        pCode = malloc(mIndex);
        memcpy(pCode, mMachineCode, mIndex);
        reset();
    }
    else
        pCode = NULL;

    return pCode;
}

/**
 * Force alignment of current position (inserts nops)
 */
void CodeGenerator::align16()
{
    while(((mIndex + CG_ALIGNMENT) & (CG_ALIGNMENT - 1)) > 0)
        nop();
}

/**
 * Request alignment of current position (inserts nops)
 */
void CodeGenerator::align()
{
    if(CG_ALIGNMENT - ((mIndex + CG_ALIGNMENT) & (CG_ALIGNMENT - 1)) < 8)
        align16();
}

/**
 * Reset
 */
void CodeGenerator::reset()
{
    mIndex = 0;
    destroy();
}

/**
 * Destroy
 */
void CodeGenerator::destroy()
{
    while(!mJumps.empty())
    {
        const Jump *const p = mJumps.back();
        mJumps.pop_back();
        delete p;
    }

    while(!mLabels.empty())
    {
        const Label *const p = mLabels.back();
        mLabels.pop_back();
        delete p;
    }
}

/**
 * Constructor
 */
CodeGenerator::CodeGenerator()
{
    reset();
}

/**
 * Destructor
 */
CodeGenerator::~CodeGenerator()
{
    destroy();
}

/**
 * MOV r32,imm32
 *
 * PARAMS
 * reg32   32 bit registernumber
 * imm32   32 bit immediate
 */
void CodeGenerator::mov_r32i32(const int reg32, const uint32_t imm32)
{
    //B8+ rd
    //MOV r32,imm32
    //Move imm32 to r32
    mMachineCode[mIndex++] = 0xB8+reg32;
    mMachineCode[mIndex++] = imm32&0xFF;
    mMachineCode[mIndex++] = ((imm32>>8)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>16)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>24)&0xFF);
}

/**
 * MOV m32,imm32
 *
 * PARAMS
 * reg32   32 bit register
 * imm32   32 bit immediate
 */
void CodeGenerator::mov_m32i32(const int reg32, const uint32_t imm32)
{
    //C7 /0
    //MOV r/m32,imm32
    //Move imm32 to r/m32
    mMachineCode[mIndex++] = 0xC7;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM, 0x0, reg32);
    mMachineCode[mIndex++] = imm32&0xFF;
    mMachineCode[mIndex++] = ((imm32>>8)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>16)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>24)&0xFF);
}

/**
 * MOV m32,imm32
 *
 * PARAMS
 * reg32    32 bit register
 * imm32    32 bit immediate
 * disp8    8 bit memory displacement
 */
void CodeGenerator::mov_m32i32_d8(const int reg32, const uint32_t imm32, const uint8_t disp8)
{
    //C7 /0
    //MOV r/m32,imm32
    //Move imm32 to r/m32
    mMachineCode[mIndex++] = 0xC7;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM_DISPB, 0x0, reg32);
    mMachineCode[mIndex++] = disp8;
    mMachineCode[mIndex++] = imm32&0xFF;
    mMachineCode[mIndex++] = ((imm32>>8)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>16)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>24)&0xFF);
}

/**
 * MOV r16,imm16
 *
 * PARAMS
 * reg16    16 bit register
 * imm16    16 bit immediate
 */
void CodeGenerator::mov_r16i16(const int reg16, const uint16_t imm16)
{
    //B8+ rw
    //MOV r16,imm16
    //Move imm16 to r16
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    mMachineCode[mIndex++] = 0xB8+reg16;
    mMachineCode[mIndex++] = imm16&0xFF;
    mMachineCode[mIndex++] = ((imm16>>8)&0xFF);
}

/**
 * MOV r32,r32
 *
 * PARAMS
 * reg32d    32 bit destination register
 * reg32s    32 bit source register
 */
void CodeGenerator::mov_r32r32(const int reg32d, const int reg32s)
{
    //8B /r
    //MOV r32,r/m32
    //Move r/m32 to r32
    mMachineCode[mIndex++] = 0x8B;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg32d, reg32s);

}

/**
 * MOV r16,r16
 *
 * PARAMS
 * reg16d    16 bit destination register
 * reg16s    16 bit source register
 */
void CodeGenerator::mov_r16r16(const int reg16d, const int reg16s)
{
    //8B /r
    //MOV r16,r/m16
    //Move r/m16 to r16
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    mov_r32r32(reg16d, reg16s);
}

/**
 * MOVZX r32,r16
 *
 * PARAMS
 * reg32    32 bit destination register
 * reg16    16 bit source register
 */
void CodeGenerator::movzx_r32r16(const int reg32, const int reg16)
{
    //0F B7 /r
    //MOVZX r32,r/m16
    //Move word to doubleword, zero-extension
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0xB7;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg32, reg16);
}

/**
 * MOVZX r32,m16
 *
 * PARAMS
 * reg32d    32 bit destination register
 * reg32s    32 bit memory pointer
 */
void CodeGenerator::movzx_r32m16(const int reg32d, const int reg32s)
{
    //0F B7 /r
    //MOVZX r32,r/m16
    //Move word to doubleword, zero-extension
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0xB7;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM, reg32d, reg32s);
}

/**
 * NOP
 */
void CodeGenerator::nop()
{
    //90
    //NOP
    //No operation
    mMachineCode[mIndex++] = 0x90;
}

/**
 * MOV r8,i8
 *
 * PARAMS
 * reg8    8 bit destination register
 * imm8    8 bit immediate
 */
void CodeGenerator::mov_r8i8(const int reg8, const uint8_t imm8)
{
    //B0+ rb
    //MOV r8,imm8
    //Move imm8 to r8
    mMachineCode[mIndex++] = 0xB0+reg8;
    mMachineCode[mIndex++] = imm8;
}

/**
 * MOV r8,r8
 *
 * PARAMS
 * reg8d    8 bit destination register
 * reg8s    8 bit source register
 */
void CodeGenerator::mov_r8r8(const int reg8d, const int reg8s)
{
    //88 /r
    //MOV r/m8,r8
    //Move r8 to r/m8
    mMachineCode[mIndex++] = 0x88;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg8s, reg8d);
}

/**
 * MOV r8,m8
 *
 * PARAMS
 * reg8d     8 bit destination register
 * reg32s    32 bit memory pointer
 */
void CodeGenerator::mov_r8m8(const int reg8d, const int reg32s)
{
    //8A /r
    //MOV r8,r/m8
    //Move r/m8 to r8
    mMachineCode[mIndex++] = 0x8A;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM, reg8d, reg32s);
}

/**
 * MOV r8,m8
 *
 * PARAMS
 * reg8d     8 bit destination register
 * reg32s    32 bit memory pointer
 * disp8     8 bit memory dispacement
 */
void CodeGenerator::mov_r8m8_d8(const int reg8d, const int reg32s, const uint8_t disp8)
{
    //8A /r
    //MOV r8,r/m8
    //Move r/m8 to r8
    mMachineCode[mIndex++] = 0x8A;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM_DISPB, reg8d, reg32s);
    mMachineCode[mIndex++] = disp8;
}

/**
 * MOV r32,m32
 *
 * PARAMS
 * reg32d    32 bit destination register
 * reg32s    32 bit memory pointer
 */
void CodeGenerator::mov_r32m32(const int reg32d, const int reg32s)
{
    //8B /r
    //MOV r32,r/m32
    //Move r/m32 to r32
    mMachineCode[mIndex++] = 0x8B;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM, reg32d, reg32s);
}

/**
 * MOV r16,m16
 *
 * PARAMS
 * reg16d    16 bit destination register
 * reg32s    32 bit memory pointer
 */
void CodeGenerator::mov_r16m16(const int reg16d, const int reg32s)
{
    //8B /r
    //MOV r16,r/m16
    //Move r/m16 to r16
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    mov_r32m32(reg16d, reg32s);
}

/**
 * MOV m8,r8
 *
 * PARAMS
 * reg32d    32 bit memory pointer
 * reg32s    8 bit source register
 */
void CodeGenerator::mov_m8r8(const int reg32d, const int reg8s)
{
    //88 /r
    //MOV r/m8,r8
    //Move r8 to r/m8
    mMachineCode[mIndex++] = 0x88;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM, reg8s, reg32d);
}

/**
 * MOV m8,r8
 *
 * PARAMS
 * reg32d    32 bit memory pointer
 * reg8s     8 bit source register
 * disp8     8 bit memory displacement
 */
void CodeGenerator::mov_m8r8_d8(const int reg32d, const int reg8s, const uint8_t disp8)
{
    //88 /r
    //MOV r/m8,r8
    //Move r8 to r/m8
    mMachineCode[mIndex++] = 0x88;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM_DISPB, reg8s, reg32d);
    mMachineCode[mIndex++] = disp8;
}

/**
 * MOV m32,r32
 *
 * PARAMS
 * reg32d    32 bit memory pointer
 * reg32s    32 bit source register
 */
void CodeGenerator::mov_m32r32(const int reg32d, const int reg32s)
{
    //89 /r
    //MOV r/m32,r32
    //Move r32 to r/m32
    mMachineCode[mIndex++] = 0x89;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM, reg32s, reg32d);
}

/**
 * MOV m16,r16
 *
 * PARAMS
 * reg32d    32 bit memory pointer
 * reg16s    16 bit source register
 */
void CodeGenerator::mov_m16r16(const int reg32d, const int reg16s)
{
    //89 /r
    //MOV r/m16,r16
    //Move r16 to r/m16
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    mov_m32r32(reg32d, reg16s);
}

/**
 * MOVZX m32,r8
 *
 * PARAMS
 * reg32    32 bit destination register
 * reg8     8 bit source register
 */
void CodeGenerator::movzx_r32r8(const int reg32, const int reg8)
{
    //0F B6 /r
    //MOVZX r32,r/m8
    //Move byte to doubleword, zero-extension
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0xB6;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg32, reg8);
}

/**
 * MOVZX r32,m8
 *
 * PARAMS
 * reg32d    32 bit destination register
 * reg32s    32 bit memory pointer
 */
void CodeGenerator::movzx_r32m8(const int reg32d, const int reg32s)
{
    //0F B6 /r
    //MOVZX r32,r/m8
    //Move byte to doubleword, zero-extension
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0xB6;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM, reg32d, reg32s);
}

/**
 * MOVZX r16,r8
 *
 * PARAMS
 * reg16    16 bit destination register
 * reg8     8 bit source register
 */
void CodeGenerator::movzx_r16r8(const int reg16, const int reg8)
{
    //0F B6 /r
    //MOVZX r16,r/m8
    //Move byte to word with zero-extension
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    movzx_r32r8(reg16,reg8);
}

/**
 * RET
 */
void CodeGenerator::ret()
{
    //C3
    //RET
    //Near return to calling procedure
    mMachineCode[mIndex++] = 0xC3;
}

/**
 * PUSHAD
 */
void CodeGenerator::pushad()
{
    //60
    //PUSHAD
    //Push EAX, ECX, EDX, EBX, original ESP EBP ESI, and EDI
    mMachineCode[mIndex++] = 0x60;
}

/**
 * POPAD
 */
void CodeGenerator::popad()
{
    //61
    //POPAD
    //Pop EDI, ESI, EBP EBX, EDX, ECX, and EAX
    mMachineCode[mIndex++] = 0x61;
}

/**
 * POP r32
 *
 * PARAMS
 * reg32    32 bit register
 */
void CodeGenerator::pop_r32(const int reg32)
{
    //58+ rd
    //POP r32
    //Pop top of stack into r32; increment stack pointer
    mMachineCode[mIndex++] = 0x58+reg32;
}

/**
 * POP r16
 *
 * PARAMS
 * reg16    16 bit register
 */
void CodeGenerator::pop_r16(const int reg16)
{
    //58+ rw
    //POP r16
    //Pop top of stack into r16; increment stack pointer
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    pop_r32(reg16);
}

/**
 * CALL r32
 *
 * PARAMS
 * reg32    32 bit memory pointer
 */
void CodeGenerator::call_r32(const int reg32)
{
    //FF /2
    //call r/m32
    //Call near, absolute indirect, address given in r/m32
    mMachineCode[mIndex++] = 0xFF;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x2, reg32);
}

/**
 * CMP r8,i8
 *
 * PARAMS
 * reg8    8 bit register
 * imm8    8 bit immediate
 */
void CodeGenerator::cmp_r8i8(const int reg8, const uint8_t imm8)
{
    //3C ib
    //CMP AL, imm8
    //Compare imm8 with AL

    //80 /7 ib
    //CMP r/m8, imm8
    //Compare imm8 with r/m8
    if(reg8 == X86_REG_AL)
       mMachineCode[mIndex++] = 0x3C;
    else
    {
        mMachineCode[mIndex++] = 0x80;
        mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x7, reg8);
    }

    mMachineCode[mIndex++] = imm8;
}

/**
 * OR r8,i8
 *
 * PARAMS
 * reg8    8 bit register
 * imm8    8 bit immediate
 */
void CodeGenerator::or_r8i8(const int reg8, const uint8_t imm8)
{
    //0C ib
    //OR AL,imm8
    //AL OR imm8

    //80 /1 ib
    //OR r/m8,imm8
    //r/m8 OR imm8

    if(reg8 == X86_REG_AL)
       mMachineCode[mIndex++] = 0x0C;
    else
    {
        mMachineCode[mIndex++] = 0x80;
        mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x1, reg8);
    }

    mMachineCode[mIndex++] = imm8;
}

/**
 * CMP m8,i8
 *
 * PARAMS
 * reg32   32 bit memory pointer
 * imm8    8 bit immediate
 */
void CodeGenerator::cmp_m8i8(const int reg32, const uint8_t imm8)
{
    //80 /7 ib
    //CMP r/m8, imm8
    //Compare imm8 with r/m8
    mMachineCode[mIndex++] = 0x80;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM, 0x7, reg32);
    mMachineCode[mIndex++] = imm8;
}

/**
 * CMP m8,i8
 *
 * PARAMS
 * reg32   32 bit memory pointer
 * imm8    8 bit immediate
 * disp8   8 bit memory displacement
 */
void CodeGenerator::cmp_m8i8_d8(const int reg32, const uint8_t imm8, const uint8_t disp8)
{
    //80 /7 ib
    //CMP r/m8, imm8
    //Compare imm8 with r/m8
    mMachineCode[mIndex++] = 0x80;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM_DISPB, 0x7, reg32);
    mMachineCode[mIndex++] = disp8;
    mMachineCode[mIndex++] = imm8;
}

/**
 * CMP r8,r8
 *
 * PARAMS
 * reg8d   8 bit register
 * reg8s   8 bit register
 */
void CodeGenerator::cmp_r8r8(const int reg8d, const int reg8s)
{
    //38 /r
    //CMP r/m8,r8
    //Compare r8 with r/m8
    mMachineCode[mIndex++] = 0x38;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg8s, reg8d);
}

/**
 * OR r8,r8
 *
 * PARAMS
 * reg8d   8 bit destination register
 * reg8s   8 bit source register
 */
void CodeGenerator::or_r8r8(const int reg8d, const int reg8s)
{
    //08 /r
    //OR r/m8, r8
    //r/m8 OR r8
    mMachineCode[mIndex++] = 0x08;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg8s, reg8d);
}

/**
 * XOR r8,r8
 *
 * PARAMS
 * reg8d   8 bit destination register
 * reg8s   8 bit source register
 */
void CodeGenerator::xor_r8r8(const int reg8d, const int reg8s)
{
    //30 /r
    //XOR r/m8, r8
    //r/m8 XOR r8
    mMachineCode[mIndex++] = 0x30;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg8s, reg8d);
}

/**
 * XOR r32,r32
 *
 * PARAMS
 * reg32d   32 bit destination register
 * reg32s   32 bit source register
 */
void CodeGenerator::xor_r32r32(const int reg32d, const int reg32s)
{
    //31 /r
    //XOR r/m32, r32
    //r/m32 XOR r32
    mMachineCode[mIndex++] = 0x31;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg32s, reg32d);
}

/**
 * XOR r16,r16
 *
 * PARAMS
 * reg16d   16 bit destination register
 * reg16s   16 bit source register
 */
void CodeGenerator::xor_r16r16(const int reg16d, const int reg16s)
{
    //31 /r
    //XOR r/m16, r16
    //r/m16 XOR r16
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    xor_r32r32(reg16d, reg16s);
}

/**
 * AND r8,r8
 *
 * PARAMS
 * reg8d   8 bit destination register
 * reg8s   8 bit source register
 */
void CodeGenerator::and_r8r8(const int reg8d, const int reg8s)
{
    //20 /r
    //AND r/m8, r8
    //r/m8 AND r8
    mMachineCode[mIndex++] = 0x20;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg8s, reg8d);
}

/**
 * AND r8,i8
 *
 * PARAMS
 * reg8d   8 bit destination register
 * imm8    8 bit immediate
 */
void CodeGenerator::and_r8i8(const int reg8, const uint8_t imm8)
{
    //24 ib
    //AND AL,imm8
    //AL AND imm8

    //80 /4 ib
    //AND r/m8,imm8
    //r/m8 AND imm8
    if(reg8 == X86_REG_AL)
        mMachineCode[mIndex++] = 0x24;
    else
    {
        mMachineCode[mIndex++] = 0x80;
        mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x4, reg8);
    }

    mMachineCode[mIndex++] = imm8;
}

/**
 * NOT r8
 *
 * PARAMS
 * reg8   8 bit register
 */
void CodeGenerator::not_r8(const int reg8)
{
    //F6 /2
    //NOT r/m8
    //Reverse each bit of r/m8
    mMachineCode[mIndex++] = 0xF6;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x2, reg8);
}

/**
 * ADD r8,i8
 *
 * PARAMS
 * reg8   8 bit destination register
 * imm8    8 bit immediate
 */
void CodeGenerator::add_r8i8(const int reg8, const uint8_t imm8)
{
    //04 ib
    //ADD AL,imm8
    //Add imm8 to AL

    //80 /0 ib
    //ADD r/m8,imm8
    //Add imm8 to r/m8

    if(reg8 == X86_REG_AL)
        mMachineCode[mIndex++] = 0x04;
    else
    {
        mMachineCode[mIndex++] = 0x80;
        mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x0, reg8);
    }

    mMachineCode[mIndex++] = imm8;
}

/**
 * ADD r8,r8
 *
 * PARAMS
 * reg8d   8 bit destination register
 * reg8s   8 bit source register
 */
void CodeGenerator::add_r8r8(const int reg8d, const int reg8s)
{
    //00 /r
    //ADD r/m8,r8
    //Add r8 to r/m8
    mMachineCode[mIndex++] = 0x00;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg8s, reg8d);
}

/**
 * ADD r16,r16
 *
 * PARAMS
 * reg16d   16 bit destination register
 * reg16s   16 bit source register
 */
void CodeGenerator::add_r16r16(const int reg16d, const int reg16s)
{
    //01 /r
    //ADD r/m16,r16
    //Add r16 to r/m16
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    add_r32r32(reg16d, reg16s);
}

/**
 * ADD r32,r32
 *
 * PARAMS
 * reg32d   32 bit destination register
 * reg32s   32 bit source register
 */
void CodeGenerator::add_r32r32(const int reg32d, const int reg32s)
{
    //01 /r
    //ADD r/m32,r32
    //Add r32 to r/m32
    mMachineCode[mIndex++] = 0x01;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg32s, reg32d);
}

/**
 * ADD r16,i16
 *
 * PARAMS
 * reg16   16 bit destination register
 * imm16   16 bit immediate
 */
void CodeGenerator::add_r16i16(const int reg16, const uint16_t imm16)
{
    //05 iw
    //ADD AX,imm16
    //Add imm16 to AX

    //81 /0 iw
    //ADD r/m16,imm16
    //Add imm16 to r/m16
    mMachineCode[mIndex++] = X86_PREFIX_REG16;

    if(reg16 == X86_REG_AX)
        mMachineCode[mIndex++] = 0x05;
    else
    {
        mMachineCode[mIndex++] = 0x81;
        mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x0, reg16);
    }

    mMachineCode[mIndex++] = imm16&0xFF;
    mMachineCode[mIndex++] = ((imm16>>8)&0xFF);
}

/**
 * ADD r32,i32
 *
 * PARAMS
 * reg32   32 bit destination register
 * imm32   32 bit immediate
 */
void CodeGenerator::add_r32i32(const int reg32, const uint32_t imm32)
{
    //05 id
    //ADD EAX,imm32
    //Add imm32 to EAX

    //81 /0 id
    //ADD r/m32, imm32
    //imm32 Add imm32 to r/m32

    if(reg32 == X86_REG_EAX)
        mMachineCode[mIndex++] = 0x05;
    else
    {
        mMachineCode[mIndex++] = 0x81;
        mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x0, reg32);
    }

    mMachineCode[mIndex++] = imm32&0xFF;
    mMachineCode[mIndex++] = ((imm32>>8)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>16)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>24)&0xFF);
}

/**
 * SUB r8,r8
 *
 * PARAMS
 * reg8d   8 bit destination register
 * reg8s   8 bit source register
 */
void CodeGenerator::sub_r8r8(const int reg8d, const int reg8s)
{
    //28 /r
    //SUB r/m8,r8
    //Subtract r8 from r/m8
    mMachineCode[mIndex++] = 0x28;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg8s, reg8d);
}

/**
 * SUB r32,i32
 *
 * PARAMS
 * reg32   32 bit destination register
 * imm32   32 bit immediate
 */
void CodeGenerator::sub_r32i32(const int reg32, const uint32_t imm32)
{
    //2D id
    //SUB EAX,imm32
    //Subtract imm32 from EAX

    //81 /5 id
    //SUB r/m32,imm32
    //Subtract imm32 from r/m32

    if(reg32 == X86_REG_EAX)
        mMachineCode[mIndex++] = 0x2D;
    else
    {
        mMachineCode[mIndex++] = 0x81;
        mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x5, reg32);
    }

    mMachineCode[mIndex++] = imm32&0xFF;
    mMachineCode[mIndex++] = ((imm32>>8)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>16)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>24)&0xFF);
}

/**
 * SUB r8,i8
 *
 * PARAMS
 * reg8   8 bit destination register
 * imm8   8 bit immediate
 */
void CodeGenerator::sub_r8i8(const int reg8, const uint8_t imm8)
{
    //2C ib
    //SUB AL,imm8
    //Subtract imm8 from AL

    //80 /5 ib
    //SUB r/m8,imm8
    //Subtract imm8 from r/m8

    if(reg8 == X86_REG_AL)
        mMachineCode[mIndex++] = 0x2C;
    else
    {
        mMachineCode[mIndex++] = 0x80;
        mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x5, reg8);
    }

    mMachineCode[mIndex++] = imm8;
}

/**
 * INC r8
 *
 * PARAMS
 * reg8   8 bit register
 */
void CodeGenerator::inc_r8(const int reg8)
{
    //FE /0
    //INC r/m8
    //Increment r/m byte by 1
    mMachineCode[mIndex++] = 0xFE;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x0, reg8);
}

/**
 * INC r16
 *
 * PARAMS
 * reg16   16 bit register
 */
void CodeGenerator::inc_r16(const int reg16)
{
    //40+ rw
    //INC r16
    //Increment doubleword register by 1
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    inc_r32(reg16);
}

/**
 * INC r32
 *
 * PARAMS
 * reg32   32 bit register
 */
void CodeGenerator::inc_r32(const int reg32)
{
    //40+ rd
    //INC r32
    //Increment doubleword register by 1
    mMachineCode[mIndex++] = 0x40+reg32;
}

/**
 * DEC r32
 *
 * PARAMS
 * reg32   32 bit register
 */
void CodeGenerator::dec_r32(const int reg32)
{
    //48+rd
    //DEC r32
    //Decrement r32 by 1
    mMachineCode[mIndex++] = 0x48+reg32;
}

/**
 * SHL r8, 1
 *
 * PARAMS
 * reg8   8 bit register
 */
void CodeGenerator::shl1_r8(const int reg8)
{
    //D0 /4
    //SHL r/m8,1
    //Multiply r/m8 by 2, once
    mMachineCode[mIndex++] = 0xD0;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x4, reg8);
}

/**
 * SHR r8, 1
 *
 * PARAMS
 * reg8   8 bit register
 */
void CodeGenerator::shr1_r8(const int reg8)
{
    //D0 /5
    //SHR r/m8,1
    //Unsigned divide r/m8 by 2, once
    mMachineCode[mIndex++] = 0xD0;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x5, reg8);
}

/**
 * SETNC r8
 *
 * PARAMS
 * reg8   8 bit register
 */
void CodeGenerator::setnc_r8(const int reg8)
{
    //0F 93 /0
    //SETNC r/m8
    //Set byte if not carry (CF=0)
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0x93;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x0, reg8);
}

/**
 * SETC r8
 *
 * PARAMS
 * reg8   8 bit register
 */
void CodeGenerator::setc_r8(const int reg8)
{
    //0F 92 /0
    //SETC r/m8
    //Set if carry (CF=1)
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0x92;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x0, reg8);
}

/**
 * PUSH i8
 *
 * PARAMS
 * imm8   8 bit immediate
 */
void CodeGenerator::push_i8(const uint8_t imm8)
{
    //6A
    //PUSH imm8
    //Push imm8
    mMachineCode[mIndex++] = 0x6A;
    mMachineCode[mIndex++] = imm8;
}

/**
 * PUSH i16
 *
 * PARAMS
 * imm16   16 bit immediate
 */
void CodeGenerator::push_i16(const uint16_t imm16)
{
    //68
    //PUSH imm16
    //Push imm16
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    mMachineCode[mIndex++] = 0x68;
    mMachineCode[mIndex++] = imm16&0xFF;
    mMachineCode[mIndex++] = ((imm16>>8)&0xFF);
}

/**
 * PUSH i32
 *
 * PARAMS
 * imm32   32 bit immediate
 */
void CodeGenerator::push_i32(const uint32_t imm32)
{
    //68
    //PUSH imm32
    //Push imm32
    mMachineCode[mIndex++] = 0x68;
    mMachineCode[mIndex++] = imm32&0xFF;
    mMachineCode[mIndex++] = ((imm32>>8)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>16)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>24)&0xFF);
}

/**
 * PUSH r32
 *
 * PARAMS
 * reg32   32 bit register
 */
void CodeGenerator::push_r32(const int reg32)
{
    //50+rd
    //PUSH r32
    //Push r32
    mMachineCode[mIndex++] = 0x50+reg32;
}

/**
 * PUSH r16
 *
 * PARAMS
 * re16   16 bit register
 */
void CodeGenerator::push_r16(const int reg16)
{
    //50+rw
    //PUSH r16
    //Push r16
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    push_r32(reg16);
}

/**
 * JZ i8
 *
 * PARAMS
 * rel8   8 bit immediate, relative distance
 */
void CodeGenerator::jz_i8(const int8_t rel8)
{
    //74 cb
    //JZ rel8
    //Jump short if zero (ZF = 1)
    mMachineCode[mIndex++] = 0x74;
    mMachineCode[mIndex++] = rel8;
}

/**
 * JZ i16
 *
 * PARAMS
 * rel16   16 bit immediate, relative distance
 */
void CodeGenerator::jz_i16(const int16_t rel16)
{
    //0F 84 cw/cd
    //JZ rel16/32
    //Jump near if 0 (ZF=1)
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0x84;
    mMachineCode[mIndex++] = rel16&0xFF;
    mMachineCode[mIndex++] = ((rel16>>8)&0xFF);
}

/**
 * JZ i32
 *
 * PARAMS
 * rel32   32 bit immediate, relative distance
 */
void CodeGenerator::jz_i32(const int32_t rel32)
{
    //0F 84 cw/cd
    //JZ rel16/32
    //Jump near if 0 (ZF=1)
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0x84;
    mMachineCode[mIndex++] = rel32&0xFF;
    mMachineCode[mIndex++] = ((rel32>>8)&0xFF);
    mMachineCode[mIndex++] = ((rel32>>16)&0xFF);
    mMachineCode[mIndex++] = ((rel32>>24)&0xFF);
}

/**
 * JMP i8
 *
 * PARAMS
 * rel8   8 bit immediate, relative distance
 */
void CodeGenerator::jmp_i8(const int8_t rel8)
{
    //EB cb
    //JMP rel8
    //Jump short, relative, displacement relative to next instruction
    mMachineCode[mIndex++] = 0xEB;
    mMachineCode[mIndex++] = rel8;
}

/**
 * JMP i16
 *
 * PARAMS
 * rel16   16 bit immediate, relative distance
 */
void CodeGenerator::jmp_i16(const int16_t rel16)
{
    //E9 cw
    //JMP rel16
    //Jump near, relative, displacement relative to next instruction
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    mMachineCode[mIndex++] = 0xE9;
    mMachineCode[mIndex++] = rel16&0xFF;
    mMachineCode[mIndex++] = ((rel16>>8)&0xFF);
}

/**
 * JMP i32
 *
 * PARAMS
 * rel32   32 bit immediate, relative distance
 */
void CodeGenerator::jmp_i32(const int32_t rel32)
{
    //E9 cd
    //JMP rel32
    //Jump near, relative, displacement relative to next instruction
    mMachineCode[mIndex++] = 0xE9;
    mMachineCode[mIndex++] = rel32&0xFF;
    mMachineCode[mIndex++] = ((rel32>>8)&0xFF);
    mMachineCode[mIndex++] = ((rel32>>16)&0xFF);
    mMachineCode[mIndex++] = ((rel32>>24)&0xFF);
}

/**
 * JC i8
 *
 * PARAMS
 * rel8   8 bit immediate, relative distance
 */
void CodeGenerator::jc_i8(const int8_t rel8)
{
	//72 cb
	//JC rel8
	//Jump short if carry (CF=1)
    mMachineCode[mIndex++] = 0x72;
    mMachineCode[mIndex++] = rel8;
}

/**
 * JC i16
 *
 * PARAMS
 * rel16   16 bit immediate, relative distance
 */
void CodeGenerator::jc_i16(const int16_t rel16)
{
	//0F 82 cw/cd
	//JC rel16/32
	//Jump near if carry (CF=1)
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0x82;
    mMachineCode[mIndex++] = rel16&0xFF;
    mMachineCode[mIndex++] = ((rel16>>8)&0xFF);
}

/**
 * JC i32
 *
 * PARAMS
 * rel32   32 bit immediate, relative distance
 */
void CodeGenerator::jc_i32(const int32_t rel32)
{
	//0F 82 cw/cd
	//JC rel16/32
	//Jump near if carry (CF=1)
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0x82;
    mMachineCode[mIndex++] = rel32&0xFF;
    mMachineCode[mIndex++] = ((rel32>>8)&0xFF);
    mMachineCode[mIndex++] = ((rel32>>16)&0xFF);
    mMachineCode[mIndex++] = ((rel32>>24)&0xFF);
}

/**
 * Insert JMP to label
 *
 * PARAMS
 * label    destination
 */
void CodeGenerator::jmp(const Label_t label)
{
    mJumps.push_back(new Jump(mIndex, label, &CodeGenerator::insert_jmp));
    nop(); nop(); nop();
    nop(); nop(); nop();
}

/**
 * Insert JZ to label
 *
 * PARAMS
 * label    destination
 */
void CodeGenerator::jz(const Label_t label)
{
    mJumps.push_back(new Jump(mIndex, label, &CodeGenerator::insert_jz));
    nop(); nop(); nop();
    nop(); nop(); nop();
}

/**
 * Insert JNZ to label
 *
 * PARAMS
 * label    destination
 */
void CodeGenerator::jnz(const Label_t label)
{
    mJumps.push_back(new Jump(mIndex, label, &CodeGenerator::insert_jnz));
    nop(); nop(); nop();
    nop(); nop(); nop();
}

/**
 * Insert JC to label
 *
 * PARAMS
 * label    destination
 */
void CodeGenerator::jc(const Label_t label)
{
    mJumps.push_back(new Jump(mIndex, label, &CodeGenerator::insert_jc));
    nop(); nop(); nop();
    nop(); nop(); nop();
}

/**
 * Insert JNC to label
 *
 * PARAMS
 * label    destination
 */
void CodeGenerator::jnc(const Label_t label)
{
    mJumps.push_back(new Jump(mIndex, label, &CodeGenerator::insert_jnc));
    nop(); nop(); nop();
    nop(); nop(); nop();
}

/**
 * JNZ i8
 *
 * PARAMS
 * rel8   8 bit immediate, relative distance
 */
void CodeGenerator::jnz_i8(const int8_t rel8)
{
    //75 cb
    //JNZ rel8
    //Jump short if not zero (ZF=0)
    mMachineCode[mIndex++] = 0x75;
    mMachineCode[mIndex++] = rel8;
}

/**
 * JNZ i16
 *
 * PARAMS
 * rel16   16 bit immediate, relative distance
 */
void CodeGenerator::jnz_i16(const int16_t rel16)
{
    //0F 85 cw/cd
    //JNZ rel16/32
    //Jump near if not zero (ZF=0)
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0x85;
    mMachineCode[mIndex++] = rel16&0xFF;
    mMachineCode[mIndex++] = ((rel16>>8)&0xFF);
}

/**
 * JNZ i32
 *
 * PARAMS
 * rel32   32 bit immediate, relative distance
 */
void CodeGenerator::jnz_i32(const int32_t rel32)
{
    //0F 85 cw/cd
    //JNZ rel16/32
    //Jump near if not zero (ZF=0)
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0x85;
    mMachineCode[mIndex++] = rel32&0xFF;
    mMachineCode[mIndex++] = ((rel32>>8)&0xFF);
    mMachineCode[mIndex++] = ((rel32>>16)&0xFF);
    mMachineCode[mIndex++] = ((rel32>>24)&0xFF);
}

/**
 * JNC i8
 *
 * PARAMS
 * rel8   8 bit immediate, relative distance
 */
void CodeGenerator::jnc_i8(const int8_t rel8)
{
	//73 cb
	//JNC rel8
	//Jump short if not carry (CF=0)
    mMachineCode[mIndex++] = 0x73;
    mMachineCode[mIndex++] = rel8;
}

/**
 * JNC i16
 *
 * PARAMS
 * rel16   16 bit immediate, relative distance
 */
void CodeGenerator::jnc_i16(const int16_t rel16)
{
	//0F 83 cw/cd
	//JNC rel16/32
	//Jump near if not carry (CF=0)
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0x83;
    mMachineCode[mIndex++] = rel16&0xFF;
    mMachineCode[mIndex++] = ((rel16>>8)&0xFF);
}

/**
 * JNC i32
 *
 * PARAMS
 * rel32   32 bit immediate, relative distance
 */
void CodeGenerator::jnc_i32(const int32_t rel32)
{
	//0F 83 cw/cd
	//JNC rel16/32
	//Jump near if not carry (CF=0)
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0x83;
    mMachineCode[mIndex++] = rel32&0xFF;
    mMachineCode[mIndex++] = ((rel32>>8)&0xFF);
    mMachineCode[mIndex++] = ((rel32>>16)&0xFF);
    mMachineCode[mIndex++] = ((rel32>>24)&0xFF);
}

/**
 * RDTSC
 */
void CodeGenerator::rdtsc()
{
    //0F 31
    //RDTSC
    //Read time-stamp counter into EDX:EAX
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0x31;
}

/**
 * MUL r8
 * AX = AL * r8
 *
 * PARAMS
 * reg8     8 bit register
 */
void CodeGenerator::mul_r8(const int reg8)
{
    //F6 /4
    //MUL r/m8
    //Unsigned multiply (AX ← AL ∗ r/m8)
    mMachineCode[mIndex++] = 0xF6;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x4, reg8);
}

/**
 * MUL m32
 * EDX:EAX = EAX * m32
 *
 * PARAMS
 * reg32     32 bit memory pointer
 */
void CodeGenerator::mul_m32(const int reg32)
{
    //F7 /4
    //MUL r/m32
    //Unsigned multiply (EDX:EAX ← EAX ∗ r/m32)
    mMachineCode[mIndex++] = 0xF7;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM, 0x4, reg32);
}

/**
 * XCHG r8,r8
 *
 * PARAMS
 * reg8d     8 bit destination register
 * reg8s     8 bit source register
 */
void CodeGenerator::xchg_r8r8(const int reg8d, int reg8s)
{
    //86 /r
    //XCHG r/m8,r8
    //Exchange r8 (byte register) with byte from r/m8
    mMachineCode[mIndex++] = 0x86;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg8s, reg8d);
}

/**
 * DIV r8
 * AL = AX / r8, AH = reminder
 *
 * PARAMS
 * reg8     8 bit register
 */
void CodeGenerator::div_r8(const int reg8)
{
    //F6 /6
    //DIV r/m8
    //Unsigned divide AX by r/m8; AL <- Quotient, AH <- Remainder
    mMachineCode[mIndex++] = 0xF6;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x6, reg8);
}

/**
 * SHR r8,i8
 *
 * PARAMS
 * reg8     8 bit destination register
 * imm8     8 bit immediate
 */
void CodeGenerator::shr_r8i8(const int reg8, const uint8_t imm8)
{
    //C0 /5 ib
    //SHR r/m8,imm8
    //Unsigned divide r/m8 by 2, imm8 times
    mMachineCode[mIndex++] = 0xC0;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x5, reg8);
    mMachineCode[mIndex++] = imm8;
}

/**
 * SHR r32,i8
 *
 * PARAMS
 * reg8     32 bit destination register
 * imm8     8 bit immediate
 */
void CodeGenerator::shr_r32i8(const int reg32, const uint8_t imm8)
{
    //C1 /5 ib
    //SHR r/m32,imm8
    //Unsigned divide r/m32 by 2, imm8 times
    mMachineCode[mIndex++] = 0xC1;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x5, reg32);
    mMachineCode[mIndex++] = imm8;
}

/**
 * SHR r16,i8
 *
 * PARAMS
 * reg16     16 bit destination register
 * imm8      8 bit immediate
 */
void CodeGenerator::shr_r16i8(const int reg16, const uint8_t imm8)
{
    //C1 /5 ib
    //SHR r/m16,imm8
    //Unsigned divide r/m16 by 2, imm8 times
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    shr_r32i8(reg16, imm8);
}

/**
 * SHL r8,i8
 *
 * PARAMS
 * reg8      8 bit destination register
 * imm8      8 bit immediate
 */
void CodeGenerator::shl_r8i8(const int reg8, const uint8_t imm8)
{
    //C0 /4 ib
    //SHL r/m8,imm8
    //Multiply r/m8 by 2, imm8 times
    mMachineCode[mIndex++] = 0xC0;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x4, reg8);
    mMachineCode[mIndex++] = imm8;
}

/**
 * SHL r16,i8
 *
 * PARAMS
 * reg16     16 bit destination register
 * imm8      8 bit immediate
 */
void CodeGenerator::shl_r16i8(const int reg16, const uint8_t imm8)
{
    //C1 /4 ib
    //SHL r/m16,imm8
    //Multiply r/m16 by 2, imm8 times
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    shl_r32i8(reg16, imm8);
}

/**
 * SHL r32,i8
 *
 * PARAMS
 * reg32     32 bit destination register
 * imm8      8 bit immediate
 */
void CodeGenerator::shl_r32i8(const int reg32, const uint8_t imm8)
{
    //C1 /4 ib
    //SHL r/m32,imm8
    //Multiply r/m32 by 2, imm8 times
    mMachineCode[mIndex++] = 0xC1;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x4, reg32);
    mMachineCode[mIndex++] = imm8;
}

/**
 * MOV m8,i8
 *
 * PARAMS
 * reg32     32 bit destination register
 * imm8      8 bit immediate
 */
void CodeGenerator::mov_m8i8(const int reg32, const uint8_t imm8)
{
    //C6 /0
    //MOV r/m8,imm8
    //Move imm8 to r/m8
    mMachineCode[mIndex++] = 0xC6;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM, 0, reg32);
    mMachineCode[mIndex++] = imm8;
}

/**
 * CMP r32,i32
 *
 * PARAMS
 * reg32     32 bit register
 * imm32     32 bit immediate
 */
void CodeGenerator::cmp_r32i32(const int reg32, const uint32_t imm32)
{
    //3D id
    //CMP EAX, imm32
    //Compare imm32 with EAX

    //81 /7 id
    //CMP r/m32,imm32
    //Compare imm32 with r/m32

    if(reg32 == X86_REG_EAX)
        mMachineCode[mIndex++] = 0x3D;
    else
    {
        mMachineCode[mIndex++] = 0x81;
        mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x7, reg32);
    }

    mMachineCode[mIndex++] = imm32&0xFF;
    mMachineCode[mIndex++] = ((imm32>>8)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>16)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>24)&0xFF);
}

/**
 * CMP r16,i16
 *
 * PARAMS
 * reg16     16 bit register
 * imm16     16 bit immediate
 */
void CodeGenerator::cmp_r16i16(const int reg16, const uint16_t imm16)
{
    //3D iw
    //CMP AX, imm16
    //Compare imm16 with AX

    //81 /7 iw
    //CMP r/m16, imm16
    //Compare imm16 with r/m16
    mMachineCode[mIndex++] = X86_PREFIX_REG16;

    if(reg16 == X86_REG_AX)
        mMachineCode[mIndex++] = 0x3D;
    else
    {
        mMachineCode[mIndex++] = 0x81;
        mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x7, reg16);
    }

    mMachineCode[mIndex++] = imm16&0xFF;
    mMachineCode[mIndex++] = ((imm16>>8)&0xFF);
}

/**
 * XOR m8,i8
 *
 * PARAMS
 * reg32     32 bit memory pointer
 * imm8      8 bit immediate
 */
void CodeGenerator::xor_m8i8(const int reg32, const uint8_t imm8)
{
    //80 /6 ib
    //XOR r/m8,imm8
    //r/m8 XOR imm8
    mMachineCode[mIndex++] = 0x80;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_MEM, 0x6, reg32);
    mMachineCode[mIndex++] = imm8;
}

/**
 * BSWAP r32
 *
 * PARAMS
 * reg32     32 bit register
 */
void CodeGenerator::bswap_r32(const int reg32)
{
    //0F C8+rd
    //BSWAP r32
    //Reverses the byte order of a 32-bit register.
    mMachineCode[mIndex++] = 0x0F;
    mMachineCode[mIndex++] = 0xC8 + reg32;
}

/**
 * AND r16,i16
 *
 * PARAMS
 * reg16     16 bit register
 * imm16     16 bit immediate
 */
void CodeGenerator::and_r16i16(const int reg16, const uint16_t imm16)
{
    //25 iw
    //AND AX,imm16
    //AX AND imm16

    //81 /4 iw
    //AND r/m16,imm16
    //r/m16 AND imm16
    mMachineCode[mIndex++] = X86_PREFIX_REG16;

    if(reg16 == X86_REG_AX)
        mMachineCode[mIndex++] = 0x25;
    else
    {
        mMachineCode[mIndex++] = 0x81;
        mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x4, reg16);
    }

    mMachineCode[mIndex++] = imm16&0xFF;
    mMachineCode[mIndex++] = ((imm16>>8)&0xFF);
}

/**
 * AND r32,i32
 *
 * PARAMS
 * reg32     32 bit register
 * imm32     32 bit immediate
 */
void CodeGenerator::and_r32i32(const int reg32, const uint32_t imm32)
{
    //25 id
    //AND EAX,imm32
    //EAX AND imm32

    //81 /4 id
    //AND r/m32,imm32
    //r/m32 AND imm32

    if(reg32 == X86_REG_EAX)
        mMachineCode[mIndex++] = 0x25;
    else
    {
        mMachineCode[mIndex++] = 0x81;
        mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, 0x4, reg32);
    }

    mMachineCode[mIndex++] = imm32&0xFF;
    mMachineCode[mIndex++] = ((imm32>>8)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>16)&0xFF);
    mMachineCode[mIndex++] = ((imm32>>24)&0xFF);
}

/**
 * TEST r32,r32
 *
 * PARAMS
 * reg32d    32 bit destination register
 * reg32s    32 bit source register
 */
void CodeGenerator::test_r32r32(const int reg32d, const int reg32s)
{
    //85 /r
    //TEST r/m32, r32
    //AND r32 with r/m32; set SF, ZF, PF according to result.
    mMachineCode[mIndex++] = 0x85;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg32s, reg32d);
}

/**
 * TEST r16,r16
 *
 * PARAMS
 * reg16d    16 bit destination register
 * reg16s    16 bit source register
 */
void CodeGenerator::test_r16r16(const int reg16d, const int reg16s)
{
    //85 /r
    //TEST r/m16, r16
    //AND r16 with r/m16; set SF, ZF, PF according to result.
    mMachineCode[mIndex++] = X86_PREFIX_REG16;
    mMachineCode[mIndex++] = 0x85;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg16s, reg16d);
}

/**
 * TEST r8,r8
 *
 * PARAMS
 * reg8d    8 bit destination register
 * reg8s    8 bit source register
 */
void CodeGenerator::test_r8r8(const int reg8d, const int reg8s)
{
    //84 /r
    //TEST r/m8, r8
    //AND r8 with r/m8; set SF, ZF, PF according to result.
    mMachineCode[mIndex++] = 0x84;
    mMachineCode[mIndex++] = X86_MODRM_BYTE(X86_MOD_REG, reg8s, reg8d);
}
