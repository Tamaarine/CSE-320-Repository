#include <stdlib.h>
#include <stdio.h>

#include "bdd.h"
#include "debug.h"
#include "helpingFunctions.h"

/*
 * Macros that take a pointer to a BDD node and obtain pointers to its left
 * and right child nodes, taking into account the fact that a node N at level l
 * also implicitly represents nodes at levels l' > l whose left and right children
 * are equal (to N).
 *
 * You might find it useful to define macros to do other commonly occurring things;
 * such as converting between BDD node pointers and indices in the BDD node table.
 */
#define LEFT(np, l) ((l) > (np)->level ? (np) : bdd_nodes + (np)->left)
#define RIGHT(np, l) ((l) > (np)->level ? (np) : bdd_nodes + (np)->right)

/**
 * Look up, in the node table, a BDD node having the specified level and children,
 * inserting a new node if a matching node does not already exist.
 * The returned value is the index of the existing node or of the newly inserted node.
 *
 * The function aborts if the arguments passed are out-of-bounds.
 */
int bdd_lookup(int level, int left, int right) {
    // Try this function first because it looks like the easiest one out of the three
    
    // This is what we are returning to the caller
    int indexOutput = 0;
    
    // So the steps we are going to take is that
    // 1. If the left and right index are equal to each other then no new node is created
    // we are just going to return left or right either one doesn't matter
    if(left == right)
    {
        // We can just return left or right doesn't matter because we don't
        // want to have redundancy
        return left;
    }
    
    // 2. If left != right index then we have to check the hash table using the
    // hash function to see if any previous entry is found, if it is found then we used
    // that one and return that -> We might have to do linear probing if the entry
    // the index is mapped to isn't that entry to see if we can find it and we also have
    // to do wrap arounds
    
    // Okay first let's call our hasing function to get our hashed index
    int hashedIndex = hashFunction(left, right);
    
    // Then we check in hashtable at hashedIndex to see if there is any entry
    if(*(bdd_hash_map + hashedIndex) == NULL)
    {
        // This means that the index at that hash_map is empty which means
        // the node doesn't exist yet hence we can proceed to insert it into
        // the array table
        
        // Let's make a new BDD NODE with the given specification first
        BDD_NODE toInsert;
        toInsert.level = level;
        toInsert.left = left;
        toInsert.right = right;
        
        // Insert it into the table
        *(bdd_nodes + free_node_counter) = toInsert;
        
        // Before we increment free_node_counter we put it as output
        indexOutput = free_node_counter;
        
        // Also increment the free_node_counter
        free_node_counter ++;
        
        // Then we also have to insert it into the hash table with the address of the BDD NODE we just created
        *(bdd_hash_map + hashedIndex) = &(*(bdd_nodes + indexOutput));
        
        BDD_NODE * nodePtr2 = *(bdd_hash_map + hashedIndex);
        printf("Function: nodePtr2 is %d %d %d\n", nodePtr2->level, nodePtr2->left, nodePtr2->right);
        
        // Then we can return indexOutput
        return indexOutput;
    }
    else
    {
        // If we are here then that means an entry in the hashfunction occupied here
        // we need to check if it matches our node
        
        // Making the nodePtr
        BDD_NODE * nodePtr = *(bdd_hash_map + hashedIndex);
        
        // Now if we access the nodeptr we can check it's value
        // if the hashedIndex's node pointer is equal to the node we are looking
        // then we must return that nodePtr's index in the table
        if(nodePtr->level == level && nodePtr->left == left && nodePtr->right == right)
        {
            // Have to figure out how to calculate the index
            // if we have found the entry at hashedIndex
            // The formula is simple, what is stored in nodePtr subtract by the base address
            // Divide by the size of the BDD_NODE
            // printf("nodePtr using & is %d\n", &nodePtr);
            
            BDD_NODE * ptr1 = &*bdd_nodes; // Pointer to the base_address
                    
            // Have to figure out how to calculate the index
            int address_diff = nodePtr - ptr1;
            
            // Don't need to divide by sizeof struct because it is already done for us in the subtraction
            
            // Then that's it that is the index where the entry is located in the array we can just return
            return address_diff;
        }
        // This means that the hashedIndex's node doesn't match with the one we are looking for
        // hence we have to start a linear probing to look for it
        else
        {
            // i starts at 1 because we are skipping the hashedIndex entry because we checked already
            // it didn't match
            for(int i=1;i<BDD_HASH_SIZE;i++)
            {
                // First we have to calculate the total offset we are off
                int sumOffset = hashedIndex + i;
                
                // Then we have to mod that by BDD_HASH_SIZE to handle the wrap arounds
                int modOffset = sumOffset % BDD_HASH_SIZE;
                
                // Then we can check if that entry match by getting the pointer on that index
                BDD_NODE * helpingPtr = *(bdd_hash_map + modOffset);
                
                // If this is true then we have got a match
                if(helpingPtr->level == level && helpingPtr->left == left && helpingPtr->right == right)
                {
                    BDD_NODE * ptr1 = &*bdd_nodes; // Pointer to the base_address
                    
                    // Have to figure out how to calculate the index
                    int address_diff = helpingPtr - ptr1;
            
                    // Don't need to divide
                    
                    // Then that's it that is the index where the entry is located in the array we can just return
                    return address_diff;
                }
                else if(helpingPtr == NULL)
                {
                    // If that entry is an empty entry then we have to insert it into that position
                    // with the free_node_counter index
                    
                    // Making our toInsert BDD_NODE
                    BDD_NODE toInsert = {level, left, right};
                    
                    // Put it in our array of nodes
                    *(bdd_nodes + free_node_counter) = toInsert;
                    
                    // Set our return value
                    indexOutput = free_node_counter;
                    
                    // Increment our node_counter
                    free_node_counter ++;
                    
                    // Then we also set our helpingPtr to the address of the new node we just created
                    helpingPtr = &(*(bdd_nodes + indexOutput));
                    
                    // And we can finally return
                    return indexOutput;
                }
                
                // However, if that entry doesn't match then we have to go to the next iteration and check
                
            }
            
            // If we are here meaning that we have checked every single spot and we couldn't find
            // the entry node. The table is full but is impossible because we have more
            // than enough space hence we can just leave this as it is. We will never get here
        }
    }
    
    // 3. Finally if the entry is not found in the hash table we have to store it in the table
    // and insert it into the hash table where it is suppose to be mapped to
    // and update the free_node_counter 
    
    // We will never get here don't worry
    return indexOutput;
}

BDD_NODE *bdd_from_raster(int w, int h, unsigned char *raster) {
    // Try this function next oh lord it looks really hard
    BDD_NODE node = {'0', 1,2};
    
    
    
        
    
    
    return NULL;
}

void bdd_to_raster(BDD_NODE *node, int w, int h, unsigned char *raster) {
    // TO BE IMPLEMENTED
}

int bdd_serialize(BDD_NODE *node, FILE *out) {
    // TO BE IMPLEMENTED
    return -1;
}

BDD_NODE *bdd_deserialize(FILE *in) {
    // TO BE IMPLEMENTED
    return NULL;
}

unsigned char bdd_apply(BDD_NODE *node, int r, int c) {
    // TO BE IMPLEMENTED
    return 0;
}

BDD_NODE *bdd_map(BDD_NODE *node, unsigned char (*func)(unsigned char)) {
    // TO BE IMPLEMENTED
    return NULL;
}

BDD_NODE *bdd_rotate(BDD_NODE *node, int level) {
    // TO BE IMPLEMENTED
    return NULL;
}

BDD_NODE *bdd_zoom(BDD_NODE *node, int level, int factor) {
    // TO BE IMPLEMENTED
    return NULL;
}
