/************************************************************
  **** TranslationCache.h (header)
   ***
    ** Author:
     *   Tommy Hellstrom
     *
     * Description:
     *   A cache for translated Chip-8 code
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
#ifndef _TRANSLATIONCACHE_H_
#define _TRANSLATIONCACHE_H_

#include <stdint.h>

#include "Chip8def.h"
#include "CodeBlock.h"

#define CACHESIZE 1048576

class TranslationCache
{
    private:

        CodeBlock *pmBlockTable[C8_MEMSIZE];

        int mBlockCount;

        /**
         * Removes all codeblocks
         */
        void destroy();

    public:

        static const int TABLE_SIZE = C8_MEMSIZE;

        /**
         * Execute block pointed to by PC
         *
         * PARAMS
         * rPC  reference to emulated PC
         *
         * RETURNS
         * true if block exist, otherwise false
         */
        bool execute(uint32_t &rPC) const;

        /**
         * Executes several blocks pointed to by PC
         *
         * PARAMS
         * rPC      reference to emulated PC
         * opcount  least number of opcodes to execute
         *
         * RETURNS
         * true if block exist, otherwise false
         */
        bool executeN(uint32_t &rPC, const int opcount) const;

        /**
         * Insert a codeblock
         *
         * PARAMS
         * pBlock  pointer to CodeBlock
         *
         * RETURNS
         * true if successful, otherwise false
         */
        bool insert(CodeBlock *const pBlock);

        /**
         * Check if block exist at address
         *
         * PARAMS
         * address  address to check
         *
         * RETURNS
         * true if block exists, otherwise false
         */
        bool exists(const uint32_t address) const;

        /**
         * Removes a block from the cache
         *
         * PARAMS
         * address  address to remove
         */
        void remove(const uint32_t address);

        /**
         * Replace block at address
         *
         * PARAMS
         * address  position in cache
         */
        void replace(CodeBlock *const pBlock);

        /**
         * Return number of codeblock in the cache
         *
         * RETURNS
         * number of codeblocks in the cache
         */
        int getNumberOfBlocks() const;

        /**
         * Removes all codeblocks
         */
        void flush();

        /**
         * Constructor
         */
            TranslationCache();

        /**
         * Destructor
         */
            ~TranslationCache();
        //   TranslationCache(const TranslationCache&);
        //   TranslationCache& TranslationCache=(const TranslationCache&);
};

#endif
