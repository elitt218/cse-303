#include "clist.h"

#pragma once
#include<vector>
using namespace std;
/// TODO: complete this implementation of a thread-safe (concurrent) hash
///       table of integers, implemented as an array of linked lists.

class chash
{
	/// The bucket list
	std::vector<clist> buckets;
//	int size = 0;

public:
	chash(unsigned _buckets) : buckets(_buckets)//: buckets((int)_buckets)
	{
//		size = (int)_buckets;
//		clist list = 10;
//		buckets.assign(size, list);
	}

	int hash(int key) const {
	   	int hash = key % buckets.capacity();
		return hash;
//		return key % size;
	}

	/// insert *key* into the appropriate linked list if it doesn't already
	/// exist; return true if the key was added successfully.
	bool insert(int key)
	{
//		cout << "hello !";
		return buckets[hash(key)].insert(key);
/*		int bucketNum =hash(key);
		clist& temp = const_cast<clist&>(buckets[bucketNum]);
		return temp.insert(key);
*/	}
	/// remove *key* from the appropriate list if it was present; return true
	/// if the key was removed successfully.
	bool remove(int key)
	{
//		cout << "rem";
		return buckets[hash(key)].remove(key);
/*		int bucketNum =hash(key);
		clist& temp = const_cast<clist&>(buckets[bucketNum]);
		return temp.remove(key);
*/	}
	/// return true if *key* is present in the appropriate list, false
	/// otherwise
	bool lookup(int key) const
	{
//		cout << "look";
		return buckets[hash(key)].lookup(key);
/*		int bucketNum =hash(key);
		clist& temp = const_cast<clist&>(buckets[bucketNum]);
		return temp.lookup(key);
*/	}

	//The following are not tested by the given tester but are required for grading
	//No locks are required for these.

	//This refers to the number of buckets not the total number of elements.
	size_t getSize() const
	{
//		return 0;
		return buckets.size();
//		return size;
	}
	size_t getBucketSize(size_t bucket) const
	{
//		return 0;
		return buckets[bucket].getSize();
	}
	int getElement(size_t bucket, size_t idx) const
	{
//		return 0;
		return buckets[bucket].getElement(idx);
	}



	//These functions just need to exist, they do not need to do anything
	int getElement(size_t idx) const
	{
		return 0;
	}
};
