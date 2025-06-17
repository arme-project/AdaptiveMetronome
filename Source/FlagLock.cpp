#include "FlagLock.h"

FlagLock::FlagLock(std::atomic_flag& f)
	: flag(f),
	locked(!flag.test_and_set())
{
}

FlagLock::~FlagLock()
{
	flag.clear();
}