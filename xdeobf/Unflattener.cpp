#include "stdafx.h"

// Callback for block optimizer
int Unflattener::func(mblock_t *blk) {
	mba = blk->mba;
	if (mba->maturity == maturity) {
		return 0;
	}

	maturity = mba->maturity;
	dbg("[I] Current maturity: %s\n", mmatToString(maturity));

	if (maturity == MMAT_LOCOPT) {
		success = performSwitchReconstruction();
		mba->verify(true);
		return 1;
	}

	if (success && maturity == MMAT_GLBOPT2) {
		success = performControlFlowReconstruction();
		mba->verify(true);
		return 1;
	}

	return 0;
}

// >>> PHASE #1: dispatcher switch reconstruction

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
	if (!copyCommonBlocks()) {
		return false;
	}
	if (!createSwitch()) {
		return false;
	}
	return true;
}

// Find register used by dispatcher to resolve target block
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
		std::vector<uint64> values; // Values compared against this register
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

// Find first comparison against dispatcher variable and extract it to separate block if necessary
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
		splitBlock(blk, condBegin);
	}

	dispatcherRoot = blk;
	dbg("[I] Dispatcher root is %d\n", dispatcherRoot->serial);
	return true;
}

// Find all blocks that belong to dispatcher and resolve switch cases
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

		mblock_t *jump, *fall;
		minsn_t *tail = blk->tail;

		// TODO: verify if block doesn't do anything else

		if (getBlockCondExits(blk, jump, fall)) {
			if (!tail->l.is_reg() || tail->l.r != dispatcherVar || tail->r.t != mop_n) {
				dbg("[E] Unexpected conditional at dispatcher block (id: %d)\n", id);
				return false;
			}

			uint32 cmp = (uint32)tail->r.nnn->value;

			if (tail->opcode == m_jz) {
				que.push(fall->serial);
				if (!addCase(cmp, jump)) {
					return false;
				}
			} else if (tail->opcode == m_jnz) {
				que.push(jump->serial);
				if (!addCase(cmp, fall)) {
					return false;
				}
			} else {
				que.push(jump->serial);
				que.push(fall->serial);
			}
		} else if (blk->nsucc() == 1) {
			que.push(blk->succ(0));
		}
	}

	dbg("[I] Found %lu dispatcher blocks and %lu dispatcher exit points\n", dispatcherBlocks.size(), keyToTarget.size());
	return true;
}

bool Unflattener::addCase(uint32 key, mblock_t *dst) {
	auto &elem = keyToTarget[key];
	if (elem) {
		msg("[E] Key %u has more than one target blocks (%d, %d)\n", elem->serial, dst->serial);
		return false;
	}
	elem = dst;
	return true;
}

// Sometimes there jump into internal dispatcher nodes, change them to dispatcher root
bool Unflattener::normalizeJumpsToDispatcher() {
	for (int i = 0; i < mba->qty; i++) {
		if (!normalizeJumpsToDispatcher(mba->get_mblock(i))) {
			return false;
		}
	}

	bool ok = true;

	// Now check if we didn't miss anything
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
		return true; // We don't care about jumps from dispatcher blocks
	}

	if (blk->nsucc() == 1) {
		if (shouldNormalize(mba->get_mblock(blk->succ(0)))) {
			//dbg("[I] Changing goto for %d\n", blk->serial);
			forceBlockGoto(blk, dispatcherRoot);
		}
		return true;
	}
	
	mblock_t *jump, *fall;
	if (!getBlockCondExits(blk, jump, fall)) {
		return true;
	}

	bool normJump = shouldNormalize(jump);
	bool normFall = shouldNormalize(fall);

	if (normJump || normFall) {
		normJump = canNormalize(jump);
		normFall = canNormalize(jump);
	}

	if (normJump && normFall) {
		//dbg("[I] Changing conditional into goto for %d\n", blk->serial);
		forceBlockGoto(blk, dispatcherRoot);
	} else if (normJump) {
		//dbg("[I] Changing conditional jump for %d\n", blk->serial);
		setBlockJcc(blk, dispatcherRoot);
	} else if (normFall) {
		//dbg("[I] Changing fallthrough for %d\n", blk->serial);
		insertGotoBlock(blk, dispatcherRoot);
	}

	return true;
}

bool Unflattener::shouldNormalize(mblock_t *blk) {
	return blk != dispatcherRoot && dispatcherBlocks.count(skipGotos(blk));
}

