#pragma once

struct Unflattener : public optblock_t {
	int func(mblock_t *blk);
	bool performSwitchReconstruction();
	bool findDispatcherVar();
	bool extractDispatcherRoot();
	bool processDispatcherSubgraph();

	mbl_array_t *mba;
	mba_maturity_t maturity = MMAT_ZERO;
	mreg_t dispatcherVar = mr_none;
	mblock_t *dispatcherRoot = nullptr;
	std::set<int> dispatcherBlocks;
	std::map<uint32, int> entries;
};
