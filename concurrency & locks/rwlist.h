#pragma once

#include<pthread.h>
//static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
/// TODO: complete this implementation of a thread-safe (concurrent) sorted
/// linked list of integers, which should use readers/writer locking.

class rwlist
{
    /// a node consists of a value and a pointer to another node
    struct Node
    {
        int value;
        Node* next;
    };
    
    /// The head of the list is referenced by this pointer
    Node* head = NULL;
    int linkedlistsize = 0;
    mutable pthread_rwlock_t rwlock;
    
public:
    rwlist(int)
    : head(NULL)
		{pthread_rwlock_init(&rwlock, NULL);}
    
    
    /// insert *key* into the linked list if it doesn't already exist; return
    /// true if the key was added successfully
    bool insert(int key)
    {
        pthread_rwlock_wrlock(&rwlock);
        Node* currentNode = head;
        Node* previousNode = NULL;
        Node* insertNode = new Node();
        insertNode->value = key; //assign value to the key
        
        //if head is null, make insertNode head.
        if(head == NULL){
            head = insertNode;
            head->next = NULL;
            linkedlistsize++;
            pthread_rwlock_unlock(&rwlock);
            return true;
			
        }else if(head->value > key){ //checks to see if it is inserted in front of head
            insertNode->next = head; //head becomes the next of insert, which in turn becomes the new head
            head = insertNode;
            linkedlistsize++;
            pthread_rwlock_unlock(&rwlock);
            return true;
        }
        while(currentNode != NULL){
            if(currentNode->value > insertNode->value){
                previousNode->next = insertNode; //the previous node now points to insertedNode
                insertNode->next = currentNode; //the insertNode's new
                linkedlistsize++;
                pthread_rwlock_unlock(&rwlock);
                return true;
            }
            else if(currentNode->value == key)
            {
                pthread_rwlock_unlock(&rwlock);
                return false;
            }
            else{
                previousNode = currentNode;
                currentNode = currentNode->next;
            }
        }
        //if exits look then that means that insertNode is the largest and goes at the end
        previousNode->next = insertNode; //the previous node now points to insertedNode
        insertNode->next = NULL; //the insertNode's new
        linkedlistsize++;
        pthread_rwlock_unlock(&rwlock);
        return true;
    }
    /// remove *key* from the list if it was present; return true if the key
    /// was removed successfully.
    bool remove(int key)
    {
        pthread_rwlock_wrlock(&rwlock);
        Node* currentNode = head;
        Node* previousNode = NULL;
        if(linkedlistsize == 1 && currentNode->value == key){
            head = NULL;
            currentNode = NULL;
            linkedlistsize--;
            delete currentNode;
            pthread_rwlock_unlock(&rwlock);
            return true;
        }
        while(currentNode != NULL){
            if(currentNode->value != key) {
                previousNode = currentNode;
                currentNode = currentNode->next;
            }
            else if(currentNode->value == key && previousNode== NULL){
                head = currentNode->next;
                delete currentNode;
            }
            else if (currentNode->value == key && previousNode != NULL) {
                previousNode->next == currentNode->next;
                delete currentNode;
            }
            linkedlistsize--;
            pthread_rwlock_unlock(&rwlock);
            return true;
        }
		pthread_rwlock_unlock(&rwlock);
		return false;
    }
	
    /// return true if *key* is present in the list, false otherwise
    bool lookup(int key) const
    {
        pthread_rwlock_rdlock(&rwlock);
        
        Node* currentNode = head;
        while(currentNode !=  NULL){
            if(currentNode->value == key){
                pthread_rwlock_unlock(&rwlock);
                return true;
            }else{
                currentNode = currentNode->next;
            }
        }
        pthread_rwlock_unlock(&rwlock);
        return false;
    }
    
    
    //The following are not tested by the given tester but are required for grading
    //No locks are required for these.
    size_t getSize() const
    {
        return linkedlistsize;
    }
    int getElement(size_t idx) const
    {
        Node* currentNode = head;
        for(ssize_t i = 1; i < idx; i++)
        {
            currentNode = currentNode->next;
            if(currentNode == NULL){
                //cout << "you are accessing something that does not exist"
            }
        }
        return currentNode->value;
    }
    
    
    //These functions just need to exist, they do not need to do anything
    size_t getBucketSize(size_t bucket) const
    {
        return 0;
    }
    int getElement(size_t bucket, size_t idx) const
    {
        return 0;
    }
};
