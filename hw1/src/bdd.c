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
    
    // printf("Got here with level %d left %d right %d\n", level, left, right);
    
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
        *(bdd_hash_map + hashedIndex) = &*(bdd_nodes + indexOutput);
        
        BDD_NODE * nodePtr2 = *(bdd_hash_map + hashedIndex);
        // printf("Function: nodePtr2 is %d %d %d\n", nodePtr2->level, nodePtr2->left, nodePtr2->right);
        
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
            
            BDD_NODE * ptr1 = bdd_nodes; // Pointer to the base_address
                    
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
                if(helpingPtr == NULL)
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
                    *(bdd_hash_map + modOffset) = &(*(bdd_nodes + indexOutput));
                    
                    // And we can finally return
                    return indexOutput;
                }
                else if(helpingPtr->level == level && helpingPtr->left == left && helpingPtr->right == right)
                {
                    BDD_NODE * ptr1 = bdd_nodes; // Pointer to the base_address
                    
                    // Have to figure out how to calculate the index
                    int address_diff = helpingPtr - ptr1;
            
                    // Don't need to divide
                    
                    // Then that's it that is the index where the entry is located in the array we can just return
                    return address_diff;
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

/**
 * This is the helper function that does the fixings
 * it is very similar to mergeSort in term that it will break it
 * down to the very end and then build it back up
 * 
 * Don't work don't use this code. Use the updated version
 */
// int helper_recusive_function(unsigned char * raster, int left, int right, int passedLevel)
// {
//     // If left is less than right then we will continue to break it down
//     if(left < right)
//     {
//         // Find the middle point
//         int middle = left + (right - left)/2;
        
//         int returnLeft = helper_recusive_function(raster, left, middle, passedLevel - 1);
//         int returnRight = helper_recusive_function(raster, middle + 1, right, passedLevel - 1);
        
//         // After we get the returned value from both we can
//         // make our actual node from those two return value
//         int constructedNodeIndex = bdd_lookup(passedLevel, returnLeft, returnRight);
        
//         return constructedNodeIndex;
//     }
//     // Means that we hit the base_node we have to return the actual value that is in that index
//     // left or right doesn't really matter because they are equal anyway
//     return *(raster + left);
// }

// Function prototypes so they won't argue which one comes first
int helper_recursive_function_split_row(unsigned char * raster, int row, int col, int width, int height, int level, int wholeWidth, int originalWidth, int originalHeight);
int helper_recursive_function_split_col(unsigned char * raster, int row, int col, int width, int height, int level, int wholeWidth, int originalWidth, int originalHeight);

/**
 * This is the helper function that does the fixings
 * we will be breaking down the squares by top and bottom.
 * Then left and right until we reach the individual pixels which then we will
 * go ahead and put it in our table
 */
int helper_recursive_function_split_row(unsigned char * raster, int row, int col, int width, int height, int level, int wholeWidth, int originalWidth, int originalHeight)
{
    // Also get the base case out of the way
    if(width == 1 && height == 1)
    {
        // Before we return what is at that pixel we must compare
        // the row and col index to the original size of the square before being expanded
        if(row < originalHeight && col < originalWidth)
        {
            // If the row and col index falls in the original square's dimension
            // then we can actually return the pixel value
            return *(raster + (wholeWidth * row + col));
        }
        else
        {
            // Else we return 0 because everything outside of the original square are 0
            return 0;
        }
                
        // printf("Split row %d\n", *(raster + (wholeWidth * row + col)));
    }
    else
    {
        // First we calculate our midpoint using height
        int mid = height / 2;
        
        // Then we will proceed to call our recursive cases
        int returnLeft = helper_recursive_function_split_col(raster, row, col, width, mid, level - 1, wholeWidth, originalWidth, originalHeight);
        int returnRight = helper_recursive_function_split_col(raster, row + mid, col, width, mid, level - 1, wholeWidth, originalWidth, originalHeight);
        
        // After those two recursive call returns then we can construct our bdd_nodes
        int constructedNode = bdd_lookup(level, returnLeft, returnRight);
        
        // And return that constructedNode's index
        return constructedNode;
    }
}

/**
 * This one is for splitting across the col this is called first
 */
int helper_recursive_function_split_col(unsigned char * raster, int row, int col, int width, int height, int level, int wholeWidth, int originalWidth, int originalHeight)
{
    // First we will do our base case which is when the width and height
    // are equal then we will calculate the pixel's position in the raster array and make
    // our node from there
    if(width == 1 && height == 1)
    {
        // Before we return what is at that pixel we must compare
        // the row and col index to the original size of the square before being expanded
        if(row < originalHeight && col < originalWidth)
        {
            // If the row and col index are in the original square then we can return the pixel value
            return *(raster + (wholeWidth * row + col));
        }
        else
        {
            // Else we will just return 0
            return 0;
        }
                
        // printf("Split col %d\n", *(raster + (wholeWidth * row + col)));
        // Then we will return the value that is in this current pixel
        // we can get the location of the individual pixel by adding
        // width * row + col which will create the index of where the pixel is located
        // on the 1D array
    }
    else
    {
        // First we calculate the mid point using the width
        int mid = width / 2;
        
        // Then we will proceed with our recursive calls
        int returnLeft = helper_recursive_function_split_row(raster, row, col, mid, height, level - 1, wholeWidth, originalWidth, originalHeight);
        int returnRight = helper_recursive_function_split_row(raster, row, col + mid, mid, height, level - 1, wholeWidth, originalWidth, originalHeight);
        
        // After those two recursive calls return then we can begin making our node
        int constructedNode = bdd_lookup(level, returnLeft, returnRight);
        
        return constructedNode;
    }
}

BDD_NODE *bdd_from_raster(int w, int h, unsigned char *raster) {
    // Try this function next oh lord it looks really hard
    
    // Before we try to construct the bdd from raster we must do some checkings first
    // if the width and height are not valid we return NULL
    if(w < 0 || h < 0)
    {
        return NULL;
    }
    
    // Getting the log of width
    int level = bdd_min_level(w, h);
    
    // Gets exponent to the power of 2 which is the minimal dimension that we need to use
    int dividedBy2 = level / 2;
    
    int dimension = pow2(dividedBy2);
    
    // Call the recursive helping function. We call split row first
    int returnedValue = helper_recursive_function_split_row(raster, 0, 0, dimension, dimension, level, w, w, h);
    
    // // Find the root node using pointer arithemtic
    BDD_NODE * output = bdd_nodes + returnedValue;
    
    // // Then just return the pointer
    return output;  
}

void bdd_to_raster(BDD_NODE *node, int w, int h, unsigned char *raster) {
    // TO BE IMPLEMENTED
}

/**
 * Function that will help me do the post-order traversal
 */
int helping_bdd_serialize_recursive_function(char level, int left, int right, FILE * out, int serial)
{
    // Meaning that we have hit the base_node we will just return left or right
    // because in the previous level we have set left and right to be equal to each
    // other if we are calling the left node 
    if(level == 0)
    {
        int availableSerial = serial;
        
        // We first check if it is in index_map
        // If it is not in the map then we will put it in the map with the serial
        if(*(bdd_index_map + left) == 0)
        {
            // Put it in the corresponding index
            *(bdd_index_map + left) = availableSerial;
            
            // Increment the availableSerial
            availableSerial ++;
            
            // If we did visited this node then we have to write to stdout
            printf("@");
            printf("%c", left);
        }
        // If it has already been visited then we don't do anything
        // and we can just return avaiableSerial
        return availableSerial;
    }
    else
    {
        // However if we are not at the base case then we will have to do some
        // post-order traversal
        
        // The first step is to verify that the current node that we are visiting has
        // not been visited yet
        int returnedIndex = bdd_lookup(level, left, right);
        
        int availableSerial = serial;
        
        // This means that the current node we are visiting hasn't been
        // visited yet then we can proceed with post-order
        if(*(bdd_index_map + returnedIndex) == 0)
        {
            // Before we make our recursive call we have to make sure
            // left is not a leaf-node index
            if(left < BDD_NUM_LEAVES)
            {
                // This means that the left child of the current node is a leaf-node
                // then we have to make a special call
                availableSerial = helping_bdd_serialize_recursive_function(0, left, left, out, availableSerial);
            }
            else
            {
                // If we are here then that means the left child is not a leaf node
                // hence we can make a normal recursive call
                // But we have to get our left child's information first from the table
                BDD_NODE leftPtr = *(bdd_nodes + left);
                
                availableSerial = helping_bdd_serialize_recursive_function(leftPtr.level, leftPtr.left, leftPtr.right, out, availableSerial);
            }
            
            // Then we have to also do it for the right child
            if(right < BDD_NUM_LEAVES)
            {
                // This means that the right child of the current node is a leaf-node
                // then we have to make a special call
                availableSerial = helping_bdd_serialize_recursive_function(0, right, right, out, availableSerial);
            }
            else
            {
                // If we are here then that means the right child is not a leaf node
                // hence we can make a normal recursive call
                // But we have to get our right child's information first from the table
                BDD_NODE rightPtr = *(bdd_nodes + right);
                
                availableSerial = helping_bdd_serialize_recursive_function(rightPtr.level, rightPtr.left, rightPtr.right, out, availableSerial);
            }
            
            // Finally when we get here we will have our next availableSerial
            // after fixing all the children
            // We can finally insert the current node as visited and onto the table
            // and write to stdout our child node's result
            *(bdd_index_map + returnedIndex) = availableSerial;
            
            // Increment availableSerial after using it
            availableSerial ++;
            
            char ascii_level = '@' + level;
            
            printf("%c", ascii_level); // Printing the level we are currently at
            
            int leftSerial = -1;
            int rightSerial = -1;
            
            if(left < BDD_NUM_LEAVES)
            {
                // If the left is a leaf-node then we just have to print the matching serial number
                leftSerial = *(bdd_index_map + left);
                // printf("%c", *(bdd_index_map + left));
            }
            else
            {
                // If it is not a leaf-node then we have to actually look for the serial number
                // using bdd_lookup
                BDD_NODE leftPtr = *(bdd_nodes + left);
                // printf("%c", *(bdd_index_map + bdd_lookup(leftPtr.level, leftPtr.left, leftPtr.right)));
                
                leftSerial = *(bdd_index_map + bdd_lookup(leftPtr.level, leftPtr.left, leftPtr.right));
            }
            
            if(right < BDD_NUM_LEAVES)
            {
                // If the left is a leaf-node then we just have to print the matching serial number
                rightSerial = *(bdd_index_map + right);
                // printf("%c", *(bdd_index_map + right));
            }
            else
            {
                // If it is not a leaf-node then we have to actually look for the serial number
                // using bdd_lookup
                BDD_NODE rightPtr = *(bdd_nodes + right);
                // printf("%c", *(bdd_index_map + bdd_lookup(rightPtr.level, rightPtr.left, rightPtr.right)));
                rightSerial = *(bdd_index_map + bdd_lookup(rightPtr.level, rightPtr.left, rightPtr.right));
            }
            
            // Finally we print the child using special algorithm 
            printSerialNumberChild(leftSerial);
            printSerialNumberChild(rightSerial);
            
            return availableSerial;
        }
        
        // Again if the current node has already been visited
        // then that means the child also has been visited already
        // we can just return the availableSerial
        return availableSerial;
    }
}

int bdd_serialize(BDD_NODE *node, FILE *out) {
    // Oh boi here we go again, this is going to be really interesting
    // When we call bdd_serialize the bdd_nodes table are already filled with the BDD_NODES
    // we can begin working on the algorithm immediately
    
    int result = helping_bdd_serialize_recursive_function(node->level, node->left, node->right, out, 1);
    
    // Meaning that no serial was used therefore something went through
    if(result == 1)
    {
        // Something went wrong
        return -1;
    }
    else
    {
        // If we are here then that means everything went smoothly hence we can return 0
        return 0;
    }
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

/**
 * Determine the minimum number of levels required to cover a raster
 * with a specified width w and height h.
 *
 * @param w  The width of the raster to be covered.
 * @param h  The height of the raster to be covered.
 * @return  The least value l >=0 such that w <= 2^(l/2) and h <= 2^(l/2).
 */
int bdd_min_level(int w, int h)
{
    // Very simple algorithm to find the first minimal level to cover
    // width and height
    int count = 0;
    int workingProduct = 1;
    
    // height is bigger then we have to based on height to find our minimal level
    if(w < h)
    {
        while(workingProduct < h)
        {
            workingProduct *= 2;
            count ++;
        }
    }
    // The width is bigger then we have to based the min level on the width
    else
    {
        while(workingProduct < w)
        {
            workingProduct *= 2;
            count ++;
        }
    }
    
    // If we are here then that means count have the minimal power of 2 we need
    // we need to multiply that by 2 to return the minimal level
    return 2 * count;
}
