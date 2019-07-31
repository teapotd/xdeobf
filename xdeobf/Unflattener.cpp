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

	return 0;
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
			// We're looking for jz/jg instructions...
			if (curins->opcode != m_jz && curins->opcode != m_jg && curins->opcode != m_jnz && curins->opcode != m_jle) {
				return 0;
			}

			// ...which compare register against a number
			if (curins->l.t != mop_r || curins->r.t != mop_n) {
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
		int maxSeen = -1;
	};

	dispatcherVar = mr_none;

	Visitor tmp;
	mba->for_all_topinsns(tmp);

	if (tmp.maxSeen < 0) {
		dbg("[I] Dispatcher var not found\n");
		return false;
	}

	JumpInfo &best = tmp.seen[tmp.maxSeen];
	double entropy = best.entropy();

	if (best.values.size() < 2) {
		dbg("[I] Dispatcher var not found\n");
		return false;
	}

	if (entropy < 0.9) {
		dbg("[I] Too low entropy for dispatcher var (%lf)\n", entropy);
		return false;
	}

	dispatcherVar = best.reg;

	qstring name;
	get_mreg_name(&name, dispatcherVar, 4);
	dbg("[I] Dispatcher var is %s\n", name.c_str());
	return true;
}
