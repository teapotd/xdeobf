#include "stdafx.h"

int Unflattener::func(mblock_t *blk) {
	mba = blk->mba;
	if (mba->maturity == maturity) {
		return 0;
	}

	maturity = mba->maturity;
	dbg("[I] Current maturity: %s\n", mmatToString(maturity));

	if (maturity == MMAT_LOCOPT) {
		return performSwitchReconstruction();
	}
	return 0;
}

int Unflattener::performSwitchReconstruction() {
	if (!findDispatcherVar()) {
		return 0;
	}

	if (!extractDispatcherRoot()) {
		return 0;
	}

	return 1;
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
