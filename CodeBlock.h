/************************************************************
  **** CodeBlock.h (header and implementation)
   ***
    ** Author:
     *   Tommy Hellstrom
     *
     * Description:
     *   A class that contain pointer to a block of
     *   machinecode allocated with mmap()
     *
     * Revision history:
     *   When         Who       What
     *   20090528     me        created
     *
     * License information:
     *   GPLv3
     *
     ********************************************************/

//#pragma once
#ifndef _CODEBLOCK_H_
#define _CODEBLOCK_H_

#include <cstdlib>
#include <stdint.h>
#include <sys/mman.h>

class CodeBlock
{
    private:
        void       *pmBlock;
        size_t     size;

    public:
        int         opcount;
        uint32_t    address;
        uint32_t  (*pfnCodeBlock)();

        /**
         * Constructor
         *
         * PARAMS
         * pBlock   pointer to memory allocated with malloc
         * pCode    pointer to code starting point
         * address  the address for the code in the emulated machine
         * opcount  number of emulated opcodes the block contains
         */
        CodeBlock(void *const pBlock, void *const pCode, const uint32_t address, const int opcount, const size_t size)
        {
            pmBlock = pBlock;
            this->address = address;
            this->opcount = opcount;
            this->size = size;
            pfnCodeBlock = (uint32_t(*)()) pCode;
            //pfnCodeBlock = reinterpret_cast<uint32_t(*)()>(reinterpret_cast<uintptr_t>(pCode));
        }

        /**
         * Destructor
         */
        ~CodeBlock()
        {
            if(pmBlock != NULL)
                munmap(pmBlock, this->size);
        }

     // CodeBlock(const CodeBlock&);
     // CodeBlock& CodeBlock=(const CodeBlock&);
};

#endif //_CODEBLOCK_H_
