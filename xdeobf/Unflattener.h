#pragma once

struct Unflattener : public optblock_t {
	int func(mblock_t *blk);
	int performSwitchReconstruction();
	bool findDispatcherVar();
	bool extractDispatcherRoot();

	mbl_array_t *mba;
	mba_maturity_t maturity = MMAT_ZERO;
	mreg_t dispatcherVar = mr_none;
	int dispatcherRoot = -1;
};
