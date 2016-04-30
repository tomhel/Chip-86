/************************************************************
  **** RegTracker.cpp (implementation of .h)
   ***
    ** Author:
     *   Tommy Hellstrom
     *
     * Description:
     *   Tracks dynamic register allocation, chip8 to ia32
     *
     * Revision history:
     *   When         Who       What
     *   20090519     me        created
     *
     * License information:
     *   GPLv3
     *
     ********************************************************/

#include "RegTracker.h"

/**
 * Reset the status (mark as free) of a IA 8 bit register
 *
 * PARAMS
 * x86reg   register to reset
 */
void RegTracker::resetRegX8(const int x86reg)
{
    mX86Reg8[x86reg].age = 0;
    mX86Reg8[x86reg].modified = false;
    mX86Reg8[x86reg].free = true;
}

/**
 * CodeGeneration:
 * Save a allocated IA 8 bit register (if modified)
 *
 * PARAMS
 * x86reg   register to save
 */
void RegTracker::doSaveRegX8(const int x86reg)
{
    if(mX86Reg8[x86reg].modified && !mX86Reg8[x86reg].free)
    {
        const int r32 = temporaryRegX32();

        dirtyRegX32(r32);

        const uint32_t addr = mC8_regBaseAddr + mX86Reg8[x86reg].c8reg;
        codegen->mov_r32i32(r32, addr);
        codegen->mov_m8r8(r32,x86reg);
        mX86Reg8[x86reg].modified = false;
    }
}

/**
 * Code generation:
 * Replaces a IA register with another IA register
 *
 * PARAMS
 * x86reg_dst   destination register
 * x86reg_src   source register
 * loadvalue    old value will be copied if true, otherwise not
 */
void RegTracker::doReplaceRegX8(const int x86reg_dst, const int x86reg_src, const bool loadvalue)
{
    dirtyRegX8(x86reg_dst);

    if(!mX86Reg8[x86reg_dst].free)
        mFreeRegX8Count++;

    mX86Reg8[x86reg_dst] = mX86Reg8[x86reg_src];

    if(loadvalue)
        codegen->mov_r8r8(x86reg_dst, x86reg_src);

    resetRegX8(x86reg_src);
}

/**
 * Code generation:
 * Save chip8 address register to memory if it is allocated
 * to IA register and modified
 *
 * PARAMS
 * x86reg       IA register to save
 */
void RegTracker::doSaveRegC16(const int x86reg)
{
    if(mX86Reg32.modified && !mX86Reg32.free)
    {
        const int r32 = temporaryRegX32();

        dirtyRegX32(r32);

        codegen->mov_r32i32(r32, mC8_addressRegAddr);
        codegen->mov_m32r32(r32, x86reg);
        mX86Reg32.modified = false;
    }
}

/**
 * Code generation:
 * Allocate a IA 8 bit register
 *
 * PARAMS
 * x86reg       IA register to allocate
 * c8reg        chip8 register to load
 * loadvalue    value will be loaded from memory if true
 */
void RegTracker::doAllocRegX8(const int x86reg, const int c8reg, const bool loadvalue)
{
    dirtyRegX8(x86reg);

    if(mX86Reg8[x86reg].free)
        mFreeRegX8Count--;

    resetRegX8(x86reg);
    mX86Reg8[x86reg].free = false;
    mX86Reg8[x86reg].c8reg = c8reg;

    if(loadvalue)
    {
        const int r32 = temporaryRegX32();

        dirtyRegX32(r32);

        const uint32_t addr = mC8_regBaseAddr + c8reg;
        codegen->mov_r32i32(r32, addr);
        codegen->mov_r8m8(x86reg, r32);
    }
}

/**
 * Code generation:
 * Swap two IA registers
 *
 * PARAMS
 * x86reg1   register to swap
 * x86reg2   register to swap
 * loadvalue reg2 will be overwritten if false
 */
