#pragma once

struct Unflattener : public optblock_t {
	int func(mblock_t *blk);
	bool performSwitchReconstruction();
	bool findDispatcherVar();
	bool extractDispatcherRoot();
	bool processDispatcherSubgraph();
	bool normalizeJumpsToDispatcher();
	bool normalizeJumpsToDispatcher(mblock_t *blk);
	bool createSwitch();

	bool shouldNormalize(int blk);

	mbl_array_t *mba;
	mba_maturity_t maturity = MMAT_ZERO;
	mreg_t dispatcherVar = mr_none;
	mblock_t *dispatcherRoot = nullptr;
	std::set<mblock_t*> dispatcherBlocks;
	std::map<uint32, mblock_t*> entries;
};
