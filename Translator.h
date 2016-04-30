/************************************************************
  **** Translator.h (header)
   ***
    ** Author:
     *   Tommy Hellstrom
     *
     * Description:
     *   Translate CHIP-8 code into x86 code blocks.
     *
     * Revision history:
     *   When         Who       What
     *   20090425     me        created
     *
     * License information:
     *   GPLv3
     *
     ********************************************************/

#pragma once
#ifndef _TRANSLATOR_H_
#define _TRANSLATOR_H_

#include <list>
#include <stdint.h>

#include "x86def.h"
#include "Chip8def.h"
#include "CodeGenerator.h"
#include "RegTracker.h"
#include "CodeBlock.h"

#define NEW_FRAME    1
#define NO_NEW_FRAME 0

#define TO_COND_BRANCH 2

#define LCG_INCREMENT  12345
#define LCG_MULTIPLIER 1103515245

class Translator
{
    private:

        struct DecodedOpcode;

        typedef void (Translator::*TranslatorMemberFnGenerate_t)(const DecodedOpcode &);

        /**
         * Structure that stores decoded opcode information, IR-node
         */
        struct DecodedOpcode
        {
            bool                         isCondBranchDest;
            bool                         inCondition;
            bool                         leader;
            bool                         ignore;
            int                          arg1;
            int                          arg2;
            uint32_t                     arg3;
            uint32_t                     address;
            uint32_t                     opcode;
            TranslatorMemberFnGenerate_t pfnGenOpcode;

            DecodedOpcode()
            {isCondBranchDest = false; ignore = false; leader = false; inCondition = false;}
         // DecodedOpcode(const DecodedOpcode&);
         // DecodedOpcode& DecodedOpcode=(const DecodedOpcode&);
         // ~DecodedOpcode();
        };

        CodeGenerator               codegen;
        RegTracker                  tracker;
        std::list<DecodedOpcode *>  mDecodedOps;
        std::list<CodeBlock *>      mBlocks;
        Label_t                     mLabelCondBranchDest;
        Label_t                     mLabelCondReturnDest;
        bool                        mReadyToTranslate;
        bool                        mCondition;
        bool                        inlineSub;
        int                         mCountdown;
        uint32_t                    mNextOpAddress;
        uintptr_t                   mC8_regBaseAddr;
        uintptr_t                   mC8_addressRegAddr;
        uintptr_t                   mC8_delaytimerAddr;
        uintptr_t                   mC8_soundtimerAddr;
        uintptr_t                   mC8_keyBaseAddr;
        uintptr_t                   mC8_memBaseAddr;
        uintptr_t                   mC8_screenBaseAddr;
        uintptr_t                   mC8_newFrameAddr;
        uintptr_t                   mC8_seedRngAddr;
        uintptr_t                   mC8_stackPointerAddr;

        /**
         * Destroy
         */
        void destroy();

        /**
         * Decodes an opcode
         *
         * PARAMS
         * rNode    decoded info is saved to this node
         */
        void decode(DecodedOpcode &rNode);

        /**
         * Start translation.
         * Generates machinecode from IR
         */
        void translate();

        /**
         * Sets the codegeneration-function for an IR-node.
         * If the node is in an IF statement a forced return
         * will be generated.
         *
         * PARAMS
         * rNode         ref. to IR-node (decoded node)
         * pfnGenOpcode  func. pointer to codegeneration routin for that node
         */
        void setOpcodeFunction(DecodedOpcode &rNode, const TranslatorMemberFnGenerate_t pfnGenOpcode);

        /**
         * Generates code to force return
         *
         * PARAMS
         * rNode    ref. to IR-node (decoded node)
         */
        void generateReturn(const DecodedOpcode &rNode);

        /**
         * Unknown opcode
         * sets decoded info for an unknown opcode (ignore = true)
         *
         * PARAMS
         * rNode    ref. to IR-node (decoded node)
         */
        void unknownOpcode(DecodedOpcode &rNode);

        /**
         * Decode 00E0
         * Clear the screen
         */
        void decode00E0(DecodedOpcode &rNode);

