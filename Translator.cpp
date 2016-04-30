/************************************************************
  **** Translator.cpp (implementation of.h)
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

#include "Translator.h"

/**
 * Reset the translator
 */
void Translator::reset()
{
    mCondition = false;
    mReadyToTranslate = false;
    mCountdown = 0;

    codegen.reset();
    tracker.reset();
    destroy();
}

/**
 * Destroy
 */
void Translator::destroy()
{
    while(!mBlocks.empty())
    {
        const CodeBlock *const p = mBlocks.back();
        mBlocks.pop_back();
        delete p;
    }

    while(!mDecodedOps.empty())
    {
        const DecodedOpcode *const p = mDecodedOps.back();
        mDecodedOps.pop_back();
        delete p;
    }
}

/**
 * Get a translated codeblock
 *
 * PARAMS
 * ppBlock  pointer to pointer to Codeblock
 *
 * RETURNS
 * returns true if codeblock exist, otherwise false
 */
bool Translator::getCodeBlock(CodeBlock **const ppBlock)
{
    if(mBlocks.empty())
        return false;

    *ppBlock = mBlocks.front();
    mBlocks.pop_front();

    if(mBlocks.empty())
        reset();

    return true;
}

/**
 * Start translation.
 * Generates machinecode from IR
 */
void Translator::translate()
{
    int opcount = 0;
    int i = 0;
    uint32_t address = mDecodedOps.front()->address;

    while(!mDecodedOps.empty())
    {
        opcount++;
        DecodedOpcode *pNode = mDecodedOps.front();
        mDecodedOps.pop_front();

        if(!pNode->ignore)
        {
            if(pNode->isCondBranchDest)
                codegen.insertLabel(mLabelCondBranchDest);

            if (pNode->leader && i > 0)
            {
                generateReturn(*pNode);
                void *pBlock;
                size_t size;
                void *const pCode = codegen.getAlignedCodePointer(&pBlock, &size);
                mBlocks.push_front(new CodeBlock(pBlock, pCode, address, opcount, size));
                address = pNode->address;
                opcount = 1;

                tracker.reset();
                //condition = false;
                //countdown = 0;
            }

            (this->*pNode->pfnGenOpcode)(*pNode);
        }

        delete pNode;
        i++;
    }

    void *pBlock;
    size_t size;
    void *const pCode = codegen.getAlignedCodePointer(&pBlock, &size);
    mBlocks.push_front(new CodeBlock(pBlock, pCode, address, opcount, size));
}

/**
 * Emit an opcode to build a codeblock
 *
 * PARAMS
 * opcode   the opcode
 * rC8PC    ref. to emulated PC
 */
bool Translator::emit(const uint32_t opcode, uint32_t &rC8PC)
{
    if(mReadyToTranslate)
    {
        rC8PC = mNextOpAddress;
        return false;
    }

    DecodedOpcode *pNode = new DecodedOpcode();
    pNode->address = rC8PC;
    pNode->opcode = opcode;
    decode(*pNode);
    mDecodedOps.push_back(pNode);

    if(mCondition && mCountdown == 0)
    {
        mReadyToTranslate = true;
        pNode->isCondBranchDest = true;
    }
    else if(mCondition)
        mCountdown--;

    if(mReadyToTranslate)
    {
        mNextOpAddress = mDecodedOps.front()->address;
        rC8PC = mNextOpAddress;
        translate();
    }
    else
        rC8PC = mNextOpAddress;

    return !mReadyToTranslate;
}

/**
 * Sets the codegeneration-function for an IR-node.
 * If the node is in an IF statement a forced return
 * will be generated.
 *
 * PARAMS
 * rNode         ref. to IR-node (decoded node)
 * pfnGenOpcode  func. pointer to codegeneration routin for that node
 */
inline void Translator::setOpcodeFunction(DecodedOpcode &rNode, const TranslatorMemberFnGenerate_t pfnGenOpcode)
{
    if(!mCondition)
        rNode.pfnGenOpcode = pfnGenOpcode;
    else
        rNode.pfnGenOpcode = &Translator::generateReturn;
}

/**
 * Generates code to force return
 *
 * PARAMS
 * rNode    ref. to IR-node (decoded node)
 */
void Translator::generateReturn(const DecodedOpcode &rNode)
{
    if(!rNode.inCondition)
        tracker.saveRegisters();

    tracker.restoreDirty();

    codegen.mov_r32i32(X86_REG_EAX, rNode.address);

    codegen.ret();
}

/**
 * Unknown opcode
 * sets decoded info for an unknown opcode (ignore = true)
 *
 * PARAMS
 * rNode    ref. to IR-node (decoded node)
 */
void Translator::unknownOpcode(DecodedOpcode &rNode)
{
    rNode.ignore = true;
    rNode.pfnGenOpcode = NULL;
}

/**
 * Decode 00E0
 * Clear the screen
 */
