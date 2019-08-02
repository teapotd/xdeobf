#pragma once

struct Unflattener : public optblock_t {
	int func(mblock_t *blk);

	void performSwitchReconstruction();
	void findDispatcherVar();
	void extractDispatcherRoot();
	void processDispatcherSubgraph();
	void normalizeJumpsToDispatcher();
	void normalizeJumpsToDispatcher(mblock_t *blk);
	void copyCommonBlocks();
	void copyCommonBlocks(std::set<mblock_t*> &used, mblock_t *root);
	void createSwitch();

	void addCase(uint32 key, mblock_t *dst);
	bool shouldNormalize(mblock_t *blk);
	bool canNormalize(mblock_t *blk);

	void performControlFlowReconstruction();
	void rediscoverSwitch();
	void recoverSuccesors(mblock_t *blk);
	void setTargetBlock(mblock_t *exitPoint, uint32 targetKey);

	mbl_array_t *mba;
	mba_maturity_t maturity = MMAT_ZERO;
	mreg_t dispatcherVar = mr_none;
	mblock_t *dispatcherRoot = nullptr;
	std::set<mblock_t*> dispatcherBlocks;
	std::map<uint32, mblock_t*> keyToTarget;
	bool success = false;
};