        /**
         * Generate 00E0
         * Clear the screen
         */
        void generate00E0(const DecodedOpcode &rNode);

        /**
         * Decode 00EE
         * return from subroutin
         */
        void decode00EE(DecodedOpcode &rNode);

        /**
         * Generate 00EE
         * return from subroutin
         */
        void generate00EE(const DecodedOpcode &rNode);

        /**
         * Decode 1NNN
         * jump to address NNN
         */
        void decode1NNN(DecodedOpcode &rNode);

        /**
         * Generate 1NNN
         * jump to address NNN
         */
        void generate1NNN(const DecodedOpcode &rNode);

        /**
         * Decode 2NNN
         * call subroutine at NNN
         */
        void decode2NNN(DecodedOpcode &rNode);

        /**
         * Generate 2NNN
         * call subroutine at NNN
         */
        void generate2NNN(const DecodedOpcode &rNode);

        /**
         * Decode 3XNN
         * skip next instruction if VX == kk
         */
        void decode3XNN(DecodedOpcode &rNode);

        /**
         * Generate 3XNN
         * skip next instruction if VX == kk
         */
        void generate3XNN(const DecodedOpcode &rNode);

        /**
         * Decode 4XNN
         * skip next instruction if VX != NN
         */
        void decode4XNN(DecodedOpcode &rNode);

        /**
         * Generate 4XNN
         * skip next instruction if VX != NN
         */
        void generate4XNN(const DecodedOpcode &rNode);

        /**
         * Decode 5XY0
         * skip next instruction if VX == VY
         */
        void decode5XY0(DecodedOpcode &rNode);

        /**
         * Generate 5XY0
         * skip next instruction if VX == VY
         */
        void generate5XY0(const DecodedOpcode &rNode);

        /**
         * Decode 6XNN
         * sets VX to nn
         */
        void decode6XNN(DecodedOpcode &rNode);

        /**
         * Generate 6XNN
         * sets VX to nn
         */
        void generate6XNN(const DecodedOpcode &rNode);

        /**
         * Decode 7XNN
         * adds NN to vx. carry not affected
         */
        void decode7XNN(DecodedOpcode &rNode);

        /**
         * Generate 7XNN
         * adds NN to vx. carry not affected
         */
        void generate7XNN(const DecodedOpcode &rNode);

        /**
         * Decode 8XY0
         * set vx to vy
         */
        void decode8XY0(DecodedOpcode &rNode);

        /**
         * Generate 8XY0
         * set vx to vy
         */
        void generate8XY0(const DecodedOpcode &rNode);

        /**
         * Decode 8XY1
         * VX = VX | VY
         */
        void decode8XY1(DecodedOpcode &rNode);

        /**
         * Generate 8XY1
         * VX = VX | VY
         */
        void generate8XY1(const DecodedOpcode &rNode);

        /**
         * Decode 8XY2
         * VX = VX & VY
         */
        void decode8XY2(DecodedOpcode &rNode);

        /**
         * Generate 8XY2
         * VX = VX & VY
         */
        void generate8XY2(const DecodedOpcode &rNode);

        /**
         * Decode 8XY3
         * VX = VX xor VY
         */
        void decode8XY3(DecodedOpcode &rNode);

        /**
         * Generate 8XY3
         * VX = VX xor VY
         */
        void generate8XY3(const DecodedOpcode &rNode);

        /**
         * Decode 8XY4
         * add vy to vx. set carry to 1 if overflow otherwise 0
         */
        void decode8XY4(DecodedOpcode &rNode);

        /**
         * Generate 8XY4
         * add vy to vx. set carry to 1 if overflow otherwise 0
         */
        void generate8XY4(const DecodedOpcode &rNode);

        /**
         * Decode 8XY5
         * sub vy from vx. set carry to 1 if no borrow otherwise 0
         */
        void decode8XY5(DecodedOpcode &rNode);

        /**
         * Generate 8XY5
         * sub vy from vx. set carry to 1 if no borrow otherwise 0
         */
        void generate8XY5(const DecodedOpcode &rNode);

