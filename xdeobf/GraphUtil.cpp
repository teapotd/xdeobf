#include "stdafx.h"

void delMBlockEdge(mblock_t *src, mblock_t *dst) {
	src->succset.del(dst->serial);
	dst->predset.del(src->serial);
}

void addMBlockEdge(mblock_t *src, mblock_t *dst) {
	src->succset.add(dst->serial);
	dst->predset.add(src->serial);
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
		addMBlockEdge(inBlock, dst);

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
	addMBlockEdge(dst, src);

	src->mark_lists_dirty();
	dst->mark_lists_dirty();
	mba->mark_chains_dirty();
	return dst;
}

mblock_t *skipGotos(mblock_t *blk) {
	while (true) {
		minsn_t *insn = getf_reginsn(blk->head);
		if (!insn || insn->opcode != m_goto || insn->l.t != mop_b) {
			return blk;
		}
		blk = blk->mba->get_mblock(insn->l.b);
	}
}

void forceMBlockGoto(mblock_t *src, mblock_t *dst) {
	mbl_array_t *mba = src->mba;

	while (src->nsucc() > 0) {
		mblock_t *other = mba->get_mblock(src->succ(0));
		delMBlockEdge(src, other);
		other->mark_lists_dirty();
	}

	addMBlockEdge(src, dst);

	if (src->tail && is_mcode_jcond(src->tail->opcode)) {
		minsn_t *insn = getJccRealBegin(src->tail);
		while (insn) {
			minsn_t *next = insn->next;
			src->remove_from_block(insn);
			delete insn;
			insn = next;
		}
	}

	if (!src->tail || src->tail->opcode != m_goto) {
		minsn_t *ins = new minsn_t(src->end);
		ins->opcode = m_goto;
		ins->l.t = mop_b;
		ins->l.b = dst->serial;
		src->insert_into_block(ins, src->tail);
	} else {
		src->tail->l.b = dst->serial;
	}

	src->type = BLT_1WAY;
	src->mark_lists_dirty();
	dst->mark_lists_dirty();
}

void setMBlockJcc(mblock_t *src, mblock_t *dst) {
	QASSERT(133702, src->tail && is_mcode_jcond(src->tail->opcode) && src->tail->d.t == mop_b);

	mbl_array_t *mba = src->mba;
	mblock_t *old = mba->get_mblock(src->tail->d.b);

	delMBlockEdge(src, old);
	addMBlockEdge(src, dst);
	src->tail->d.b = dst->serial;

	src->mark_lists_dirty();
	dst->mark_lists_dirty();
	old->mark_lists_dirty();
}
