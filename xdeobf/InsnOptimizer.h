#pragma once

struct InsnOptimizer : public optinsn_t, public minsn_visitor_t {
	int idaapi func(mblock_t *blk, minsn_t *ins, int optflags);
	int idaapi visit_minsn();

	int processHints(mop_t &op);
	int applyPatterns(minsn_t *insn);
};