        /**
         * Decode 8XY6
         * Shifts VX right by one. VF is set to the value of the least
         * significant bit of VX before the shift.
         */
        void decode8XY6(DecodedOpcode &rNode);

        /**
         * Generate 8XY6
         * Shifts VX right by one. VF is set to the value of the least
         * significant bit of VX before the shift.
         */
        void generate8XY6(const DecodedOpcode &rNode);

        /**
         * Decode
         * 8XY7 Sets VX to VY minus VX. VF is set to 0 when there's a borrow,
         * and 1 when there isn't.
         */
        void decode8XY7(DecodedOpcode &rNode);

        /**
         * Generate 8XY7
         * Sets VX to VY minus VX. VF is set to 0 when
         * there's a borrow, and 1 when there isn't.
         */
        void generate8XY7(const DecodedOpcode &rNode);

        /**
         * Decode 8XYE
         * Shifts VX left by one. VF is set to the value of
         * the most significant bit of VX before the shift
         */
        void decode8XYE(DecodedOpcode &rNode);

        /**
         * Generate 8XYE
         * Shifts VX left by one. VF is set to the value of
         * the most significant bit of VX before the shift
         */
        void generate8XYE(const DecodedOpcode &rNode);

        /**
         * Decode 9XY0
         * skip next instruction if VX != VY
         */
        void decode9XY0(DecodedOpcode &rNode);

        /**
         * Generate 9XY0
         * skip next instruction if VX != VY
         */
        void generate9XY0(const DecodedOpcode &rNode);

        /**
         * Decode ANNN
         * set I to nnn
         */
        void decodeANNN(DecodedOpcode &rNode);

        /**
         * Generate ANNN
         * set I to nnn
         */
        void generateANNN(const DecodedOpcode &rNode);

        /**
         * Decode BNNN
         * jump to address NNN + V0
         */
        void decodeBNNN(DecodedOpcode &rNode);

        /**
         * Generate BNNN
         * jump to address NNN + V0
         */
        void generateBNNN(const DecodedOpcode &rNode);

        /**
         * Decode CXNN
         * set vx to rand & NN
         */
        void decodeCXNN(DecodedOpcode &rNode);

        /**
         * Generate CXNN
         * set vx to rand & NN
         */
        void generateCXNN(const DecodedOpcode &rNode);

        /**
         * Decode DXYN
         * Draws a sprite at coordinate (VX, VY); that has a width of
         * 8 pixels and a height of N pixels.
         * As described above, VF is set to 1 if any screen pixels are flipped
         * from set to unset when the sprite is drawn, and to 0 if that doesn't happen
         */
        void decodeDXYN(DecodedOpcode &rNode);

        /**
         * Generate DXYN
         * Draws a sprite at coordinate (VX, VY); that has a width of
         * 8 pixels and a height of N pixels.
         * As described above, VF is set to 1 if any screen pixels are flipped
         * from set to unset when the sprite is drawn, and to 0 if that doesn't happen
         */
        void generateDXYN(const DecodedOpcode &rNode);

        /**
         * Decode EX9E
         * Skips the next instruction if the key stored in VX is pressed.
         */
        void decodeEX9E(DecodedOpcode &rNode);

        /**
         * Generate EX9E
         * Skips the next instruction if the key stored in VX is pressed.
         */
        void generateEX9E(const DecodedOpcode &rNode);

        /**
         * Decode EXA1
         * Skips the next instruction if the key stored in VX isn't pressed..
         */
        void decodeEXA1(DecodedOpcode &rNode);

        /**
         * Generate EXA1
         * Skips the next instruction if the key stored in VX isn't pressed..
         */
        void generateEXA1(const DecodedOpcode &rNode);

        /**
         * Decode FX07
         * Sets VX to the value of the delay timer
         */
        void decodeFX07(DecodedOpcode &rNode);

        /**
         * Generate FX07
         * Sets VX to the value of the delay timer
         */
        void generateFX07(const DecodedOpcode &rNode);

        /**
         * Decode FX0A
         * A key press is awaited, and then stored in VX.
         */
        void decodeFX0A(DecodedOpcode &rNode);

