#pragma once
#include <atomic>

/**
* \class FlagLock
* \brief A class to lock a given std::atomic_flag.
*
* This class is used to lock the playersInUse flag when we are modifying
* players. It takes a reference to the flag in its constructor, and
* atomically tests and sets the flag. If the flag was previously clear, it sets
* the locked member to true, and if the flag was previously set, it sets the
* locked member to false.
*
* When the object is destroyed, the flag is atomically cleared.
*/
class FlagLock
{
public:

	std::atomic_flag& flag;
	bool locked;

	/*
	* \brief Constructor that takes a reference to an atomic_flag.
	*/
	FlagLock(std::atomic_flag& f);

	/*
	* \brief Destructor that clears the atomic_flag.
	*/
	~FlagLock();
};