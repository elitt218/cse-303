#pragma once
//#include<mutex>
#include<pthread.h>
#include <iostream>
using namespace std;
/// TODO: complete this implementation of a thread-safe (concurrent) hash
///       table of integers, implemented as an array of linked lists.  In
///       this implementation, each list should have a "sentinel" node that
///       contains the lock, so we can't just reuse the clist implementation.
///       In addition, the API now allows for multiple keys on each
///       operation.
class shash2
{
	struct Node
	{
		int value;
		Node *next;
	};
	struct sentinel
	{
//		mutable std::mutex lock;
		pthread_rwlock_t rwlock;
		Node* next;
	};

	sentinel **buckets;
	int bucketamt = 0;
	
public:
	shash2(unsigned _buckets)
		{
			bucketamt = (int)_buckets;
			buckets = new sentinel *[_buckets];
			for(int i=0; i<bucketamt; i++) {
				buckets[i] = new sentinel();
				pthread_rwlock_init(&buckets[i]->rwlock, NULL);
			}
		}
	int hash(int key) const
		{
			return key % bucketamt;
		}

	Node* create(int value, Node* next)
		{
			Node* newNode = (Node*)malloc(sizeof(Node));
			newNode->value = value;
			newNode->next = next;
			return newNode;
		}

	bool insertIt(int key)
		{
			int buck = hash(key);
			pthread_rwlock_wrlock(&buckets[buck]->rwlock);
//			cout << "insert " << key << endl;
			Node* head = buckets[buck]->next;
			Node* current = head;
			Node* previous = NULL;
			if(head == NULL) {
				Node* insertNode = create(key, NULL);
				buckets[buck]->next = insertNode;
				pthread_rwlock_unlock(&buckets[buck]->rwlock);
				return true;
			}

			while(current->next != NULL && current->value < key) {
				previous = current;
				current = current->next;
			}
			//insert before  head
			if(previous == NULL && current->value > key) {
				Node* insertNode = create(key, head);
				buckets[buck]->next = insertNode;
				pthread_rwlock_unlock(&buckets[buck]->rwlock);
				return true;
			}
			//key is smaller
			else if(previous != NULL && current->value > key) {
				Node* insertNode = create(key, current);
				previous->next = insertNode;
				pthread_rwlock_unlock(&buckets[buck]->rwlock);
				return true;
			}
			//key is bigger
			else if(current->value < key) {
				Node* insertNode = create(key, NULL);
				current->next = insertNode;
				pthread_rwlock_unlock(&buckets[buck]->rwlock);
				return true;
			}
			pthread_rwlock_unlock(&buckets[buck]->rwlock);
			return false;
		}

	bool removeIt(int key)
		{
			int buck = hash(key);
			pthread_rwlock_wrlock(&buckets[buck]->rwlock);
			Node* current = buckets[buck]->next;
			Node* previous = NULL;

			Node* head = buckets[buck]->next;

			while(current != NULL) {
				if(current->value != key) {
					previous = current;
					current = current->next;
				}
				else if(current->value == key && previous == NULL) {
					buckets[buck]->next = current->next;
					pthread_rwlock_unlock(&buckets[buck]->rwlock);
					return true;
				}
				else if(current->value == key && previous != NULL) {
					previous->next = current->next;
					pthread_rwlock_unlock(&buckets[buck]->rwlock);
					return true;
				}
			}
			pthread_rwlock_unlock(&buckets[buck]->rwlock);
			return false;

		}

	bool lookupIt(int key) const
		{
			int buck = hash(key);
			pthread_rwlock_rdlock(&buckets[buck]->rwlock);
			Node* current = buckets[buck]->next;
			Node* previous = NULL;

			while(current != NULL) {
				if(current->value == key) {
					pthread_rwlock_unlock(&buckets[buck]->rwlock);
					return true;
				}
				else {
					current = current->next;
				}
			}
			pthread_rwlock_unlock(&buckets[buck]->rwlock);
			return false;

		}
	/// insert /num/ values from /keys/ array into the hash, and return the
	/// success/failure of each insert in /results/ array.
	void insert(int* keys, bool* results, int num)
	{
		for(int i=0; i<num; i++) {
			results[i] = insertIt(keys[i]);
		}
	}
	/// remove *key* from the list if it was present; return true if the key
	/// was removed successfully.
	void remove(int* keys, bool* results, int num)
	{
		for(int i=0; i<num; i++) {
			results[i] = removeIt(keys[i]);
		}
	}
	/// return true if *key* is present in the list, false otherwise
	void lookup(int* keys, bool* results, int num) const
	{
		for(int i=0; i<num; i++) {
			results[i] = lookupIt(keys[i]);
		}
	}

	//The following are not tested by the given tester but are required for grading
	//No locks are required for these.

	//This refers to the number of buckets not the total number of elements.
	size_t getSize() const
	{
		return bucketamt;
	}

	//This refers to the number of elements in a bucket, not the sentinel node.
	size_t getBucketSize(size_t idx) const
	{
		Node* head = buckets[idx]->next;
		int size = 0;
		while(head) {
			size++;
			head = head->next;
		}
		return size;
	}
	int getElement(size_t bucket, size_t idx) const
	{
		Node* current = buckets[bucket]->next;
		for(size_t i = 0; i < idx; i++)
		{
			current = current->next;
			if(current == NULL){
				//cout << "you are accessing something that does not
				//exist"
			}
		}
		return current->value;
	}


	//These functions just need to exist, they do not need to do anything
	int getElement(size_t idx) const
	{
		return 0;
	}
};
