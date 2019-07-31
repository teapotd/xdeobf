#include "stdafx.h"

mblock_t *copyMBlockEmpty(mblock_t *src, int insertBefore) {
	mblock_t *dst = src->mba->insert_block(insertBefore);

	// Copy struct members
	dst->flags = src->flags;
	dst->start = src->start;
	dst->end = src->end;
	dst->type = src->type;

	// Copy lists
	dst->dead_at_start = src->dead_at_start;
	dst->mustbuse = src->mustbuse;
	dst->maybuse = src->maybuse;
	dst->mustbdef = src->mustbdef;
	dst->maybdef = src->maybdef;
	dst->dnu = src->dnu;

	// Copy sval_t
	dst->maxbsp = src->maxbsp;
	dst->minbstkref = src->minbstkref;
	dst->minbargref = src->minbargref;

	dst->mark_lists_dirty();
	return dst;
}

mblock_t *splitMBlock(mblock_t *src, minsn_t *splitInsn) {
	mbl_array_t *mba = src->mba;
	mblock_t *dst = copyMBlockEmpty(src, src->serial);

	while (src->head && src->head != splitInsn) {
		minsn_t *cur = src->head;
		src->remove_from_block(cur);
		dst->insert_into_block(cur, dst->tail);
	}

	src->start = (src->head ? src->head->ea : src->end);
	dst->end = src->start;
	dst->type = BLT_1WAY;

	for (int in : src->predset) {
		mblock_t *inBlock = mba->get_mblock(in);

		inBlock->succset.del(src->serial);
		inBlock->succset.add(dst->serial);
		dst->predset.add(in);

		if (inBlock->tail) {
			if (inBlock->tail->opcode == m_goto) {
				inBlock->tail->l.b = dst->serial;
			} else if (is_mcode_jcond(inBlock->tail->opcode)) {
				inBlock->tail->d.b = dst->serial;
			}
		}

		inBlock->mark_lists_dirty();
	}

	src->predset.clear();
	src->predset.add(dst->serial);
	dst->succset.add(src->serial);

	src->mark_lists_dirty();
	dst->mark_lists_dirty();
	mba->mark_chains_dirty();
	return dst;
}
