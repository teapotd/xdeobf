#pragma once

mblock_t *copyMBlockEmpty(mblock_t *src, int insertBefore);
mblock_t *splitMBlock(mblock_t *src, minsn_t *splitInsn);