bool Unflattener::canNormalize(mblock_t *blk) {
	return dispatcherBlocks.count(skipGotos(blk));
}

// Different cases may have common blocks what confuses decompiler. Make them unique by copying.
bool Unflattener::copyCommonBlocks() {
	std::set<mblock_t*> used;

	// Mark prologue blocks as used
	mblock_t *blk = mba->get_mblock(0);
	while (blk->nsucc() == 1) {
		used.insert(blk);
		blk = mba->get_mblock(blk->succ(0));
	}

	for (auto &entry : keyToTarget) {
		if (!copyCommonBlocks(used, entry.second)) {
			return false;
		}
	}
	return true;
}

bool Unflattener::copyCommonBlocks(std::set<mblock_t*> &used, mblock_t *root) {
	if (used.count(root)) {
		msg("[W] Multiple keys pointing to block %d\n", root->serial);
		return true;
	}

	std::set<mblock_t*> blocks;
	std::queue<int> que;
	que.push(root->serial);

	while (!que.empty()) {
		int id = que.front();
		mblock_t *blk = mba->get_mblock(id);
		que.pop();

		if (id == mba->qty-1 || dispatcherBlocks.count(blk)) {
			continue;
		}

		if (blk->nsucc() > 2) {
			msg("[W] Switches are currently unsupported (id: %d)\n", root->serial);
			return true;
		}

		if (blocks.insert(blk).second) {
			for (int succ : blk->succset) {
				que.push(succ);
			}
		}
	}

	std::map<mblock_t*, mblock_t*> oldToNew;

	auto mapBlock = [&](mblock_t *blk) {
		auto iter = oldToNew.find(blk);
		return iter != oldToNew.end() ? iter->second : blk;
	};

	for (mblock_t *block : blocks) {
		if (used.insert(block).second) {
			continue; // No need to copy
		}

		if (endsWithCall(block)) {
			msg("[W] Common block %d ends with call\n", block->serial);
			continue; // Messing with calls is not fun
		}

		if (oldToNew.empty()) {
			// Add goto STOP before our copies, so we don't break anything
			insertGotoBlock(mba->get_mblock(mba->qty-2), mba->get_mblock(mba->qty-1));
		}

		mblock_t *copy = copyBlock(block, mba->qty - 1);
		oldToNew[block] = copy;
		used.insert(copy);
	}

	if (oldToNew.empty()) {
		return true;
	}

	for (mblock_t *block : blocks) {
		mblock_t *mapped = mapBlock(block);
		mblock_t *jump, *fall;

		if (block->nsucc() == 1) {
			mblock_t *dst = mapBlock(mba->get_mblock(block->succ(0)));
			if (endsWithCall(mapped)) {
				used.insert(insertGotoBlock(mapped, dst));
			} else {
				forceBlockGoto(mapped, dst);
			}
		} else if (getBlockCondExits(block, jump, fall)) {
			used.insert(insertGotoBlock(mapped, mapBlock(fall)));
			setBlockJcc(mapped, mapBlock(jump));
		}
	}

	//dbg("[I] Copied %lu common blocks for subgraph of %d\n", oldToNew.size(), root->serial);
	return true;
}

// Finally, change dispatcher root into NWAY block (a switch)
bool Unflattener::createSwitch() {
	std::map<mblock_t*, svalvec_t> targetsToKeys;
	for (auto &entry : keyToTarget) {
		targetsToKeys[entry.second].push_back(entry.first);
	}
	targetsToKeys[dispatcherRoot] = {}; // default branch

	mcases_t *cases = new mcases_t();
	for (auto &entry : targetsToKeys) {
		cases->targets.push_back(entry.first->serial);
		cases->values.push_back(entry.second);
	}

	minsn_t *jtbl = new minsn_t(dispatcherRoot->tail->ea);
	jtbl->opcode = m_jtbl;
	jtbl->l.t = mop_r;
	jtbl->l.r = dispatcherVar;
	jtbl->l.size = 4;
	jtbl->r.t = mop_c;
	jtbl->r.c = cases;

	dispatcherRoot->type = BLT_NWAY;
	deleteWholeJcc(dispatcherRoot);
	dispatcherRoot->insert_into_block(jtbl, dispatcherRoot->tail);
	recalculateSuccesors(dispatcherRoot);
	dbg("[I] Switch created\n");
	return true;
}

