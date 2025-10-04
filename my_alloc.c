#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#define META_SIZE sizeof(meta_block)


typedef struct meta_block
{
    size_t size;
    struct meta_block *next;
    int free;   // mark if it's free or not
    
}meta_block;


// ------------------------------------------------------------------------


meta_block* global_base = NULL;



// to look for free block
meta_block* find_free_block(meta_block** last, size_t size)
{
    meta_block* curr = global_base;
    // meta_block* prev = NULL;
    // replace prev with 'last'

    while(curr && !(curr->free && curr->size >= size)) //* This is where we'll bring in the fitting check (currently it's first fit)
    {
        *last = curr;  //? "*last" is the pointer, "last" is the pointer to that pointer
        curr = curr->next;
    }

    return curr;

}
// to request for free block
meta_block* request_block(meta_block* last, size_t size)
{
    meta_block* block = NULL;
    
    block = sbrk(0); // current program break
    
    //* *//
    void* request = sbrk(size + META_SIZE); // gives the starting part
    //* *//

    // just for debugging help
    assert((void*)block == request);
    
    if(request == (void*)-1)  //* (void*)-1 is what sbrk returns on its failure, Nothing Special
    {
        return NULL;
    }

    if(last)
    {
        last->next = block;
    }

    block->size = size;
    block->next = NULL; // marks it as 'the last block'
    block->free = 0;

    return block;
}


void* my_alloc(int size)
{
    // Naive way
    // return sbrk(size);



    meta_block* block;

    if(size < 0)
    {
        // Makes no sense
        return NULL;
    }

    // Now, we have two cases:  (1) If it's the first call to my_alloc [which means global_base is still NULL]    (2) If it's not the first time [which means global_base isn't NULL]
    //* (1): First call
    if(!global_base)
    {
        block = request_block(NULL ,size);
        
        if(!block)
        {
            // error from sbrk
            return NULL; // exactly what malloc does
        }

        global_base = block;
    }
    else
    {   //* (2): Not the first call

        // If it isn't the first time, there are already blocks in the list, and suppose we don't find an empty block when searched for, then we'll have to request a block, but to add it to the Linked List i.e apparently the heap, we'll need to get the end of the linked list. So:
        
        //-----------------------------------//
        // meta_block* last = NULL;          //
        // meta_block* temp = global_base;   //
        // while(temp)                       //
        // {                                 //
        //     last = temp;                  //
        //     temp = temp->next;            //
        // }                                 //
        // Now "last" points to the last     //
        //-----------------------------------//

        
        meta_block* last = global_base;

        block = find_free_block(&last, size);
        if(!block)
        {
            //* if we can't find one, we'll request 
            block = request_block(last, size); // give the last also, coz that's where the request block will be fixed
            if(!block)
            {
                return NULL;
            }
        }
        else
        {
            //* found a free block
            block->free = 0;
        }

        
    }
    
    // ! Very Imp. line of code
    return (block + 1);
}



void freee(void* ptr)
{
    if(!ptr)
    {
        return;
    }

    meta_block* block_ptr = (meta_block*)ptr - 1;

    assert(block_ptr->free == 0);

    block_ptr->free = 1;
}

void* call_oc(size_t nelem, size_t elsize)
{
    size_t size = nelem * elsize;

    // TODO: Check for overflow
    if (nelem != 0 && size / nelem != elsize) 
    {
        // overflow happened
        return NULL;
    }

    void* ptr = my_alloc(size);
    
    if(!ptr)
        return NULL;
    
    memset(ptr, 0, size);
    return ptr;
}

void* reall_oc(void* ptr, size_t size)
{
    //*NOTE:  ptr = NULL  ------> realloc behaves as malloc
    if(!ptr)
    {
        return my_alloc(size);
    }

    meta_block* block_ptr = (meta_block*)ptr - 1;
    
    if(block_ptr->size >= size)
    {
        // already have enough space
        return ptr;
    }

    // need to really realloc
    //* Malloc new space and free old space
    //* then just copy old data to the new space
    void* new_ptr;
    new_ptr = my_alloc(size);

    if(!new_ptr)
    {
        return NULL;
    }

    memcpy(new_ptr, ptr, block_ptr->size);
    freee(ptr);
    
    return new_ptr;
    
}


int main()
{

    int s =50;

    void* o = sbrk(0);
    void* n = my_alloc(50);
    void*N = sbrk(0);

    printf("Old => %p\n", o);
    printf("Start of allocation => %p\n", n);
    printf("End of allocation => %p\n", N);

    return 0;
}
