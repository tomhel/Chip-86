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

#pragma once
#ifndef _REGTRACKER_H_
#define _REGTRACKER_H_

#include <stdint.h>

#include "x86def.h"
#include "Chip8def.h"
#include "CodeGenerator.h"

class RegTracker
{
    private:

        /**
         * Keep track of IA register status
         */
        struct Reginfo
        {
            int     c8reg;
            int     age;
            bool    modified;
            bool    free;

            Reginfo()
            {age = 0; modified = false; free = true;}
            //default copy constructor
            //default assignment constructor
            //default destructor
        };

        CodeGenerator      *codegen;

        Reginfo             mX86Reg8[X86_COUNT_REGS_8BIT];
        Reginfo             mX86Reg32;
        uintptr_t           mC8_addressRegAddr;
        uintptr_t           mC8_regBaseAddr;
        int                 mFreeRegX8Count;

        int                 mDirtyCount;
        int                 mDirtyOrder[X86_COUNT_REGS_32BIT];
        bool                mDirtyReg32[X86_COUNT_REGS_32BIT];

        /**
         * Reset the status (free/not free) of a IA 8 bit register
         *
         * PARAMS
         * x86reg   register to reset
         */
        void resetRegX8(const int x86reg);

        /**
         * CodeGeneration:
         * Save a allocated IA 8 bit register (if modified)
         *
         * PARAMS
         * x86reg   register to save
         */
        void doSaveRegX8(const int x86reg);

        /**
         * Code generation:
         * Replaces a IA register with another IA register
         *
         * PARAMS
         * x86reg_dst   destination register
         * x86reg_src   source register
         * loadvalue    old value will be copied if true, otherwise not
         */
        void doReplaceRegX8(const int x86reg_dst, const int x86reg_src, const bool loadvalue);

        /**
         * Code generation:
         * Save chip8 address register to memory if it is allocated
         * to IA register and modified
         *
         * PARAMS
         * x86reg       IA register to save
         */
        void doSaveRegC16(const int x86reg);

        /**
         * Code generation:
         * Allocate a IA 8 bit register
         *
         * PARAMS
         * x86reg       IA register to allocate
         * c8reg        chip8 register to load
         * loadvalue    value will be loaded from memory if true
         */
        void doAllocRegX8(const int x86reg, const int c8reg, const bool loadvalue);

        /**
         * Code generation:
         * Swap two IA registers
         *
         * PARAMS
         * x86reg1   register to swap
         * x86reg2   register to swap
         * loadvalue reg2 will be overwritten if false
         */
        void doSwapRegX8(const int x86reg1, const int x86reg2, const bool loadvalue);

        /**
         * Code generation:
         * Deallocates a IA 8 bit register (if allocated)
         *
         * PARAMS
         * x86reg   register to deallocate
         */
        void doDeallocRegX8(const int x86reg);

        /**
         * Code generation:
         * Allocate chip8 addressregister to IA register
         *
         * PARAMS
         * x86reg       IA register to load to
         * loadvalue    loads value from memory if true
         */
        void doAllocRegC16(const int x86reg, const bool loadvalue);

        /**
         * Code generation:
         * Deallocated the chip8 addressregister (if allocated)
         *
         * PARAMS
         * x86reg   the allocated IA register
         */
        void doDeallocRegC16(const int x86reg);

    public:

        static const int REG_C16 = X86_REG_ESI;
        static const int REG_TMP = X86_REG_EDI;
        static const int REG_RET = X86_REG_EAX;

        /**
         * Constructor
         *
         * PARAMS
         * cg               CodeGenerator object
         * c8_regPtr        chip8 register structure
         * c8_addressRegPtr chip8 addressregister
         */
        RegTracker(CodeGenerator *const cg, uint8_t c8_regPtr[C8_GPREG_COUNT], uint32_t *const c8_addressRegPtr);

        /**
         * Allocates chip8 8 bit register to IA 8 bit register
         *
         * PARAM
         * c8reg      chip8 register to load
         *
         * RETURNS
         * number of the IA register that was allocated
         */
        int allocRegX8(const int c8reg);

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
        int allocRegX8(const int c8reg, const bool loadvalue);

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
        int allocRegX8(const int x86reg, const int c8reg);

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
        int allocRegX8(const int x86reg, const int c8reg, const bool loadvalue);

        /**
         * Allocates chip8 addressregister to IA
         *
         * RETURNS
         * number of the IA register that was allocated
         */
        int allocRegC16();

        /**
         * Allocates chip8 addressregister to IA
         *
         * PARAM
         * loadvalue  loads value in memory if true
         *
         * RETURNS
         * number of the IA register that was allocated
         */
        int allocRegC16(const bool loadvalue);

        /**
         * Deallocates an IA 8 bit register (if allocated)
         *
         * PARAM
         * x86reg   register to deallocate
         */
        void deallocRegX8(const int x86reg);

        /**
         * Deallocates chip8 addressregister (if allocated)
         */
        void deallocRegC16();

        /**
         * Save all live and modified registers to memory
         */
        void saveRegisters();

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
        bool reallocRegX8(const int x86reg_from, const int x86reg_to);
        /**
         * Mark IA 8 bit register as modified
         *
         * PARAMS
         * x86reg    register to mark
         */
        void modifiedRegX8(const int x86reg);

        /**
         * Checks if Chip8 8 bit register is allocated
         *
         * PARAMS
         * c8reg    Chip8 register to check
         *
         * RETURNS
         * true if allocated, otherwise false
         */
        void modifiedRegC16();

        /**
         * Checks if Chip8 8 bit register is allocated
         *
         * PARAMS
         * c8reg    Chip8 register to check
         *
         * RETURNS
         * true if allocated, otherwise false
         */
        bool isAllocatedRegC8(const int c8reg) const;

        /**
         * Checks if IA 8 bit register is allocated
         *
         * PARAMS
         * x86reg   the register to check
         * RETURNS
         * true if allocated, otherwise false
         */
        bool isAllocatedRegX8(const int x86reg) const;

        /**
         * Checks if Chip8 addressregister is allocated
         *
         * RETURNS
         * true if allocated, otherwise false
         */
        bool isAllocatedRegC16() const;

        /**
         * Return the number of free IA 8 bit registers
         *
         * RETURNS
         * number of free 8 bit registers
         */
        int getNumberOfFreeX8Regs() const;

        /**
         * Reset the tracker
         */
        void reset();

        /**
         * Mark a IA 32 bit register as dirty
         *
         * PARAMS
         * x86reg   register to mark
         */
        void dirtyRegX32(const int x86reg);

        /**
         * Mark a IA 16 bit register as dirty
         *
         * PARAMS
         * x86reg   register to mark
         */
        void dirtyRegX16(const int x86reg);

        /**
         * Mark a IA 8 bit register as dirty
         *
         * PARAMS
         * x86reg   register to mark
         */
        void dirtyRegX8(const int x86reg);

        /**
         * Check to see if a IA 32bit register i dirty
         *
         * PARAMS
         * x86reg   register to check
         *
         * RETURNS
         * true if dirty otherwise false
         */
        bool isDirtyX32(const int x86reg) const;

        /**
         * Pops dirty registers back from stack
         */
        void restoreDirty();

        /**
         * Get the temporary IA 32 bit register
         *
         * RETURNS
         * number of the temporary register
         */
        int temporaryRegX32() const;

         // RegTracker(const RegTracker&);
         // RegTracker& RegTracker=(const RegTracker&);
         // ~RegTracker()
};

#endif //_REGTRACKER_H_