// >>> PHASE #2: control flow reconstruction

bool Unflattener::performControlFlowReconstruction() {
	if (!rediscoverSwitch()) {
		return false;
	}

	if (!recoverSuccesors(mba->get_mblock(0))) {
		return false;
	}

	for (auto &entry : keyToTarget) {
		if (!recoverSuccesors(entry.second)) {
			return false;
		}
	}

	return true;
}

bool Unflattener::rediscoverSwitch() {
	keyToTarget.clear();
	dispatcherBlocks.clear();
	dispatcherRoot = nullptr;

	mblock_t *blk = mba->get_mblock(0);
	while (blk->nsucc() == 1) {
		blk = mba->get_mblock(blk->succ(0));
	}

	if (blk->type != BLT_NWAY) {
		msg("[E] Dispatcher switch not found (id: %d)\n", blk->serial);
		return false;
	}

	minsn_t *tail = blk->tail;

	if (!tail || tail->opcode != m_jtbl || tail->r.t != mop_c) {
		msg("[E] Dispatcher switch is invalid (id: %d)\n", blk->serial);
		return false;
	}

	if (tail->l.t != mop_r || tail->l.r != dispatcherVar) {
		msg("[E] Unexpected switch (id: %d)\n", blk->serial);
		return false;
	}

	mcases_t *cases = tail->r.c;

	for (size_t i = 0; i < cases->size(); i++) {
		for (sval_t key : cases->values[i]) {
			keyToTarget[uint32(key)] = mba->get_mblock(cases->targets[i]);
		}
	}

	dispatcherRoot = blk;
	dbg("[I] Dispatcher switch is %d\n", dispatcherRoot->serial);
	return true;
}

bool Unflattener::recoverSuccesors(mblock_t *blk) {
	std::queue<int> que;
	std::set<int> seen;
	que.push(blk->serial);

	mblock_t *exitPoint = nullptr;
	int nExitPoints = 0;

	while (!que.empty()) {
		int id = que.front();
		que.pop();

		if (!seen.insert(id).second) {
			continue;
		}

		mblock_t *blk = mba->get_mblock(id);

		for (int next : blk->succset) {
			if (next == dispatcherRoot->serial) {
				exitPoint = blk;
				nExitPoints++;
			} else {
				que.push(next);
			}
		}
	}

	if (nExitPoints == 0) {
		return true;
	}

	if (nExitPoints != 1) {
		msg("[E] Dispatcher switch case has %d exit points (id: %d)\n", nExitPoints, blk->serial);
		return false;
	}

	mblock_t *curBlock = exitPoint;

	while (true) {
		for (minsn_t *ins = curBlock->tail; ins; ins = ins->prev) {
			if (ins->opcode == m_mov && ins->d.t == mop_r && ins->d.r == dispatcherVar) {
				if (ins->l.t != mop_n) {
					msg("[E] Non-numeric assignment to dispatcher variable (id: %d)\n", curBlock->serial);
					return false;
				}
				return setTargetBlock(exitPoint, uint32(ins->l.nnn->value));
			}
		}

		if (curBlock->predset.find(dispatcherRoot->serial) != curBlock->predset.end()) {
			msg("[E] Assignment to dispatcher variable not found (id: %d)\n", curBlock->serial);
			return false;
		}

		if (curBlock->npred() != 1) {
			if (curBlock->npred() != 2) {
				msg("[E] Unexpected predecessors (id: %d)\n", curBlock->serial);
				return false;
			}
			break;
		}

		curBlock = mba->get_mblock(curBlock->pred(0));
	}

	msg("divergeeee %d\n", curBlock->serial);
	return true;
}

bool Unflattener::setTargetBlock(mblock_t *exitPoint, uint32 targetKey) {
	if (!keyToTarget.count(targetKey)) {
		msg("[E] Missing key: %lu (id: %d)\n", targetKey, exitPoint->serial);
		return false;
	}

	if (endsWithJcc(exitPoint)) {
		msg("[E] Exit point is conditional jump (id: %d)\n", exitPoint->serial);
		return false;
	}

	if (exitPoint->type != BLT_1WAY) {
		msg("[E] Unexpected block type (id: %d)\n", exitPoint->serial);
		return false;
	}

	forceBlockGoto(exitPoint, keyToTarget[targetKey]);
	return true;
}
