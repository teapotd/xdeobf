#include "stdafx.h"

void removeEdge(mblock_t *src, mblock_t *dst) {
	src->succset.del(dst->serial);
	dst->predset.del(src->serial);
	src->mark_lists_dirty();
	dst->mark_lists_dirty();
}

void addEdge(mblock_t *src, mblock_t *dst) {
	src->succset.add(dst->serial);
	dst->predset.add(src->serial);
	src->mark_lists_dirty();
	dst->mark_lists_dirty();
}

void removeAllOutgoingEdges(mblock_t *blk) {
	while (blk->nsucc() > 0) {
		mblock_t *other = blk->mba->get_mblock(blk->succ(0));
		removeEdge(blk, other);
	}
}

// Insert new empty block with attributes copied from src
mblock_t *copyBlockEmpty(mblock_t *src, int insertBefore) {
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

// Insert new block with instructions and attributes copied from src
mblock_t *copyBlock(mblock_t *src, int insertBefore) {
	mblock_t *dst = copyBlockEmpty(src, insertBefore);
	dst->flags |= MBL_FAKE;
	for (minsn_t *ins = src->head; ins; ins = ins->next) {
		dst->insert_into_block(new minsn_t(*ins), dst->tail);
	}
	return dst;
}

// Split block before specified instruction
mblock_t *splitBlock(mblock_t *src, minsn_t *splitInsn) {
	mbl_array_t *mba = src->mba;
	mblock_t *dst = copyBlockEmpty(src, src->serial);

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
		addEdge(inBlock, dst);

		if (endsWithGoto(inBlock)) {
			inBlock->tail->l.b = dst->serial;
		} else if (endsWithJcc(inBlock) && inBlock->tail->d.b == src->serial) {
			inBlock->tail->d.b = dst->serial;
		}
	}

	src->predset.clear();
	addEdge(dst, src);
	return dst;
}

// Skip 1WAY blocks containing only gotos
mblock_t *skipGotos(mblock_t *blk) {
	while (true) {
		minsn_t *insn = getf_reginsn(blk->head);
		if (!insn || insn->opcode != m_goto || insn->l.t != mop_b) {
			return blk;
		}
		blk = blk->mba->get_mblock(insn->l.b);
	}
}

// Set block to 1WAY and add/change goto to specified dst
void forceBlockGoto(mblock_t *src, mblock_t *dst) {
	removeAllOutgoingEdges(src);
	addEdge(src, dst);

	if (endsWithJcc(src)) {
		deleteWholeJcc(src);
	}

	if (endsWithGoto(src)) {
		src->tail->l.b = dst->serial;
	} else {
		minsn_t *ins = new minsn_t(src->end);
		ins->opcode = m_goto;
		ins->l.t = mop_b;
		ins->l.b = dst->serial;
		src->insert_into_block(ins, src->tail);
	}

	src->type = BLT_1WAY;
}

void setBlockJcc(mblock_t *src, mblock_t *dst) {
	QASSERT(133702, endsWithJcc(src));
	mbl_array_t *mba = src->mba;
	mblock_t *old = mba->get_mblock(src->tail->d.b);

	removeEdge(src, old);
	addEdge(src, dst);
	src->tail->d.b = dst->serial;
}

// Insert block with a single goto after specified block
bool insertGotoBlock(mblock_t *after, mblock_t *dst) {
	if (endsWithGoto(after)) {
		return false; // Inserting this block is useless then
	}

	removeEdge(after, after->nextb);

	mblock_t *blk = copyBlockEmpty(after, after->serial + 1);
	blk->start = after->end - 1;
	blk->end = after->end;
	blk->type = BLT_1WAY;
	blk->flags |= MBL_FAKE;

	minsn_t *ins = new minsn_t(blk->start);
	ins->opcode = m_goto;
	ins->l.t = mop_b;
	ins->l.b = dst->serial;
	blk->insert_into_block(ins, blk->tail);

	addEdge(after, blk);
	addEdge(blk, dst);
	return true;
}
