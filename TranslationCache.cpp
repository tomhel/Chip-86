/************************************************************
  **** TranslationCache.cpp (implementation of .h)
   ***
    ** Author:
     *   Tommy Hellstrom
     *
     * Description:
     *   A cache for translated code
     *
     * Revision history:
     *   When         Who       What
     *   20090425     me        created
     *
     * License information:
     *   To be decided... possibly GPL
     *
     ********************************************************/

#include "TranslationCache.h"

/**
 * Removes all codeblocks
 */
void TranslationCache::flush()
{
    destroy();
}

/**
 * Removes all codeblocks
 */
void TranslationCache::destroy()
{
    for(int i = 0; i < TABLE_SIZE; i++)
        remove(i);
}

/**
 * Execute block pointed to by PC
 *
 * PARAMS
 * rPC  reference to emulated PC
 *
 * RETURNS
 * true if block exist, otherwise false
 */
bool TranslationCache::execute(uint32_t &rPC) const
{
    if(pmBlockTable[rPC] == NULL)
        return false;

    rPC = pmBlockTable[rPC]->pfnCodeBlock();

    return true;

}

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
bool TranslationCache::executeN(uint32_t &rPC, const int opcount) const
{
    int ops = 0;

    do
    {
        if(pmBlockTable[rPC] == NULL)
            return false;

        ops+=pmBlockTable[rPC]->opcount;

        rPC = pmBlockTable[rPC]->pfnCodeBlock();

    } while(ops < opcount);

    return true;
}

/**
 * Return number of codeblock in the cache
 *
 * RETURNS
 * number of codeblocks in the cache
 */
int TranslationCache::getNumberOfBlocks() const
{
    return mBlockCount;
}

/**
 * Insert a codeblock
 *
 * PARAMS
 * pBlock  pointer to CodeBlock
 *
 * RETURNS
 * true if successful, otherwise false
 */
bool TranslationCache::insert(CodeBlock *const pBlock)
{
    if(pmBlockTable[pBlock->address] == NULL)
    {
        pmBlockTable[pBlock->address] = pBlock;
        mBlockCount++;
        return true;
    }

    return false;
}

/**
 * Check if block exist at address
 *
 * PARAMS
 * address  address to check
 *
 * RETURNS
 * true if block exists, otherwise false
 */
bool TranslationCache::exists(const uint32_t address) const
{
    return pmBlockTable[address] != NULL;
}

/**
 * Removes a block from the cache
 *
 * PARAMA
 * address  address to remove
 */
void TranslationCache::remove(const uint32_t address)
{
    if(pmBlockTable[address] != NULL)
    {
        delete pmBlockTable[address];
        pmBlockTable[address] = NULL;
        mBlockCount--;
    }
}

/**
 * Replace block at address
 *
 * PARAMS
 * address  position in cache
 */
void TranslationCache::replace(CodeBlock *const pBlock)
{
    if(pmBlockTable[pBlock->address] != NULL)
        delete pmBlockTable[pBlock->address];
    else
        mBlockCount++;

    pmBlockTable[pBlock->address] = pBlock;
}

/**
 * Constructor
 */
TranslationCache::TranslationCache()
{
    for(int i = 0; i < TABLE_SIZE; i++)
        pmBlockTable[i] = NULL;

    mBlockCount = 0;
}

/**
 * Destructor
 */
TranslationCache::~TranslationCache()
{
    destroy();
}
