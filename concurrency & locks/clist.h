#pragma once
#include <mutex>
#include <iostream>
using namespace std;
/// TODO: complete this implementation of a thread-safe (concurrent) sorted
/// linked list of integers

class clist
{
    int linkedlistsize = 0;
    /// a Node consists of a value and a pointer to another node
    struct Node
    {
        int value;
        Node* next;
    };
    
    /// The head of the list is referenced by this pointer
    Node* head = NULL;
    mutable std::mutex clist_mutex;
    
public:
    clist()
    : head(NULL)
    {
    }
    clist(int)
    : head(NULL)
    {
    }
    
    
    /// insert *key* into the linked list if it doesn't already exist; return
    /// true if the key was added successfully.
    /*Previous node begins at head. It will change to the */
    bool insert(int key) 
    {
        std::lock_guard<std::mutex> lock(clist_mutex);
        Node* currentNode = head;
        Node* previousNode = NULL;
        Node* insertNode = new Node();
        insertNode->value = key; //assign value to the key
        
        //if head is null, make insertNode head.
        if(head == NULL){
            head = insertNode;
            head->next = NULL;
            linkedlistsize++;
            return true;
        }else if(head->value > key){ //checks to see if it is inserted in front of head
            insertNode->next = head; //head becomes the next of insert, which in turn becomes the new head
            head = insertNode;
            linkedlistsize++;
            return true;
        }
        while(currentNode != NULL){
            if(currentNode->value > insertNode->value){
                previousNode->next = insertNode; //the previous node now points to insertedNode
                insertNode->next = currentNode; //the insertNode's new
                linkedlistsize++;
                return true;
            }
            else if(currentNode->value == key)
            {
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
        return true;
    }
    /// remove *key* from the list if it was present; return true if the key
    /// was removed successfully.
    bool remove(int key) 
    {
        std::lock_guard<std::mutex> lock(clist_mutex);
        
        Node* currentNode = head;
        Node* previousNode = NULL;
        if(linkedlistsize == 1 && currentNode->value == key){
            head = NULL;
            currentNode = NULL;
            linkedlistsize--;
            delete currentNode;
            return true;
        }
        while(currentNode != NULL){
            if(currentNode->value != key) {
				previousNode = currentNode;
				currentNode = currentNode->next;
			}
			else  if(currentNode->value == key && previousNode== NULL){
					head = currentNode->next;
					delete currentNode;
				}
			else if (currentNode->value == key && previousNode != NULL) {
				previousNode->next == currentNode->next;
				delete currentNode;
			}
			linkedlistsize--;
			return true;
		}
		return false;
    }
    
    /// return true if *key* is present in the list, false otherwise
    bool lookup(int key) const
    {
        std::lock_guard<std::mutex> lock(clist_mutex);
        
        Node* currentNode = head;
        while(currentNode !=  NULL){
            if(currentNode->value == key){
                return true;
            }else{
                currentNode = currentNode->next;
            }
        }
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
        for(size_t i = 1; i < idx; i++)
        {
            currentNode = currentNode->next;
            if(currentNode == NULL){
                //cout << "you are accessing something that does not exist" 
            }
        }
        return currentNode->value;
    }
    void toString() {
        Node* currentNode = head;
        while(currentNode != NULL) {
            std::cout << currentNode->value << " ";
            currentNode = currentNode -> next;
        }
        //std::cout << endl;
    }
    void printList() const
    {
        if (head != NULL)
        {
            printf("%d", head->value);
            Node* curr = head->next;
            while(curr != NULL)
            {
                printf("-->%d", curr->value);
                curr = curr->next;
            }
        }
        printf("\n");
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

