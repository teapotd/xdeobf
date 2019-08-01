#pragma once

void removeEdge(mblock_t *src, mblock_t *dst);
void addEdge(mblock_t *src, mblock_t *dst);
void removeAllOutgoingEdges(mblock_t *blk);
mblock_t *copyBlockEmpty(mblock_t *src, int insertBefore);
mblock_t *copyBlock(mblock_t *src, int insertBefore);
mblock_t *splitBlock(mblock_t *src, minsn_t *splitInsn);
mblock_t *skipGotos(mblock_t *blk);
void forceBlockGoto(mblock_t *src, mblock_t *dst);
void setBlockJcc(mblock_t *src, mblock_t *dst);
bool insertGotoBlock(mblock_t *after, mblock_t *dst);
