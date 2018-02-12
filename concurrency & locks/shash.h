#pragma once

/// TODO: complete this implementation of a thread-safe (concurrent) hash
///       table of integers, implemented as an array of linked lists.  In
///       this implementation, each list should have a "sentinel" node that
///       contains the lock, so we can't just reuse the clist implementation
class shash
{
	struct Node
	{
		int value;
		Node *next;
	};
	struct sentinel
	{
		std::mutex lock;
		Node* next;
	};

	sentinel **buckets;
	int bucketamt = 0;
	
public:
	shash(unsigned _buckets)
	{
		bucketamt = (int)_buckets;
		buckets = new sentinel *[_buckets];
		for(int i=0; i<bucketamt; i++) {
			buckets[i] = new sentinel();
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
	
	/// insert *key* into the appropriate linked list if it doesn't already
	/// exist; return true if the key was added successfully.
	bool insert(int key)
	{
		int buck = hash(key);
		buckets[buck]->lock.lock();
//		cout << "insert " << key << endl;
		Node* head = buckets[buck]->next;
		Node* current = head;
		Node* previous = NULL;
		if(head == NULL) {
			Node* insertNode = create(key, NULL);
			buckets[buck]->next = insertNode;
			buckets[buck]->lock.unlock();
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
			buckets[buck]->lock.unlock();
			return true;
		}
		//key is smaller
		else if(previous != NULL && current->value > key) {
			Node* insertNode = create(key, current);
			previous->next = insertNode;
			buckets[buck]->lock.unlock();
			return true;
		}
		//key is bigger
		else if(current->value < key) {
			Node* insertNode = create(key, NULL);
			current->next = insertNode;
			buckets[buck]->lock.unlock();
			return true;
		}
		buckets[buck]->lock.unlock();
		return false;
	}
	
	/// remove *key* from the appropriate list if it was present; return true
	/// if the key was removed successfully.
	bool remove(int key)
	{
		int buck = hash(key);
		buckets[buck]->lock.lock();
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
				buckets[buck]->lock.unlock();
				return true;
			}
			else if(current->value == key && previous != NULL) {
				previous->next = current->next;
				buckets[buck]->lock.unlock();
				return true;
			}
		}
		buckets[buck]->lock.unlock();
		return false;
	}
	
	/// return true if *key* is present in the appropriate list, false
	/// otherwise
	bool lookup(int key) const
	{
		int buck = hash(key);
		buckets[buck]->lock.lock();
		Node* current = buckets[buck]->next;
		Node* previous = NULL;

		while(current != NULL) {
			if(current->value == key) {
				buckets[buck]->lock.unlock();
				return true;
			}
			else {
				current = current->next;
			}
		}
		buckets[buck]->lock.unlock();
		return false;
	}

	//The following are not tested by the given tester but are required for grading
	//No locks are required for these.

	//This refers to the number of buckets not the total number of elements.
	size_t getSize() const
	{
		return bucketamt;
	}

	//This refers to the number of elements in a bucket, not the sentinel node.
	size_t getBucketSize(size_t bucket) const
	{
		Node* head = buckets[bucket]->next;
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