        /**
         * Generate FX0A
         * A key press is awaited, and then stored in VX.
         */
        void generateFX0A(const DecodedOpcode &rNode);

        /**
         * Decode FX15
         * delay to vx
         */
        void decodeFX15(DecodedOpcode &rNode);

        /**
         * Generate FX15
         * delay to vx
         */
        void generateFX15(const DecodedOpcode &rNode);

        /**
         * Decode FX18
         * sound to vx
         */
        void decodeFX18(DecodedOpcode &rNode);

        /**
         * Generate FX18
         * sound to vx
         */
        void generateFX18(const DecodedOpcode &rNode);

        /**
         * Decode FX1E
         * adds vx to I
         */
        void decodeFX1E(DecodedOpcode &rNode);

        /**
         * Generate FX1E
         * adds vx to I
         */
        void generateFX1E(const DecodedOpcode &rNode);

        /**
         * Decode FX29
         * Sets I to the location of the sprite for the character in VX.
         * Characters 0-F (in hexadecimal); are represented by a 4x5 font.
         */
        void decodeFX29(DecodedOpcode &rNode);

        /**
         * Generate FX29
         * Sets I to the location of the sprite for the character in VX.
         * Characters 0-F (in hexadecimal); are represented by a 4x5 font.
         */
        void generateFX29(const DecodedOpcode &rNode);

        /**
         * Decode FX33
         * Stores the Binary-coded decimal representation of VX at the addresses I,
         * I plus 1, and I plus 2.
         */
        void decodeFX33(DecodedOpcode &rNode);

        /**
         * Generate FX33
         * Stores the Binary-coded decimal representation of VX at the addresses I,
         * I plus 1, and I plus 2.
         */
        void generateFX33(const DecodedOpcode &rNode);

        /**
         * Decode FX55
         * Stores V0 to VX in memory starting at address I.
         */
        void decodeFX55(DecodedOpcode &rNode);

        /**
         * Generate FX55
         * Stores V0 to VX in memory starting at address I.
         */
        void generateFX55(const DecodedOpcode &rNode);

        /**
         * Decode FX65
         * Fills V0 to VX with values from memory starting at address I.
         */
        void decodeFX65(DecodedOpcode &rNode);

        /**
         * Generate
         * FX65 Fills V0 to VX with values from memory starting at address I.
         */
        void generateFX65(const DecodedOpcode &rNode);

    public:

        /**
         * Emit an opcode to build a codeblock
         *
         * PARAMS
         * opcode   the opcode
         * rC8PC    ref. to emulated PC
         */
        bool emit(const uint32_t opcode, uint32_t &rC8PC);

        /**
         * Get a translated codeblock
         *
         * PARAMS
         * ppBlock  pointer to pointer to Codeblock
         *
         * RETURNS
         * returns true if codeblock exist, otherwise false
         */
        bool getCodeBlock(CodeBlock **const ppBlock);

        /**
         * Reset the translator
         */
        void reset();

        /**
         * Constructor
         *
         * PARAMS
         * c8_regArray          registers
         * pC8_seedRngAddr      random seed
         * pC8_addressReg       addressregister
         * pC8_delaytimer       delaytimer
         * pC8_soundtimer       soundtimer
         * pC8_newframe         new-frame-indicator
         * c8_keyArray          keypresses
         * c8_memArray          memory
         * c8_screendata        screen
         * pC8_stackPointer     stackpointer
         */
                Translator(uint8_t c8_regArray[C8_GPREG_COUNT],
                           uint32_t *const pC8_seedRngAddr,
                           uint32_t *const pC8_addressReg,
                           uint8_t *const pC8_delaytimer,
                           uint8_t *const pC8_soundtimer,
                           uint32_t *const pC8_newframe,
                           uint8_t c8_keyArray[C8_KEY_COUNT],
                           uint8_t c8_memArray[C8_MEMSIZE],
                           uint8_t c8_screendata[C8_RES_HEIGHT][C8_RES_WIDTH],
                           uint32_t **const pC8_stackPointer);

        /**
         * Destructor
         */
                ~Translator();
        //      Translator(const Translator&);
        //      Translator& Translator=(const Translator&);
};

#endif
