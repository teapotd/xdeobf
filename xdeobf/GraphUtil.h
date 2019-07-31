#pragma once

void changeMBlockSuccesor(mblock_t *src, mblock_t *oldDst, mblock_t *newDst);
mblock_t *copyMBlockEmpty(mblock_t *src, int insertBefore);
mblock_t *splitMBlock(mblock_t *src, minsn_t *splitInsn);
