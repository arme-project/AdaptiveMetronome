#include "FlagLock.h"

// Constructor that takes a reference to an atomic_flag.
FlagLock::FlagLock(std::atomic_flag& f)
	: flag(f),
	locked(!flag.test_and_set())
{
}

// Destructor that clears the atomic_flag.
FlagLock::~FlagLock()
{
	flag.clear();
}