void RegTracker::doSwapRegX8(const int x86reg1, const int x86reg2, const bool loadvalue)
{
    dirtyRegX8(x86reg1);
    dirtyRegX8(x86reg2);

    const Reginfo tmp = mX86Reg8[x86reg1];
    mX86Reg8[x86reg1] = mX86Reg8[x86reg2];
    mX86Reg8[x86reg2] = tmp;

    if(loadvalue)
        codegen->xchg_r8r8(x86reg1, x86reg2);
    else
       codegen->mov_r8r8(x86reg2, x86reg1);
}

/**
 * Code generation:
 * Deallocates a IA 8 bit register (if allocated)
 *
 * PARAMS
 * x86reg   register to deallocate
 */
void RegTracker::doDeallocRegX8(const int x86reg)
{
    if(!mX86Reg8[x86reg].free)
    {
        doSaveRegX8(x86reg);
        resetRegX8(x86reg);
        mFreeRegX8Count++;
    }
}

/**
 * Code generation:
 * Allocate chip8 addressregister to IA register
 *
 * PARAMS
 * x86reg       IA register to load to
 * loadvalue    loads value from memory if true
 */
void RegTracker::doAllocRegC16(const int x86reg, const bool loadvalue)
{
    if(!mX86Reg32.free)
        return;

    dirtyRegX32(x86reg);

    mX86Reg32.free = false;
    mX86Reg32.modified = false;

    if(loadvalue)
    {
        const int r32 = temporaryRegX32();

        dirtyRegX32(r32);

        codegen->mov_r32i32(r32, mC8_addressRegAddr);
        codegen->mov_r32m32(x86reg, r32);
    }
}

/**
 * Code generation:
 * Deallocated the chip8 addressregister (if allocated)
 *
 * PARAMS
 * x86reg   the allocated IA register
 */
void RegTracker::doDeallocRegC16(const int x86reg)
{
    doSaveRegC16(x86reg);

    mX86Reg32.free = true;
    mX86Reg32.modified = false;
}

/**
 * Constructor
 *
 * PARAMS
 * cg               CodeGenerator object
 * c8_regPtr        chip8 register structure
 * c8_addressRegPtr chip8 addressregister
 */
RegTracker::RegTracker(CodeGenerator *const cg, uint8_t c8_regPtr[C8_GPREG_COUNT], uint32_t *const c8_addressRegPtr)
{
    mC8_addressRegAddr = (uintptr_t) c8_addressRegPtr;
    mC8_regBaseAddr = (uintptr_t) c8_regPtr;
    codegen = cg;

    reset();
}

/**
 * Allocates chip8 8 bit register to IA 8 bit register
 *
 * PARAM
 * c8reg      chip8 register to load
 *
 * RETURNS
 * number of the IA register that was allocated
 */
int RegTracker::allocRegX8(const int c8reg)
{
    return allocRegX8(c8reg, true);
}

/**
 * Allocates chip8 8 bit register to IA 8 bit register
 *
 * PARAM
 * c8reg      chip8 register to load
 * loadvalue  loads value in memory if true
 *
 * RETURNS
 * number of the IA register that was allocated
 */
int RegTracker::allocRegX8(const int c8reg, const bool loadvalue)
{
    bool free = false;
    bool allocated = false;
    int ifree;
    int ioldest;
    int iallocated;
    int oldest = -1;

    for(int a = 3; a >= 0; a--)
            for(int b = 0; b < 2; b++)
            {
                const int i = a + (b * 4);

                mX86Reg8[i].age++;

                if(mX86Reg8[i].c8reg == c8reg && !mX86Reg8[i].free)
                {
                    allocated = true;
                    iallocated = i;
                }
                else if(mX86Reg8[i].free)
                {
                    free = true;
                    ifree = i;
                }
                else if(mX86Reg8[i].age > oldest)
                {
                    oldest = mX86Reg8[i].age;
                    ioldest = i;
                }
            }

    if(allocated)
    {
        mX86Reg8[iallocated].age = 0;
        return iallocated;
    }
    else if(free)
    {
        doAllocRegX8(ifree, c8reg, loadvalue);
        return ifree;
    }
    else
    {
        doDeallocRegX8(ioldest);
        doAllocRegX8(ioldest, c8reg, loadvalue);
        return ioldest;
    }
}

/**
 * Allocates chip8 8 bit register to IA 8 bit register
 *
 * PARAM
 * x86reg     IA register to load to
 * c8reg      chip8 register to load
 *
 * RETURNS
 * number of the IA register that was allocated
 */