void Translator::decode00E0(DecodedOpcode &rNode)
{
    setOpcodeFunction(rNode, &Translator::generate00E0);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 00E0
 * Clear the screen
 */
void Translator::generate00E0(const DecodedOpcode &rNode)
{
    const Label_t loop = codegen.newLabel();
    const int r32 = tracker.temporaryRegX32();

    tracker.dirtyRegX32(r32);

    codegen.mov_r32i32(r32, mC8_screenBaseAddr);

    codegen.insertLabel(loop);
        for(int d = 0; d < C8_RES_WIDTH; d+=4)
            codegen.mov_m32i32_d8(r32, C8_PIXEL_OFF, d);

        codegen.add_r32i32(r32, C8_RES_WIDTH);
    codegen.cmp_r32i32(r32, mC8_screenBaseAddr + C8_RES_HEIGHT * C8_RES_WIDTH);
    codegen.jnz(loop);

    codegen.mov_r32i32(r32, mC8_newFrameAddr);
    codegen.mov_m32i32(r32, NEW_FRAME);
}

/**
 * Decode 00EE
 * return from subroutin
 */
void Translator::decode00EE(DecodedOpcode &rNode)
{
    rNode.pfnGenOpcode = &Translator::generate00EE;
    rNode.inCondition = mCondition;
    mReadyToTranslate = !mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 00EE
 * return from subroutin
 */
void Translator::generate00EE(const DecodedOpcode &rNode)
{
    if(!rNode.inCondition)
        tracker.saveRegisters();

    bool pop = false;

    if(!tracker.isDirtyX32(tracker.REG_TMP))
    {
        pop = true;
        codegen.push_r32(tracker.REG_TMP);
    }

    codegen.mov_r32i32(tracker.REG_TMP, mC8_stackPointerAddr);
    codegen.mov_r32m32(X86_REG_EAX, tracker.REG_TMP);
    codegen.sub_r32i32(X86_REG_EAX, 4);
    codegen.mov_m32r32(tracker.REG_TMP, X86_REG_EAX);
    codegen.mov_r32m32(X86_REG_EAX, X86_REG_EAX);

    if(pop)
        codegen.pop_r32(tracker.REG_TMP);

    tracker.restoreDirty();

    codegen.ret();

    /*if(!mCondition)
        tracker.saveRegisters();

    tracker.restoreDirty();

    codegen.xor_r32r32(X86_REG_EAX, X86_REG_EAX);
    codegen.ret();*/
}

/**
 * Decode 1NNN
 * jump to address NNN
 */
void Translator::decode1NNN(DecodedOpcode &rNode)
{
	rNode.arg3 = rNode.opcode & 0x0FFF;

    rNode.pfnGenOpcode = &Translator::generate1NNN;
    rNode.inCondition = mCondition;

    mReadyToTranslate = !mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 1NNN
 * jump to address NNN
 */
void Translator::generate1NNN(const DecodedOpcode &rNode)
{
    if(!rNode.inCondition)
        tracker.saveRegisters();

    tracker.restoreDirty();

    codegen.mov_r32i32(X86_REG_EAX, rNode.arg3);

    codegen.ret();
}

/**
 * Decode 2NNN
 * call subroutine at NNN
 */
void Translator::decode2NNN(DecodedOpcode &rNode)
{
    rNode.arg3 = rNode.opcode & 0x0FFF;
    rNode.pfnGenOpcode = &Translator::generate2NNN;
    rNode.inCondition = mCondition;
    mReadyToTranslate = !mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 2NNN
 * call subroutine at NNN
 */
void Translator::generate2NNN(const DecodedOpcode &rNode)
{
    if(!rNode.inCondition)
        tracker.saveRegisters();

    bool pop = false;

    if(!tracker.isDirtyX32(tracker.REG_TMP))
    {
        pop = true;
        codegen.push_r32(tracker.REG_TMP);
    }

    codegen.mov_r32i32(tracker.REG_TMP, mC8_stackPointerAddr);
    codegen.mov_r32m32(X86_REG_EAX, tracker.REG_TMP);
    codegen.mov_m32i32(X86_REG_EAX, rNode.address + C8_OPCODE_SIZE);
    codegen.add_r32i32(X86_REG_EAX, 4);
    codegen.mov_m32r32(tracker.REG_TMP, X86_REG_EAX);

    if(pop)
        codegen.pop_r32(tracker.REG_TMP);

    tracker.restoreDirty();

    codegen.mov_r32i32(X86_REG_EAX, rNode.arg3);

    codegen.ret();


    /*const uint32_t retval = ((rNode.address + C8_OPCODE_SIZE) << 16) | rNode.arg3;

    if(!mCondition)
        tracker.saveRegisters();

    tracker.restoreDirty();

    codegen.mov_r32i32(X86_REG_EAX, retval);
    codegen.ret();*/
}

/**
 * Decode 3XNN
 * skip next instruction if VX == kk
 */
void Translator::decode3XNN(DecodedOpcode &rNode)
{
	rNode.arg2 = rNode.opcode & 0x00FF;
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generate3XNN);
    rNode.inCondition = mCondition;

    if(!mCondition)
    {
       mCondition = true;
       mCountdown = TO_COND_BRANCH;
    }

    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 3XNN
 * skip next instruction if VX == kk
 */
void Translator::generate3XNN(const DecodedOpcode &rNode)
{
    mLabelCondBranchDest = codegen.newLabel();
    const int r = tracker.allocRegX8(rNode.arg1);
    tracker.saveRegisters();

    if(rNode.arg2 == 0)
        codegen.test_r8r8(r, r);
    else
        codegen.cmp_r8i8(r, rNode.arg2);

    codegen.jz(mLabelCondBranchDest);
}

/**
 * Decode 4XNN
 * skip next instruction if VX != NN
 */
void Translator::decode4XNN(DecodedOpcode &rNode)
{
	rNode.arg2 = rNode.opcode & 0x00FF;
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generate4XNN);
    rNode.inCondition = mCondition;

    if(!mCondition)
    {
       mCondition = true;
       mCountdown = TO_COND_BRANCH;
    }

    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 4XNN
 * skip next instruction if VX != NN
 */
void Translator::generate4XNN(const DecodedOpcode &rNode)
{
    mLabelCondBranchDest = codegen.newLabel();
    const int r = tracker.allocRegX8(rNode.arg1);
    tracker.saveRegisters();

    if(rNode.arg2 == 0)
        codegen.test_r8r8(r, r);
    else
        codegen.cmp_r8i8(r, rNode.arg2);

    codegen.jnz(mLabelCondBranchDest);
}

/**
 * Decode 5XY0
 * skip next instruction if VX == VY
 */
void Translator::decode5XY0(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;
	rNode.arg2 = (rNode.opcode & 0x00F0) >> 4;

    setOpcodeFunction(rNode, &Translator::generate5XY0);
    rNode.inCondition = mCondition;

    if(!mCondition)
    {
       mCondition = true;
       mCountdown = TO_COND_BRANCH;
    }

    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 5XY0
 * skip next instruction if VX == VY
 */
void Translator::generate5XY0(const DecodedOpcode &rNode)
{
    mLabelCondBranchDest = codegen.newLabel();

    const int r1 = tracker.allocRegX8(rNode.arg1);
    const int r2 = tracker.allocRegX8(rNode.arg2);
    tracker.saveRegisters();

    codegen.cmp_r8r8(r1, r2);
    codegen.jz(mLabelCondBranchDest);
}

/**
 * Decode 6XNN
 * sets VX to nn
 */
void Translator::decode6XNN(DecodedOpcode &rNode)
{
	rNode.arg2 = rNode.opcode & 0x00FF;
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generate6XNN);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 6XNN
 * sets VX to nn
 */
void Translator::generate6XNN(const DecodedOpcode &rNode)
{
    const int r = tracker.allocRegX8(rNode.arg1, false);

    codegen.mov_r8i8(r, rNode.arg2);

    tracker.modifiedRegX8(r);
}

/**
 * Decode 7XNN
 * adds NN to vx. carry not affected
 */
void Translator::decode7XNN(DecodedOpcode &rNode)
{
	rNode.arg2 = rNode.opcode & 0x00FF;
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generate7XNN);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 7XNN
 * adds NN to vx. carry not affected
 */
void Translator::generate7XNN(const DecodedOpcode &rNode)
{
    const int r = tracker.allocRegX8(rNode.arg1);

    codegen.add_r8i8(r, rNode.arg2);

    tracker.modifiedRegX8(r);
}

/**
 * Decode 8XY0
 * set vx to vy
 */
void Translator::decode8XY0(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;
	rNode.arg2 = (rNode.opcode & 0x00F0) >> 4;

    setOpcodeFunction(rNode, &Translator::generate8XY0);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 8XY0
 * set vx to vy
 */
void Translator::generate8XY0(const DecodedOpcode &rNode)
{
    const int r1 = tracker.allocRegX8(rNode.arg1, false);
    const int r2 = tracker.allocRegX8(rNode.arg2);

    codegen.mov_r8r8(r1, r2);

    tracker.modifiedRegX8(r1);
}

/**
 * Decode 8XY1
 * VX = VX | VY
 */
void Translator::decode8XY1(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;
	rNode.arg2 = (rNode.opcode & 0x00F0) >> 4;

    setOpcodeFunction(rNode, &Translator::generate8XY1);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 8XY1
 * VX = VX | VY
 */
void Translator::generate8XY1(const DecodedOpcode &rNode)
{
    const int r1 = tracker.allocRegX8(rNode.arg1);
    const int r2 = tracker.allocRegX8(rNode.arg2);

    codegen.or_r8r8(r1, r2);

    tracker.modifiedRegX8(r1);
}

/**
 * Decode 8XY2
 * VX = VX & VY
 */
void Translator::decode8XY2(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;
	rNode.arg2 = (rNode.opcode & 0x00F0) >> 4;

    setOpcodeFunction(rNode, &Translator::generate8XY2);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 8XY2
 * VX = VX & VY
 */
void Translator::generate8XY2(const DecodedOpcode &rNode)
{
    const int r1 = tracker.allocRegX8(rNode.arg1);
    const int r2 = tracker.allocRegX8(rNode.arg2);

    codegen.and_r8r8(r1, r2);

    tracker.modifiedRegX8(r1);
}

/**
 * Decode 8XY3
 * VX = VX xor VY
 */
void Translator::decode8XY3(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;
	rNode.arg2 = (rNode.opcode & 0x00F0) >> 4;

    setOpcodeFunction(rNode, &Translator::generate8XY3);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 8XY3
 * VX = VX xor VY
 */
void Translator::generate8XY3(const DecodedOpcode &rNode)
{
    const int r1 = tracker.allocRegX8(rNode.arg1);
    const int r2 = tracker.allocRegX8(rNode.arg2);

    codegen.xor_r8r8(r1,r2);

    tracker.modifiedRegX8(r1);
}

/**
 * Decode 8XY4
 * add vy to vx. set carry to 1 if overflow otherwise 0
 */
void Translator::decode8XY4(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;
	rNode.arg2 = (rNode.opcode & 0x00F0) >> 4;

    setOpcodeFunction(rNode, &Translator::generate8XY4);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 8XY4
 * add vy to vx. set carry to 1 if overflow otherwise 0
 */
void Translator::generate8XY4(const DecodedOpcode &rNode)
{
    const int r3 = tracker.allocRegX8(C8_FLAG_REG, false);
    const int r1 = tracker.allocRegX8(rNode.arg1);
    const int r2 = tracker.allocRegX8(rNode.arg2);

    codegen.add_r8r8(r1,r2);
    codegen.setc_r8(r3);

    tracker.modifiedRegX8(r1);
    tracker.modifiedRegX8(r3);
}

/**
 * Decode 8XY5
 * sub vy from vx. set carry to 1 if no borrow otherwise 0
 */
void Translator::decode8XY5(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;
	rNode.arg2 = (rNode.opcode & 0x00F0) >> 4;

    setOpcodeFunction(rNode, &Translator::generate8XY5);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 8XY5
 * sub vy from vx. set carry to 1 if no borrow otherwise 0
 */
void Translator::generate8XY5(const DecodedOpcode &rNode)
{
    const int r3 = tracker.allocRegX8(C8_FLAG_REG, false);
    const int r1 = tracker.allocRegX8(rNode.arg1);
    const int r2 = tracker.allocRegX8(rNode.arg2);

    codegen.sub_r8r8(r1,r2);
    codegen.setnc_r8(r3);

    tracker.modifiedRegX8(r1);
    tracker.modifiedRegX8(r3);
}

/**
 * Decode 8XY6
 * Shifts VX right by one. VF is set to the value of the least
 * significant bit of VX before the shift.
 */
void Translator::decode8XY6(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generate8XY6);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 8XY6
 * Shifts VX right by one. VF is set to the value of the least
 * significant bit of VX before the shift.
 */
void Translator::generate8XY6(const DecodedOpcode &rNode)
{
    const int r2 = tracker.allocRegX8(C8_FLAG_REG, false);
    const int r1 = tracker.allocRegX8(rNode.arg1);

    codegen.shr1_r8(r1);
    codegen.setc_r8(r2);

    tracker.modifiedRegX8(r1);
    tracker.modifiedRegX8(r2);
}

/**
 * Decode
 * 8XY7 Sets VX to VY minus VX. VF is set to 0 when there's a borrow,
 * and 1 when there isn't.
 */
void Translator::decode8XY7(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;
	rNode.arg2 = (rNode.opcode & 0x00F0) >> 4;

    setOpcodeFunction(rNode, &Translator::generate8XY7);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 8XY7
 * Sets VX to VY minus VX. VF is set to 0 when
 * there's a borrow, and 1 when there isn't.
 */
void Translator::generate8XY7(const DecodedOpcode &rNode)
{
    const int r3 = tracker.allocRegX8(C8_FLAG_REG, false);
    const int r1 = tracker.allocRegX8(rNode.arg1);
    const int r2 = tracker.allocRegX8(rNode.arg2);

    codegen.mov_r8r8(r3, r2);
    codegen.sub_r8r8(r3,r1);
    codegen.mov_r8r8(r1,r3);
    codegen.setnc_r8(r3);

    tracker.modifiedRegX8(r1);
    tracker.modifiedRegX8(r3);
}

/**
 * Decode 8XYE
 * Shifts VX left by one. VF is set to the value of
 * the most significant bit of VX before the shift
 */
void Translator::decode8XYE(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generate8XYE);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 8XYE
 * Shifts VX left by one. VF is set to the value of
 * the most significant bit of VX before the shift
 */
void Translator::generate8XYE(const DecodedOpcode &rNode)
{
    const int r2 = tracker.allocRegX8(C8_FLAG_REG, false);
    const int r1 = tracker.allocRegX8(rNode.arg1);

    codegen.shl1_r8(r1);
    codegen.setc_r8(r2);

    tracker.modifiedRegX8(r1);
    tracker.modifiedRegX8(r2);
}

/**
 * Decode 9XY0
 * skip next instruction if VX != VY
 */
void Translator::decode9XY0(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;
	rNode.arg2 = (rNode.opcode & 0x00F0) >> 4;

    setOpcodeFunction(rNode, &Translator::generate9XY0);
    rNode.inCondition = mCondition;

    if(!mCondition)
    {
       mCondition = true;
       mCountdown = TO_COND_BRANCH;
    }

    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate 9XY0
 * skip next instruction if VX != VY
 */
void Translator::generate9XY0(const DecodedOpcode &rNode)
{
    mLabelCondBranchDest = codegen.newLabel();

    const int r1 = tracker.allocRegX8(rNode.arg1);
    const int r2 = tracker.allocRegX8(rNode.arg2);
    tracker.saveRegisters();

    codegen.cmp_r8r8(r1, r2);
    codegen.jnz(mLabelCondBranchDest);
}

/**
 * Decode ANNN
 * set I to nnn
 */
void Translator::decodeANNN(DecodedOpcode &rNode)
{
	rNode.arg3 = rNode.opcode & 0x0FFF;

    setOpcodeFunction(rNode, &Translator::generateANNN);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate ANNN
 * set I to nnn
 */
void Translator::generateANNN(const DecodedOpcode &rNode)
{
	const int r = tracker.allocRegC16(false);

	codegen.mov_r32i32(r, rNode.arg3);

	tracker.modifiedRegC16();
}

/**
 * Decode BNNN
 * jump to address NNN + V0
 */
void Translator::decodeBNNN(DecodedOpcode &rNode)
{
    //rNode.arg1 = 0;
	rNode.arg3 = rNode.opcode & 0x0FFF;
	rNode.pfnGenOpcode = &Translator::generateBNNN;
    rNode.inCondition = mCondition;
    mReadyToTranslate = !mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate BNNN
 * jump to address NNN + V0
 */
void Translator::generateBNNN(const DecodedOpcode &rNode)
{
    if(rNode.inCondition)
    {
        if(tracker.isAllocatedRegC8(rNode.arg1))
        {
            const int r = tracker.allocRegX8(rNode.arg1);

            if(r != X86_REG_AL)
                codegen.mov_r8r8(X86_REG_AL, r);
        }
        else
        {
            codegen.mov_r32i32(X86_REG_EAX, mC8_regBaseAddr);
            codegen.mov_r8m8(X86_REG_AL, X86_REG_EAX);
        }
    }
    else
    {
        tracker.saveRegisters();
        tracker.allocRegX8(X86_REG_AL, rNode.arg1);
    }

    tracker.restoreDirty();
    codegen.movzx_r32r8(X86_REG_EAX, X86_REG_AL);
    codegen.add_r32i32(X86_REG_EAX, rNode.arg3);

    codegen.ret();
}

/**
 * Decode CXNN
 * set vx to rand & NN
 */
void Translator::decodeCXNN(DecodedOpcode &rNode)
{
	rNode.arg2 = rNode.opcode & 0x00FF;
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generateCXNN);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate CXNN
 * set vx to rand & NN
 */
void Translator::generateCXNN(const DecodedOpcode &rNode)
{
    tracker.allocRegX8(X86_REG_AL, rNode.arg1, false);

    tracker.dirtyRegX32(X86_REG_EDX);

    if(tracker.isAllocatedRegX8(X86_REG_AH))
        for(int i = 1; i < X86_COUNT_REGS_8BIT; i++)
            if(!tracker.isAllocatedRegX8(i))
            {
                tracker.reallocRegX8(X86_REG_AH, i);
                break;
            }

    if(tracker.isAllocatedRegX8(X86_REG_AH))
    {
        tracker.dirtyRegX32(tracker.REG_TMP);
        codegen.mov_r32r32(tracker.REG_TMP, X86_REG_EAX);
    }

    if(tracker.isAllocatedRegX8(X86_REG_DL) || tracker.isAllocatedRegX8(X86_REG_DH))
    {
        if(tracker.isAllocatedRegX8(X86_REG_AH))
            codegen.push_r32(X86_REG_EDX);
        else
        {
            tracker.dirtyRegX32(tracker.REG_TMP);
            codegen.mov_r32r32(tracker.REG_TMP, X86_REG_EDX);
        }
    }

    codegen.mov_r32i32(X86_REG_EAX, LCG_MULTIPLIER);
    codegen.mov_r32i32(X86_REG_EDX, mC8_seedRngAddr);
    codegen.mul_m32(X86_REG_EDX);
    codegen.add_r32i32(X86_REG_EAX, LCG_INCREMENT);
    codegen.mov_r32i32(X86_REG_EDX, mC8_seedRngAddr);
    codegen.mov_m32r32(X86_REG_EDX, X86_REG_EAX);
    codegen.shr_r32i8(X86_REG_EAX, 24);
    codegen.and_r8i8(X86_REG_AL, rNode.arg2);

    if(tracker.isAllocatedRegX8(X86_REG_AH))
    {
        codegen.mov_r8r8(X86_REG_DL, X86_REG_AL);
        codegen.mov_r32r32(X86_REG_EAX, tracker.REG_TMP);
        codegen.mov_r8r8(X86_REG_AL, X86_REG_DL);
    }

    if(tracker.isAllocatedRegX8(X86_REG_DL) || tracker.isAllocatedRegX8(X86_REG_DH))
    {
        if(tracker.isAllocatedRegX8(X86_REG_AH))
            codegen.pop_r32(X86_REG_EDX);
        else
            codegen.mov_r32r32(X86_REG_EDX, tracker.REG_TMP);
    }

    tracker.modifiedRegX8(X86_REG_AL);
}

/**
 * Decode DXYN
 * Draws a sprite at coordinate (VX, VY); that has a width of
 * 8 pixels and a height of N pixels.
 * As described above, VF is set to 1 if any screen pixels are flipped
 * from set to unset when the sprite is drawn, and to 0 if that doesn't happen
 */
void Translator::decodeDXYN(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;
	rNode.arg2 = (rNode.opcode & 0x00F0) >> 4;
	rNode.arg3 = rNode.opcode & 0x000F;

    setOpcodeFunction(rNode, &Translator::generateDXYN);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate DXYN
 * Draws a sprite at coordinate (VX, VY); that has a width of
 * 8 pixels and a height of N pixels.
 * As described above, VF is set to 1 if any screen pixels are flipped
 * from set to unset when the sprite is drawn, and to 0 if that doesn't happen
 */
void Translator::generateDXYN(const DecodedOpcode &rNode)
{
    const int rf = tracker.allocRegX8(X86_REG_AL, C8_FLAG_REG, false);
    const int rx = tracker.allocRegX8(X86_REG_AH, rNode.arg1);
    const int ry = tracker.allocRegX8(X86_REG_BL, rNode.arg2);
    const int ra = tracker.allocRegC16();

    const int rtmp32_x = X86_REG_ECX;
    const int rtmp32_y = tracker.REG_TMP;
    const int rtmp8_cmp = X86_REG_DL;
    const int rtmp8_c = X86_REG_BH;
    const int rtmp8_b = X86_REG_DH;

    tracker.dirtyRegX32(rtmp32_y);
    tracker.dirtyRegX32(rtmp32_x);
    tracker.dirtyRegX8(rtmp8_b);
    tracker.dirtyRegX8(rtmp8_cmp);

    if(rNode.arg3 != 0)
        tracker.dirtyRegX8(rtmp8_c);

    const Label_t loop1 = codegen.newLabel();

    if(tracker.isAllocatedRegX8(X86_REG_DL) || tracker.isAllocatedRegX8(X86_REG_DH))
        codegen.push_r32(X86_REG_EDX);

    if(tracker.isAllocatedRegX8(X86_REG_CL) || tracker.isAllocatedRegX8(X86_REG_CH))
        codegen.push_r32(X86_REG_ECX);

    if(tracker.isAllocatedRegX8(X86_REG_BH) && rNode.arg3 != 0)
        codegen.push_r32(X86_REG_EBX);

    codegen.xor_r8r8(rf, rf);

    if(rNode.arg3 != 0)
    {
        codegen.xor_r8r8(rtmp8_c, rtmp8_c);
        codegen.insertLabel(loop1);
        codegen.movzx_r32r8(tracker.REG_TMP, rtmp8_c);
        codegen.add_r32r32(tracker.REG_TMP, ra);
    }
    else
        codegen.mov_r32r32(tracker.REG_TMP, ra);

    codegen.add_r32i32(tracker.REG_TMP, mC8_memBaseAddr);
    codegen.mov_r8m8(rtmp8_b, tracker.REG_TMP);

    for(int i = 0; i < 8; i++)
    {
        const Label_t zero = codegen.newLabel();
        const Label_t one = codegen.newLabel();

        codegen.movzx_r32r8(rtmp32_y, ry);
        codegen.movzx_r32r8(rtmp32_x, rx);
        codegen.and_r32i32(rtmp32_y, 0x1F);    //reg mod 32
        codegen.and_r32i32(rtmp32_x, 0x3F);    //reg mod 64
        codegen.shl_r32i8(rtmp32_y, 6);        //reg * 64
        codegen.add_r32r32(rtmp32_y, rtmp32_x);
        codegen.add_r32i32(tracker.REG_TMP, mC8_screenBaseAddr);
        codegen.shl1_r8(rtmp8_b);
        codegen.jnc(zero);
        codegen.mov_r8m8(rtmp8_cmp, tracker.REG_TMP);
        codegen.test_r8r8(rtmp8_cmp, rtmp8_cmp);
        codegen.jz(one);
        codegen.or_r8i8(rf, 1);
        codegen.insertLabel(one);
        codegen.xor_m8i8(tracker.REG_TMP, C8_PIXEL_ON);
        codegen.insertLabel(zero);
        codegen.inc_r8(rx);
    }

    codegen.sub_r8i8(rx, 8);

    if(rNode.arg3 != 0)
    {
        codegen.inc_r8(ry);
        codegen.inc_r8(rtmp8_c);
        codegen.cmp_r8i8(rtmp8_c, rNode.arg3);
        codegen.jnz(loop1);
        codegen.sub_r8r8(ry, rtmp8_c);
    }

    codegen.mov_r32i32(tracker.REG_TMP, mC8_newFrameAddr);
    codegen.mov_m32i32(tracker.REG_TMP, NEW_FRAME);

    if(tracker.isAllocatedRegX8(X86_REG_BH) && rNode.arg3 != 0)
        codegen.pop_r32(X86_REG_EBX);

    if(tracker.isAllocatedRegX8(X86_REG_CL) || tracker.isAllocatedRegX8(X86_REG_CH))
        codegen.pop_r32(X86_REG_ECX);

    if(tracker.isAllocatedRegX8(X86_REG_DL) || tracker.isAllocatedRegX8(X86_REG_DH))
        codegen.pop_r32(X86_REG_EDX);

    tracker.modifiedRegX8(rf);
}

/**
 * Decode EX9E
 * Skips the next instruction if the key stored in VX is pressed.
 */
void Translator::decodeEX9E(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generateEX9E);
    rNode.inCondition = mCondition;

    if(!mCondition)
    {
       mCondition = true;
       mCountdown = TO_COND_BRANCH;
    }

	mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate EX9E
 * Skips the next instruction if the key stored in VX is pressed.
 */
void Translator::generateEX9E(const DecodedOpcode &rNode)
{
    mLabelCondBranchDest = codegen.newLabel();

    const int r8 = tracker.allocRegX8(rNode.arg1);
    const int r32 = tracker.temporaryRegX32();

    tracker.dirtyRegX32(r32);
    tracker.saveRegisters();

    const int rt32 = r8 & 0x3; //r8 mod 4
    bool freetmp = false;
    int rtmp8;

    if(!tracker.isAllocatedRegX8(rt32 + 4))
    {
        freetmp = true;
        rtmp8 = rt32 + 4;
    }
    else if(!tracker.isAllocatedRegX8(rt32))
    {
        freetmp = true;
        rtmp8 = rt32;
    }

    codegen.movzx_r32r8(r32, r8);
    codegen.add_r32i32(r32, mC8_keyBaseAddr);

    if(freetmp)
    {
        codegen.mov_r8m8(rtmp8, r32);
        codegen.test_r8r8(rtmp8, rtmp8);
    }
    else
        codegen.cmp_m8i8(r32, 0);

    codegen.jnz(mLabelCondBranchDest);
}

/**
 * Decode EXA1
 * Skips the next instruction if the key stored in VX isn't pressed..
 */
void Translator::decodeEXA1(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generateEXA1);
    rNode.inCondition = mCondition;

    if(!mCondition)
    {
       mCondition = true;
       mCountdown = TO_COND_BRANCH;
    }

	mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate EXA1
 * Skips the next instruction if the key stored in VX isn't pressed..
 */
void Translator::generateEXA1(const DecodedOpcode &rNode)
{
    mLabelCondBranchDest = codegen.newLabel();

    const int r8 = tracker.allocRegX8(rNode.arg1);
    const int r32 = tracker.temporaryRegX32();

    tracker.dirtyRegX32(r32);
    tracker.saveRegisters();

    const int rt32 = r8 & 0x3; //r8 mod 4
    bool freetmp = false;
    int rtmp8;

    if(!tracker.isAllocatedRegX8(rt32 + 4))
    {
        freetmp = true;
        rtmp8 = rt32 + 4;
    }
    else if(!tracker.isAllocatedRegX8(rt32))
    {
        freetmp = true;
        rtmp8 = rt32;
    }

    codegen.movzx_r32r8(r32, r8);
    codegen.add_r32i32(r32, mC8_keyBaseAddr);

    if(freetmp)
    {
        codegen.mov_r8m8(rtmp8, r32);
        codegen.test_r8r8(rtmp8, rtmp8);
    }
    else
        codegen.cmp_m8i8(r32, 0);

   codegen.jz(mLabelCondBranchDest);
}

/**
 * Decode FX07
 * Sets VX to the value of the delay timer
 */
void Translator::decodeFX07(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generateFX07);
    rNode.inCondition = mCondition;
	mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate FX07
 * Sets VX to the value of the delay timer
 */
void Translator::generateFX07(const DecodedOpcode &rNode)
{
    const int r8 = tracker.allocRegX8(rNode.arg1, false);
    const int r32 = tracker.temporaryRegX32();

    tracker.dirtyRegX32(r32);

    codegen.mov_r32i32(r32, mC8_delaytimerAddr);
    codegen.mov_r8m8(r8, r32);

    tracker.modifiedRegX8(r8);
}

/**
 * Decode FX0A
 * A key press is awaited, and then stored in VX.
 */
void Translator::decodeFX0A(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;
	rNode.leader = !mCondition;

	mReadyToTranslate = !mCondition;
    setOpcodeFunction(rNode, &Translator::generateFX0A);
    rNode.inCondition = mCondition;
	mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate FX0A
 * A key press is awaited, and then stored in VX.
 */
void Translator::generateFX0A(const DecodedOpcode &rNode)
{
    const int r32 = tracker.temporaryRegX32();

    tracker.dirtyRegX32(r32);
    tracker.dirtyRegX32(X86_REG_ECX);

    const Label_t lblPRESSED = codegen.newLabel();

    codegen.mov_r32i32(r32, mC8_keyBaseAddr);
    codegen.xor_r8r8(X86_REG_CL, X86_REG_CL);

    for(int i = 0; i < C8_KEY_COUNT; i++)
    {
        codegen.mov_r8m8_d8(X86_REG_CH, r32, i);
        codegen.test_r8r8(X86_REG_CH, X86_REG_CH);
        codegen.jnz(lblPRESSED);
        codegen.inc_r8(X86_REG_CL);
    }
    //loop until i == 16

    tracker.restoreDirty();
    codegen.mov_r32i32(X86_REG_EAX, rNode.address);
    codegen.ret();

    //PRESSED:
    codegen.insertLabel(lblPRESSED);

    codegen.mov_r32i32(r32, mC8_regBaseAddr + rNode.arg1);
    codegen.mov_m8r8(r32, X86_REG_CL);

    tracker.restoreDirty();
    codegen.mov_r32i32(X86_REG_EAX, rNode.address + C8_OPCODE_SIZE);

    codegen.ret();
}

/**
 * Decode FX15
 * delay to vx
 */
void Translator::decodeFX15(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generateFX15);
    rNode.inCondition = mCondition;
	mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate FX15
 * delay to vx
 */
void Translator::generateFX15(const DecodedOpcode &rNode)
{
    const int r8 = tracker.allocRegX8(rNode.arg1);
    const int r32 = tracker.temporaryRegX32();

    tracker.dirtyRegX32(r32);

    codegen.mov_r32i32(r32, mC8_delaytimerAddr);
    codegen.mov_m8r8(r32, r8);
}

/**
 * Decode FX18
 * sound to vx
 */
void Translator::decodeFX18(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generateFX18);
    rNode.inCondition = mCondition;
	mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate FX18
 * sound to vx
 */
void Translator::generateFX18(const DecodedOpcode &rNode)
{
    const int r8 = tracker.allocRegX8(rNode.arg1);
    const int r32 = tracker.temporaryRegX32();

    tracker.dirtyRegX32(r32);

    codegen.mov_r32i32(r32, mC8_soundtimerAddr);
    codegen.mov_m8r8(r32, r8);
}

/**
 * Decode FX1E
 * adds vx to I
 */
void Translator::decodeFX1E(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generateFX1E);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate FX1E
 * adds vx to I
 */
void Translator::generateFX1E(const DecodedOpcode &rNode)
{
    const int r1 = tracker.allocRegC16();
    const int r2 = tracker.allocRegX8(rNode.arg1);
    const int r32 = tracker.temporaryRegX32();

    tracker.dirtyRegX32(r32);

    codegen.movzx_r32r8(r32, r2);
    codegen.add_r32r32(r1,r32);

    tracker.modifiedRegC16();
}

/**
 * Decode FX29
 * Sets I to the location of the sprite for the character in VX.
 * Characters 0-F (in hexadecimal); are represented by a 4x5 font.
 */
void Translator::decodeFX29(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generateFX29);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate FX29
 * Sets I to the location of the sprite for the character in VX.
 * Characters 0-F (in hexadecimal); are represented by a 4x5 font.
 */
void Translator::generateFX29(const DecodedOpcode &rNode)
{
    const int r1 = tracker.allocRegC16(false);
    const int r2 = tracker.allocRegX8(rNode.arg1);
    const int r32 = tracker.temporaryRegX32();

    tracker.dirtyRegX32(r32);

	codegen.movzx_r32r8(r1, r2);
	codegen.mov_r32r32(r32,r1);
	codegen.shl_r32i8(r1,2);
	codegen.add_r32r32(r1,r32);

	tracker.modifiedRegC16();
}

/**
 * Decode FX33
 * Stores the Binary-coded decimal representation of VX at the addresses I,
 * I plus 1, and I plus 2.
 */
void Translator::decodeFX33(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generateFX33);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate FX33
 * Stores the Binary-coded decimal representation of VX at the addresses I,
 * I plus 1, and I plus 2.
 */
void Translator::generateFX33(const DecodedOpcode &rNode)
{
    tracker.allocRegX8(X86_REG_AL, rNode.arg1);
    const int r2 = tracker.allocRegC16();

    bool freetmp = false;
    int r3;

    for(r3 = X86_COUNT_REGS_8BIT - 1; r3 >= 1; r3--)
        if(!tracker.isAllocatedRegX8(r3) && r3 != X86_REG_AH)
        {
            freetmp = true;
            break;
        }

    tracker.dirtyRegX32(tracker.REG_TMP);

    if(!freetmp)
    {
        codegen.push_r32(X86_REG_ECX);
        r3 = X86_REG_CL;
    }
    else
        tracker.dirtyRegX8(r3);

    codegen.mov_r32r32(tracker.REG_TMP, X86_REG_EAX);
    codegen.add_r32i32(r2, mC8_memBaseAddr);
    codegen.xor_r8r8(X86_REG_AH, X86_REG_AH);
    codegen.mov_r8i8(r3, 100);
    codegen.div_r8(r3);
    codegen.mov_m8r8(r2, X86_REG_AL);
    codegen.inc_r32(r2);
    codegen.mov_r8r8(X86_REG_AL, X86_REG_AH);
    codegen.xor_r8r8(X86_REG_AH, X86_REG_AH);
    codegen.mov_r8i8(r3, 10);
    codegen.div_r8(r3);
    codegen.mov_m8r8(r2, X86_REG_AL);
    codegen.inc_r32(r2);
    codegen.mov_m8r8(r2, X86_REG_AH);
    codegen.mov_r32r32(X86_REG_EAX, tracker.REG_TMP);
    codegen.sub_r32i32(r2, mC8_memBaseAddr + 2);

    if(!freetmp)
        codegen.pop_r32(X86_REG_ECX);
}

/**
 * Decode FX55
 * Stores V0 to VX in memory starting at address I.
 */
void Translator::decodeFX55(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generateFX55);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate FX55
 * Stores V0 to VX in memory starting at address I.
 */
void Translator::generateFX55(const DecodedOpcode &rNode)
{
    const int ra = tracker.allocRegC16();

    codegen.add_r32i32(ra, mC8_memBaseAddr);

    for(int i = 0; i <= rNode.arg1; i++)
    {
        if(tracker.isAllocatedRegC8(i) || tracker.getNumberOfFreeX8Regs() > 0)
        {
            const int r = tracker.allocRegX8(i);
            codegen.mov_m8r8(ra, r);
        }
        else
        {
            tracker.dirtyRegX32(tracker.REG_TMP);

            codegen.push_r32(X86_REG_EDX);
            codegen.mov_r32i32(tracker.REG_TMP, mC8_regBaseAddr + i);
            codegen.mov_r8m8(X86_REG_DL, tracker.REG_TMP);
            codegen.mov_m8r8(ra, X86_REG_DL);
            codegen.pop_r32(X86_REG_EDX);
        }

        codegen.inc_r32(ra);
    }

    codegen.sub_r32i32(ra, rNode.arg1 + mC8_memBaseAddr + 1);
}

/**
 * Decode FX65
 * Fills V0 to VX with values from memory starting at address I.
 */
void Translator::decodeFX65(DecodedOpcode &rNode)
{
	rNode.arg1 = (rNode.opcode & 0x0F00) >> 8;

    setOpcodeFunction(rNode, &Translator::generateFX65);
    rNode.inCondition = mCondition;
    mNextOpAddress = rNode.address + C8_OPCODE_SIZE;
}

/**
 * Generate
 * FX65 Fills V0 to VX with values from memory starting at address I.
 */
void Translator::generateFX65(const DecodedOpcode &rNode)
{
    const int ra = tracker.allocRegC16();

    codegen.add_r32i32(ra, mC8_memBaseAddr);

    for(int i = 0; i <= rNode.arg1; i++)
    {
        if(tracker.isAllocatedRegC8(i) || tracker.getNumberOfFreeX8Regs() > 0)
        {
            const int r = tracker.allocRegX8(i, false);
            codegen.mov_r8m8(r, ra);
            tracker.modifiedRegX8(r);
        }
        else
        {
            tracker.dirtyRegX32(tracker.REG_TMP);

            codegen.push_r32(X86_REG_EDX);
            codegen.mov_r32i32(tracker.REG_TMP, mC8_regBaseAddr + i);
            codegen.mov_r8m8(X86_REG_DL, ra);
            codegen.mov_m8r8(tracker.REG_TMP, X86_REG_DL);
            codegen.pop_r32(X86_REG_EDX);
        }

        codegen.inc_r32(ra);
    }

    codegen.sub_r32i32(ra, rNode.arg1 + mC8_memBaseAddr + 1);
}

/**
 * Decodes an opcode
 *
 * PARAMS
 * rNode    decoded info is saved to this node
 */
inline void Translator::decode(DecodedOpcode &rNode)
{
   	switch (rNode.opcode & 0xF000)
	{
		case 0x0000:
			switch (rNode.opcode & 0xF)
            {
                case 0x0: decode00E0(rNode); break;
                case 0xE: decode00EE(rNode); break;
                default:  unknownOpcode(rNode);
            }
            break;
		case 0x1000: decode1NNN(rNode); break;
		case 0x2000: decode2NNN(rNode); break;
		case 0x3000: decode3XNN(rNode); break;
		case 0x4000: decode4XNN(rNode); break;
		case 0x5000: decode5XY0(rNode); break;
		case 0x6000: decode6XNN(rNode); break;
		case 0x7000: decode7XNN(rNode); break;
		case 0x8000:
            switch (rNode.opcode & 0xF)
            {
                case 0x0: decode8XY0(rNode); break;
                case 0x1: decode8XY1(rNode); break;
                case 0x2: decode8XY2(rNode); break;
                case 0x3: decode8XY3(rNode); break;
                case 0x4: decode8XY4(rNode); break;
                case 0x5: decode8XY5(rNode); break;
                case 0x6: decode8XY6(rNode); break;
                case 0x7: decode8XY7(rNode); break;
                case 0xE: decode8XYE(rNode); break;
                default:  unknownOpcode(rNode);
            }
            break;
		case 0x9000: decode9XY0(rNode); break;
		case 0xA000: decodeANNN(rNode); break;
		case 0xB000: decodeBNNN(rNode); break;
		case 0xC000: decodeCXNN(rNode); break;
		case 0xD000: decodeDXYN(rNode); break;
		case 0xE000:
            switch(rNode.opcode & 0xF)
            {
                case 0x1: decodeEXA1(rNode); break;
                case 0xE: decodeEX9E(rNode); break;
                default:  unknownOpcode(rNode);
            }
            break;
		case 0xF000:
            switch(rNode.opcode & 0xFF)
            {
                case 0x07: decodeFX07(rNode); break;
                case 0x0A: decodeFX0A(rNode); break;
                case 0x15: decodeFX15(rNode); break;
                case 0x18: decodeFX18(rNode); break;
                case 0x1E: decodeFX1E(rNode); break;
                case 0x29: decodeFX29(rNode); break;
                case 0x33: decodeFX33(rNode); break;
                case 0x55: decodeFX55(rNode); break;
                case 0x65: decodeFX65(rNode); break;
                default:   unknownOpcode(rNode);
            }
            break;
		default: unknownOpcode(rNode);
	}
}

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
Translator::Translator(uint8_t c8_regArray[C8_GPREG_COUNT],
                       uint32_t *const pC8_seedRngAddr,
                       uint32_t *const pC8_addressReg,
                       uint8_t *const pC8_delaytimer,
                       uint8_t *const pC8_soundtimer,
                       uint32_t *const pC8_newFrame,
                       uint8_t c8_keyArray[C8_KEY_COUNT],
                       uint8_t c8_memArray[C8_MEMSIZE],
                       uint8_t c8_screenMatrix[C8_RES_HEIGHT][C8_RES_WIDTH],
                       uint32_t **const pC8_stackPointer
                      ) : tracker(&codegen, c8_regArray, pC8_addressReg)
{
    mC8_regBaseAddr = (uintptr_t) c8_regArray;
    mC8_seedRngAddr = (uintptr_t) pC8_seedRngAddr;
    mC8_addressRegAddr = (uintptr_t) pC8_addressReg;
    mC8_delaytimerAddr = (uintptr_t) pC8_delaytimer;
    mC8_soundtimerAddr = (uintptr_t) pC8_soundtimer;
    mC8_keyBaseAddr = (uintptr_t) c8_keyArray;
    mC8_memBaseAddr = (uintptr_t) c8_memArray;
    mC8_screenBaseAddr = (uintptr_t) c8_screenMatrix;
    mC8_newFrameAddr = (uintptr_t) pC8_newFrame;
    mC8_stackPointerAddr = (uintptr_t) pC8_stackPointer;

    reset();
}

/**
 * Destructor
 */
Translator::~Translator()
{
    destroy();
}
