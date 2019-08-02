#pragma once

struct Unflattener : public optblock_t {
	int func(mblock_t *blk);

	bool performSwitchReconstruction();
	bool findDispatcherVar();
	bool extractDispatcherRoot();
	bool processDispatcherSubgraph();
	bool normalizeJumpsToDispatcher();
	bool normalizeJumpsToDispatcher(mblock_t *blk);
	bool copyCommonBlocks();
	bool copyCommonBlocks(std::set<mblock_t*> &used, mblock_t *root);
	bool createSwitch();

	bool addCase(uint32 key, mblock_t *dst);
	bool shouldNormalize(mblock_t *blk);
	bool canNormalize(mblock_t *blk);

	bool performControlFlowReconstruction();
	bool rediscoverSwitch();
	bool recoverSuccesors(mblock_t *blk);
	bool setTargetBlock(mblock_t *exitPoint, uint32 targetKey);

	mbl_array_t *mba;
	mba_maturity_t maturity = MMAT_ZERO;
	mreg_t dispatcherVar = mr_none;
	mblock_t *dispatcherRoot = nullptr;
	std::set<mblock_t*> dispatcherBlocks;
	std::map<uint32, mblock_t*> keyToTarget;
	bool success = false;
};
