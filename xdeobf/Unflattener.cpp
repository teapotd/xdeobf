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
	if (!normalizeJumpsToDispatcher()) {
		return false;
	}
	if (!createSwitch()) {
		return false;
	}

	mba->mark_chains_dirty();
	dumpMbaToFile(mba, "C:\\Users\\teapot\\Desktop\\mba_dump\\done.txt");
	mba->verify(true);
	dbg("[I] Verification passed\n");
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

	minsn_t *condBegin = getJccRealBegin(blk->tail);

	if (condBegin != blk->head) {
		dbg("[I] Dispatcher root contains more than conditional jump, splitting (id: %d)\n", blk->serial);
		splitMBlock(blk, condBegin);
	}

	dispatcherRoot = blk;
	dbg("[I] Dispatcher root is %d\n", dispatcherRoot->serial);
	return true;
}

bool Unflattener::processDispatcherSubgraph() {
	std::queue<int> que;
	que.push(dispatcherRoot->serial);
	dispatcherBlocks.clear();
	keyToTarget.clear();

	while (!que.empty()) {
		int id = que.front();
		mblock_t *blk = mba->get_mblock(id);
		que.pop();

		if (!dispatcherBlocks.insert(blk).second) {
			continue;
		}

		if (blk->nsucc() > 2) {
			dbg("[E] Dispatcher block with more than 2 succesors (id: %d)\n", id);
			return false;
		}

		minsn_t *tail = blk->tail;
		// TODO: verify if block doesn't do anything else

		if (tail && is_mcode_jcond(tail->opcode)) {
			if (!tail->l.is_reg() || tail->l.r != dispatcherVar || tail->r.t != mop_n) {
				dbg("[E] Unexpected conditional at dispatcher block (id: %d)\n", id);
				return false;
			}

			uint32 cmp = (uint32)tail->r.nnn->value;
			int jump, fall;
			getBlockCondExits(blk, jump, fall);

			if (tail->opcode == m_jz) {
				que.push(fall);
				if (!mapTarget(cmp, mba->get_mblock(jump))) {
					return false;
				}
			} else if (tail->opcode == m_jnz) {
				que.push(jump);
				if (!mapTarget(cmp, mba->get_mblock(fall))) {
					return false;
				}
			} else {
				que.push(jump);
				que.push(fall);
			}
		} else if (blk->nsucc() == 1) {
			que.push(blk->succ(0));
		}
	}

	dbg("[I] Found %lu dispatcher blocks and %lu dispatcher exit points\n", dispatcherBlocks.size(), keyToTarget.size());
	return true;
}

bool Unflattener::mapTarget(uint32 key, mblock_t *dst) {
	auto &elem = keyToTarget[key];
	if (elem) {
		msg("[E] Key %u has more than one target blocks (%d, %d)\n", elem->serial, dst->serial);
		return false;
	}
	elem = dst;
	return true;
}

bool Unflattener::normalizeJumpsToDispatcher() {
	for (int i = 0; i < mba->qty; i++) {
		if (!normalizeJumpsToDispatcher(mba->get_mblock(i))) {
			return false;
		}
	}

	bool ok = true;

	for (mblock_t *blk : dispatcherBlocks) {
		if (blk == dispatcherRoot) {
			continue;
		}

		for (int in : blk->predset) {
			if (!dispatcherBlocks.count(mba->get_mblock(in))) {
				msg("[E] Internal dispatcher block %d still has references from outside\n", blk->serial);
				ok = false;
				break;
			}
		}
	}

	return ok;
}

bool Unflattener::normalizeJumpsToDispatcher(mblock_t *blk) {
	if (dispatcherBlocks.count(blk)) {
		return true;
	}

	if (blk->nsucc() == 1) {
		if (shouldNormalize(blk->succ(0))) {
			dbg("[I] Normalizing goto for %d\n", blk->serial);
			forceMBlockGoto(blk, dispatcherRoot);
		}
		return true;
	}

	if (blk->nsucc() != 2) {
		return true;
	}

	int jump, fall;
	getBlockCondExits(blk, jump, fall);

	bool normJump = shouldNormalize(jump);
	bool normFall = shouldNormalize(fall);

	if (normJump || normFall) {
		normJump = canNormalize(jump);
		normFall = canNormalize(jump);
	}

	if (normJump && normFall) {
		dbg("[I] Changing conditional into goto for %d\n", blk->serial);
		forceMBlockGoto(blk, dispatcherRoot);
	} else if (normJump) {
		dbg("[I] Normalizing conditional jump for %d\n", blk->serial);
		setMBlockJcc(blk, dispatcherRoot);
	} else if (normFall) {
		dbg("[I] Normalizing fallthrough for %d\n", blk->serial);
		insertGotoMBlock(blk, dispatcherRoot);
	}

	return true;
}

bool Unflattener::shouldNormalize(int id) {
	mblock_t *blk = mba->get_mblock(id);
	return blk != dispatcherRoot && dispatcherBlocks.count(skipGotos(blk));
}

bool Unflattener::canNormalize(int id) {
	mblock_t *blk = mba->get_mblock(id);
	return dispatcherBlocks.count(skipGotos(blk));
}

bool Unflattener::createSwitch() {
	std::map<mblock_t*, svalvec_t> targetsToKeys;
	for (auto &entry : keyToTarget) {
		targetsToKeys[entry.second].push_back(entry.first);
	}
	targetsToKeys[mba->get_mblock(mba->qty - 1)] = {}; // default branch

	minsn_t *jtbl = new minsn_t(dispatcherRoot->tail->ea);
	delMBlockAllOutgoing(dispatcherRoot);
	deleteWholeJcc(dispatcherRoot);
	dispatcherRoot->type = BLT_NWAY;

	mcases_t *cases = new mcases_t();
	for (auto &entry : targetsToKeys) {
		addMBlockEdge(dispatcherRoot, entry.first);
		cases->targets.push_back(entry.first->serial);
		cases->values.push_back(entry.second);
	}

	jtbl->opcode = m_jtbl;
	jtbl->l.t = mop_r;
	jtbl->l.r = dispatcherVar;
	jtbl->l.size = 4;
	jtbl->r.t = mop_c;
	jtbl->r.c = cases;

	dispatcherRoot->insert_into_block(jtbl, dispatcherRoot->tail);
	dbg("[I] Switch created\n");
	return true;
}
