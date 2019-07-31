#include "stdafx.h"

void changeMBlockSuccesor(mblock_t *src, mblock_t *oldDst, mblock_t *newDst) {
	src->succset.del(oldDst->serial);
	oldDst->predset.del(src->serial);
	src->succset.add(newDst->serial);
	newDst->predset.add(src->serial);

	if (src->tail && src->tail->opcode == m_goto) {
		src->tail->l.b = newDst->serial;
	}

	src->mark_lists_dirty();
	oldDst->mark_lists_dirty();
	newDst->mark_lists_dirty();
}

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
	dst->type = BLT_1WAY;

	while (src->head && src->head != splitInsn) {
		minsn_t *cur = src->head;
		src->remove_from_block(cur);
		dst->insert_into_block(new minsn_t(*cur), dst->tail);
		delete cur;
	}

	intvec_t preds = src->predset;
	for (int in : preds) {
		changeMBlockSuccesor(mba->get_mblock(in), src, dst);
	}

	src->predset.add(dst->serial);
	dst->succset.add(src->serial);

	src->mark_lists_dirty();
	dst->mark_lists_dirty();
	mba->mark_chains_dirty();
	//mba->verify(true);
	return src;
}
