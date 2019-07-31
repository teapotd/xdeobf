#include "stdafx.h"

int InsnOptimizer::func(mblock_t *blk, minsn_t *ins) {
	this->mba = blk->mba;
	this->blk = blk;
	this->topins = ins;

	int changes = ins->for_all_insns(*this);

	if (changes > 0) {
		blk->optimize_insn(ins);
		blk->mark_lists_dirty();
		blk->mba->verify(true);
	}

	return changes;
}

int InsnOptimizer::visit_minsn() {
	int changes = processHints(curins->l) + processHints(curins->r);
	if (changes > 0) {
		return changes;
	}
	return applyPatterns(curins);
}

int InsnOptimizer::processHints(mop_t &op) {
	qstring name;
	if (op.t == mop_v) {
		get_ea_name(&name, op.g);
	} else if (op.t == mop_S) {
		op.print(&name);
	} else {
		return 0;
	}

	size_t begin = name.find("ASSUME_ALWAYS_");
	if (begin == qstring::npos) {
		return 0;
	}

	begin += 14;
	size_t end = name.find('_', begin);
	if (end == qstring::npos) {
		msg("[E] Invalid ASSUME_ALWAYS hint value: end mark not found\n");
		return 0;
	}

	qstring valueStr = name.substr(begin, end);
	int64 value;

	try {
		value = std::stoll(valueStr.c_str());
	} catch (std::invalid_argument&) {
		msg("[E] Invalid ASSUME_ALWAYS hint value: invalid value - %s\n", valueStr);
		return 0;
	} catch (std::out_of_range&) {
		msg("[E] Invalid ASSUME_ALWAYS hint value: out of range - %s\n", valueStr);
		return 0;
	}

	//dbg("[I] Substituting ASSUME_ALWAYS operand with %ld\n", value);
	op.make_number(value, op.size);
	return 1;
}

int InsnOptimizer::applyPatterns(minsn_t *insn) {
	return 0;
}
