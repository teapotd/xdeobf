#include "stdafx.h"

int Unflattener::func(mblock_t *blk) {
	mba = blk->mba;
	if (mba->maturity == maturity) {
		// We're manipulating the whole graph, so just report "change" for each block
		return (maturity == MMAT_LOCOPT);
	}

	maturity = mba->maturity;
	dbg("[I] Current maturity: %s\n", mmatToString(maturity));

	if (maturity == MMAT_LOCOPT) {
		performSwitchReconstruction();
		return 1;
	}
	return 0;
}

bool Unflattener::performSwitchReconstruction() {
	if (!findDispatcherVar()) {
		return false;
	}
	if (!extractDispatcherRoot()) {
		return false;
	}
	if (!processDispatcherSubgraph()) {
		return false;
	}
	return true;
}

bool Unflattener::findDispatcherVar() {
	struct JumpInfo {
		double entropy() {
			int ones = 0;
			for (uint64 x : values) {
				ones += bitCount(uint32(x));
			}
			double balance = double(ones) / double(values.size() * 32);
			return 1.0 - abs(balance - 0.5);
		}

		mreg_t reg;
		std::vector<uint64> values;
	};

	struct Visitor : public minsn_visitor_t {
		int visit_minsn() {
			// We're looking for conditional jumps which compare register against a number
			if (!is_mcode_jcond(curins->opcode) || !curins->l.is_reg() || curins->r.t != mop_n) {
				return 0;
			}

			for (int i = 0; i < int(seen.size()); i++) {
				if (seen[i].reg == curins->l.r) {
					seen[i].values.push_back(curins->r.nnn->value);
					if (seen[i].values.size() > seen[maxSeen].values.size()) {
						maxSeen = i;
					}
					return 0;
				}
			}

			seen.push_back({ curins->l.r, {curins->r.nnn->value} });
			if (maxSeen == -1) {
				maxSeen = 0;
			}
			return 0;
		}

		std::vector<JumpInfo> seen;
		int maxSeen = -1; // Index of variable seen the most times
	};

	dispatcherVar = mr_none;

	Visitor tmp;
	mba->for_all_topinsns(tmp);

	if (tmp.maxSeen < 0) {
		msg("[E] Dispatcher var not found\n");
		return false;
	}

	JumpInfo &best = tmp.seen[tmp.maxSeen];
	double entropy = best.entropy();

	if (best.values.size() < 2) {
		msg("[E] Dispatcher var not found\n");
		return false;
	}

	if (entropy < 0.9) {
		msg("[E] Too low entropy for dispatcher var (%lf)\n", entropy);
		return false;
	}

	dispatcherVar = best.reg;

	qstring name;
	get_mreg_name(&name, dispatcherVar, 4);
	dbg("[I] Dispatcher var is %s\n", name.c_str());
	return true;
}

bool Unflattener::extractDispatcherRoot() {
	mblock_t *blk = mba->get_mblock(0);
	while (blk->nsucc() == 1) {
		blk = mba->get_mblock(blk->succ(0));
	}

	if (blk->nsucc() != 2) {
		msg("[E] Unexpected succesor count for dispatcher root (id: %d, nsucc: %d)\n", blk->serial, blk->nsucc());
		return false;
	}

	if (!blk->tail) {
		msg("[E] Suspected dispatcher root is empty (id: %d)\n", blk->serial);
		return false;
	}

	if (!is_mcode_jcond(blk->tail->opcode)) {
		msg("[E] Suspected dispatcher root doesn't end with conditional jump (id: %d)\n", blk->serial);
		return false;
	}

	if (!blk->tail->l.is_reg() || blk->tail->l.r != dispatcherVar) {
		msg("[E] Suspected dispatcher root doesn't compare against dispatcher var (id: %d)\n", blk->serial);
		return false;
	}

	// Jump is preceded by instructions setting condition codes, find first of them
	minsn_t *condBegin = blk->tail;
	while (condBegin->prev && is_mcode_set(condBegin->prev->opcode)) {
		condBegin = condBegin->prev;
	}

	if (condBegin != blk->head) {
		dbg("[I] Dispatcher root contains more than conditional jump, splitting (id: %d)\n", blk->serial);
		splitMBlock(blk, condBegin);
	}

	dispatcherRoot = blk->serial;
	dbg("[I] Dispatcher root is %d\n", dispatcherRoot);
	return true;
}

bool Unflattener::processDispatcherSubgraph() {
	std::queue<int> que;
	que.push(dispatcherRoot);
	dispatcherBlocks.clear();
	entries.clear();

	while (!que.empty()) {
		int id = que.front();
		que.pop();

		if (!dispatcherBlocks.insert(id).second) {
			continue;
		}

		mblock_t *blk = mba->get_mblock(id);
		minsn_t *tail = blk->tail;

		if (blk->nsucc() > 2) {
			dbg("[E] Dispatcher block with more than 2 succesors (id: %d)\n", id);
			return false;
		}

		// TODO: verify if block doesn't do anything else

		if (tail && is_mcode_jcond(tail->opcode)) {
			if (!tail->l.is_reg() || tail->l.r != dispatcherVar || tail->r.t != mop_n) {
				dbg("[E] Unexpected conditional at dispatcher block (id: %d)\n", id);
				return false;
			}

			int jmpTarget = tail->d.b;
			uint32 cmp = (uint32)tail->r.nnn->value;
			int dst1 = blk->succ(0), dst2 = blk->succ(1);

			if (tail->opcode == m_jz) {
				que.push(jmpTarget != dst1 ? dst1 : dst2);
				entries[cmp] = (jmpTarget != dst1 ? dst2 : dst1);
			} else if (tail->opcode == m_jnz) {
				que.push(jmpTarget == dst1 ? dst1 : dst2);
				entries[cmp] = (jmpTarget == dst1 ? dst2 : dst1);
			} else {
				que.push(dst1);
				que.push(dst2);
			}
		} else if (blk->nsucc() == 1) {
			que.push(blk->succ(0));
		}
	}

	return true;
}
