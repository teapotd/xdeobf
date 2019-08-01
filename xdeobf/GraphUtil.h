#pragma once

void recalculateSuccesors(mblock_t *block);
mblock_t *copyBlockEmpty(mblock_t *src, int insertBefore);
mblock_t *copyBlock(mblock_t *src, int insertBefore);
mblock_t *splitBlock(mblock_t *src, minsn_t *splitInsn);
mblock_t *skipGotos(mblock_t *blk);
void forceBlockGoto(mblock_t *src, mblock_t *dst);
void setBlockJcc(mblock_t *src, mblock_t *dst);
mblock_t *insertGotoBlock(mblock_t *after, mblock_t *dst);