int RegTracker::allocRegX8(const int x86reg, const int c8reg)
{
    return allocRegX8(x86reg, c8reg, true);
}

/**
 * Allocates chip8 8 bit register to IA 8 bit register
 *
 * PARAM
 * x86reg     IA register to load to
 * c8reg      chip8 register to load
 * loadvalue  loads value in memory if true
 *
 * RETURNS
 * number of the IA register that was allocated
 */
int RegTracker::allocRegX8(const int x86reg, const int c8reg, const bool loadvalue)
{
        if(mX86Reg8[x86reg].c8reg == c8reg && !mX86Reg8[x86reg].free)
        {
            mX86Reg8[x86reg].age = 0;
            return x86reg;
        }

        for(int i = 0; i < X86_COUNT_REGS_8BIT; i++)
            if(mX86Reg8[i].c8reg == c8reg && !mX86Reg8[i].free)
            {
                if(!mX86Reg8[x86reg].free)
                    doSwapRegX8(x86reg, i, loadvalue);
                else
                    doReplaceRegX8(x86reg, i, loadvalue);

                mX86Reg8[x86reg].age = 0;
                return x86reg;
            }

        if(mX86Reg8[x86reg].free)
        {
            doAllocRegX8(x86reg, c8reg, loadvalue);
            return x86reg;
        }
        else
        {
            doDeallocRegX8(x86reg);
            doAllocRegX8(x86reg, c8reg, loadvalue);
            return x86reg;
        }
}

/**
 * Allocates chip8 addressregister to IA
 *
 * RETURNS
 * number of the IA register that was allocated
 */
int RegTracker::allocRegC16()
{
    return allocRegC16(true);
}

/**
 * Allocates chip8 addressregister to IA
 *
 * PARAM
 * loadvalue  loads value in memory if true
 *
 * RETURNS
 * number of the IA register that was allocated
 */
int RegTracker::allocRegC16(const bool loadvalue)
{
    doAllocRegC16(REG_C16, loadvalue);

    return REG_C16;
}

/**
 * Deallocates an IA 8 bit register (if allocated)
 *
 * PARAM
 * x86reg   register to deallocate
 */
void RegTracker::deallocRegX8(const int x86reg)
{
    doDeallocRegX8(x86reg);
}

/*void RegTracker::deallocRegC8(const int c8reg)
{
    for(int i = 0; i < X86_COUNT_REGS_8BIT; i++)
        if(mX86Reg8[i].c8reg == c8reg && !mX86Reg8[i].free)
        {
            doDeallocRegX8(i);
            break;
        }
}*/

/**
 * Deallocates chip8 addressregister (if allocated)
 */
void RegTracker::deallocRegC16()
{
    doDeallocRegC16(REG_C16);
}

/**
 * Save all live and modified registers to memory
 */
void RegTracker::saveRegisters()
{
    /*for(int i = 0; i < X86_COUNT_REGS_8BIT; i++)
        doSaveRegX8(i);

    doSaveRegC16(REG_C16);*/

    const int r32 = temporaryRegX32();
    bool initialized = false;

    for(int i = 0; i < X86_COUNT_REGS_8BIT; i++)
        if(mX86Reg8[i].modified && !mX86Reg8[i].free)
        {
            if(!initialized)
            {
                dirtyRegX32(r32);
                codegen->mov_r32i32(r32, mC8_regBaseAddr);
                initialized = true;
            }

            codegen->mov_m8r8_d8(r32, i, mX86Reg8[i].c8reg);
            mX86Reg8[i].modified = false;
        }

    doSaveRegC16(REG_C16);
}

/**
 * Reallocates a IA 8 bit register to another 8 bit register
 *
 * PARAMS
 * x86reg_from    source
 * x86reg_to      destination
 *
 * RETURNS
 * true if successful, otherwise false
 */
bool RegTracker::reallocRegX8(const int x86reg_from, const int x86reg_to)
{
    if(mX86Reg8[x86reg_from].free || !mX86Reg8[x86reg_to].free)
        return false;

    doReplaceRegX8(x86reg_to, x86reg_from, true);

    return true;
}

/**
 * Mark IA 8 bit register as modified
 *
 * PARAMS
 * x86reg    register to mark
 */
void RegTracker::modifiedRegX8(const int x86reg)
{
    mX86Reg8[x86reg].modified = true;
}

/**
 * Checks if Chip8 8 bit register is allocated
 *
 * PARAMS
 * c8reg    Chip8 register to check
 *
 * RETURNS
 * true if allocated, otherwise false
 */
void RegTracker::modifiedRegC16()
{
    mX86Reg32.modified = true;
}

/**
 * Checks if Chip8 8 bit register is allocated
 *
 * PARAMS
 * c8reg    Chip8 register to check
 *
 * RETURNS
 * true if allocated, otherwise false
 */
bool RegTracker::isAllocatedRegC8(const int c8reg) const
{
    for(int i = 0; i < X86_COUNT_REGS_8BIT; i++)
        if(mX86Reg8[i].c8reg == c8reg && !mX86Reg8[i].free)
            return true;

    return false;
}

/**
 * Checks if IA 8 bit register is allocated
 *
 * PARAMS
 * x86reg   the register to check
 * RETURNS
 * true if allocated, otherwise false
 */
bool RegTracker::isAllocatedRegX8(const int x86reg) const
{
    return !mX86Reg8[x86reg].free;
}

/**
 * Checks if Chip8 addressregister is allocated
 *
 * RETURNS
 * true if allocated, otherwise false
 */
bool RegTracker::isAllocatedRegC16() const
{
    return !mX86Reg32.free;
}

/**
 * Return the number of free IA 8 bit registers
 *
 * RETURNS
 * number of free 8 bit registers
 */
int RegTracker::getNumberOfFreeX8Regs() const
{
    return mFreeRegX8Count;
}

/**
 * Reset the tracker
 */
void RegTracker::reset()
{
    for(int i = 0; i < X86_COUNT_REGS_8BIT; i++)
        resetRegX8(i);

    mX86Reg32.free=true;
    mX86Reg32.modified=false;
    mFreeRegX8Count = X86_COUNT_REGS_8BIT;


    for(int i = 0; i < X86_COUNT_REGS_32BIT; i++)
        mDirtyReg32[i] = false;

    mDirtyCount = 0;
}

/**
 * Mark a IA 32 bit register as dirty
 *
 * PARAMS
 * x86reg   register to mark
 */
void RegTracker::dirtyRegX32(const int x86reg)
{
    if(!mDirtyReg32[x86reg] && x86reg != REG_RET)
    {
        mDirtyReg32[x86reg] = true;
        mDirtyOrder[mDirtyCount] = x86reg;
        codegen->push_r32(x86reg);
        mDirtyCount++;
    }
}

/**
 * Mark a IA 16 bit register as dirty
 *
 * PARAMS
 * x86reg   register to mark
 */
void RegTracker::dirtyRegX16(const int x86reg)
{
    dirtyRegX32(x86reg);
}

/**
 * Mark a IA 8 bit register as dirty
 *
 * PARAMS
 * x86reg   register to mark
 */
void RegTracker::dirtyRegX8(const int x86reg)
{
    dirtyRegX32(x86reg & 0x3); //x86reg mod 4
}

/**
 * Check to see if a IA 32bit register i dirty
 *
 * PARAMS
 * x86reg   register to check
 *
 * RETURNS
 * true if dirty otherwise false
 */
bool RegTracker::isDirtyX32(const int x86reg) const
{
    return mDirtyReg32[x86reg];
}

/**
 * Pops dirty registers back from stack
 */
void RegTracker::restoreDirty()
{
    const int count = mDirtyCount - 1;

    for(int i = count; i >= 0; i--)
        codegen->pop_r32(mDirtyOrder[i]);
}

/**
 * Get the temporary IA 32 bit register
 *
 * RETURNS
 * number of the temporary register
 */
int RegTracker::temporaryRegX32() const
{
    if(mX86Reg8[X86_REG_AL].free && mX86Reg8[X86_REG_AH].free)
        return REG_RET;
    else
        return REG_TMP;
}
