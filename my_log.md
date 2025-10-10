

##### STEP_1 : understand void*
	 Just a generic pointer
##### STEP_2 : understand sbrk
	`sbrk` -> lets us manipulate the heap

	## NOTE: `sbrk` returns pointer to the first unallocated byte

	void* p = sbrk(0); // returns a pointer to the curr top of the heap ("Program Break")
	void* s = sbrk(N)
	
		N = 0 -> gives the current end of the heap (first unallocated byte)
		N > 0 -> grows the heap by 'N' bytes but before, returns the old break
		N < 0 -> Shrinks the heap


Enough of sbrk

<hr>

There's so much to implementing a custom allocator. We'll tackle everything on the go.

<hr>

<hr>

### The very naive approach

Idea: - Just sbrk() when needed


Essentially this is the first step, and things will get clear only after this.

```C
#include <stdio.h>
#include <unistd.h>

void* my_alloc(int size)
{
    return sbrk(size);
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
```

<hr>

Now, let's say we gave memory blocks when needed just like that. Later some part if wants to free the memory it has. It definitely gives us the pointer to the memory start, the same pointer that we gave it, but how would we know how much to free. The pointer just brings the address.

Now, one argument could be to just bear a slight trouble and store the asked size also, and pass it when trying to free. Yeah, definitely, but "internal bookkeeping" is as essential. Allocators need to manage memory **independently of the user**. Plus it adds a bit of convenience for the user (Though not a very major concern). Moreover, the wall between proper functioning and malfunction of the program can't be left to a human to watch for, which could be error prone.

So, the solution:
- store the block size in metadata right before the user pointer. So, allocator can every time just decrease the pointer and get the data needed, without any intervention of the user. All it'll need is the pointer, which the user is happy (and bound) to provide.

```C
typedef struct meta_block
{
    size_t size;
    struct meta_block *next;

}meta_block;
```

And since it will be having a definite size, just create it a constant variable to make it easily accessible.
```C
#define META_SIZE sizeof(meta_block)
```

#### Next problem
User gives a pointer to free the memory from the heap. You free it. However, that memory could have been somewhere in between, and not necessarily at the top. So, it's a hole in between the heap. And how we handle it? No idea. We can't shift down the program break, as there's already some memory occupied above it.

So, solution:
- "Reuse" Strategy:
	- Mark freed blocks as free, and change nothing. Just add a flag saying that memory is free or not. If flag says it's free, it can be overwritten. If not, it's occupied.
	```C
	typedef struct meta_block
	{
	    size_t size;
	    struct meta_block *next;
	    int free;
	}meta_block;
	```

And of course all of this needs to be managed. There are many ways:
- Segregated free lists
- Linked List (single/double)
- Bitmap/array-based
- Tree based Structure
- Boundary tags
- Buddy Allocator
However, for the current version we are going to use Linked List.

<hr>

So far, we've just scratched the surface, but a basic flow is clear now.

When `my_alloc` is called, first scan and find if we already have some memory(big enough) that can be reused. If found, flag it "used", again, and use the block. If not, call `sbrk` to extend the heap by a block of specified size.

<hr>

Since it's a Linked List, we'll need a head also.
So,
	`void* global_base = NULL;`

<hr>

Coming back to `my_alloc()` :
```C
void* my_alloc(int size)
{
    return sbrk(size);
}
```

This was the naive way.
Let's implement a better and smart approach, as discussed above.
The code below, pretty much explains itself:

```C
void* my_alloc(int size)
{
    meta_block* block;

    if(size < 0)
    {
        // Makes no sense
        return NULL;
    }  

    // Now, we have two cases:  (1) If it's the first call to my_alloc [which means global_base is still NULL]    (2) If it's not the first time [which means global_base isn't NULL]

    //* (1): First call
    if(!global_base)
    {
        block = request_block(NULL ,size);
        if(!block)
        {// error from sbrk
            return NULL; // exactly what malloc does
        }
        global_base = block;
    }
    else
    {   //* (2): Not the first call
    
        // If it isn't the first time, there are already blocks in the list, and suppose we don't find an empty block when searched for, then we'll have to request a block, but to add it to the Linked List i.e apparently the heap, we'll need to get the end of the linked list. So we'll need a pointer pointing at the end.
    
        meta_block* last = NULL;
        meta_block* temp = global_base;
        while(temp)
        {
            last = temp;
            temp = temp->next;
        }
        // Now "last" points to the last

        block = find_free_block(size);
        if(!block)
        {
            //* if we can't find one, we'll request
            block = request_block(last, size); // give the last also, because that's where the request block will be fixed
            if(!block)
                return NULL;
        }
        else
        {
            //* found a free block
            block->free = 0;
        }
    }
}
```

It is almost all done.
Except for the return statement, i.e. that we'll return as the pointer to the memory block.

But first, just define the functions: `request_block()` and `find_free_block()`:

```C
meta_block* find_free_block(size_t size)
{
    meta_block* curr = global_base;
    meta_block* prev = NULL;

    while(curr && !(curr->free && curr->size >= size)) //* This is where we'll bring in the fitting check (currently it's first fit)
    {
        prev = curr;
        curr = curr->next;
    }
    return curr;
}
```

It is simple and easy to look for a free block.
Let's see how we'll request, as 'a simple `sbrk()`' is not what is needed.

```C
meta_block* request_block(meta_block* last, size_t size)
{
    meta_block* block = NULL;
    block = sbrk(0); // current program break
    //* *//
    void* request = sbrk(size + META_SIZE); // gives the starting part
    //* *//
    
    // just for debugging help
    assert((void*)block == request);
    
    if(request == (void*)-1)  //* (void*)-1 is what sbrk returns on its failure, Nothing Special
    {        return NULL;      }
    
    if(last)
    {
        last->next = block;
    }
    block->size = size;
    block->next = NULL;
    block->free = 0;
    return block;
}
```

Mind the line: 
```C 
void* request = sbrk(size + META_SIZE);
```

We asked for memory for size, to provide to the user, and for meta_block, so to store about the memory block being provided to the user.

<hr>

Now, we can see things clearly. We ask the system to give us a `'S+M'` size and we use the `'M'` for metadata and give the `'S'` to the linked list. 

So, now look back to the `my_alloc()` function.
It returns `block` which is a pointer to the space created with sbrk(size+META_SIZE), where block, being a pointer of META_SIZE, when done +1, moves by the size of META_SIZE.

![alt text](<Pasted image 20251003225853.png>)

Thus, `my_alloc()` returns:
```C
return (block + 1);
```

<hr>

Now, a simple standard improvement in the code, to improve on performance.

My function:

`meta_block* find_free_block(size_t size)`

has a `prev` variable but doesn’t return it. This is okay because we recompute `last` manually outside, but usually allocators pass a **double pointer to last** (`meta_block** last`) so they don’t have to re-traverse the list. (We are currently trading a bit of performance for simplicity.)

Let's fix it:

In `find_free_block()`, we just took size.
A standard way is to pass "last" pointer with it(initially initialized to global_base). This is more efficient as once we are traversing the list inside find_free_block any way, and then again if we traverse outside of this function also, it's unnecessary utilization of resources.
So, in `malloc()` where traverse once and take it to the last manually, just point `last` to global_base, and when find_free_block will traverse for itself, it will take `last` to its destined position with itself, without to deal with an extra iteration.
so:
```C
void* my_alloc()
{
	...
	..
	.
		meta_block* last = global_base;
		
		block = find_free_block(&last, size); // we pass it by ref coz otherwise it wouldn't ever change
	.
	..
	...
}
```

&

```C
meta_block* find_free_block(meta_block** last, size_t size)
{
	meta_block* curr = global_base;
    // meta_block* prev = NULL;
    // replace prev with 'last'
    ...
    ..
    .
    //prev = curr
    *last = curr;
    .
    ..
    ...
}
```


<hr>

<hr>

Finally, we are done with allocating. 
Now let's implement freeing.

{ we use `freee()` }
```C
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
```

As discussed when freeing, the user will pass the ptr(i.e. the pointer that user has), so we take a META_SIZE step back to get the ptr we want(i.e. the pointer to the meta_data).

That's it.
And just flag the block as free.

<hr>

<hr>

Let's now finish it all up with the final parts of custom memory allocator: `realloc()` and `calloc()`

##### What is calloc?
Calloc is just malloc that initializes the memory to 0.

{ we use `call_oc` }
```C
void* call_oc(size_t nelem, size_t elsize)
{
    size_t size = nelem * elsize;
    // TODO: Check for overflow
    void* ptr = my_alloc(size);
    memset(ptr, 0, size);
    return ptr;
}
```

What does `memset` do?
```C
void *memset(void *s, int c, size_t n);
```
`memset` fills the first 'n' bytes of memory pointed by 's', with the value of 'c' (converted to unsigned char)

##### What is realloc?
Realloc stands for reallocate.
It's job is to change the size of a memory block you previously got from malloc, calloc or realloc, while trying to preserve the data already stored in that block.

```C
void reall_oc(void* ptr, size_t size)

// ptr is the pointer to previously allocated memory
//@note:  ptr = NULL ---> realloc behaves as malloc
// size is the 'new requested size'
```
This is realloc.

```C
void* reall_oc(void* ptr, size_t size)
{
    //*NOTE:  ptr = NULL  ------> realloc behaves as malloc
    if(!ptr)
    {      return my_alloc(size);       }

    meta_block* block_ptr = (meta_block*)ptr - 1;
    
    if(block_ptr->size >= size)
    {   // already have enough space
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
    free(ptr);

    return new_ptr;
}
```

<hr>

<hr>

## ALIGNMENT


Alignment is what hardware expects.
And it cannot be ignored. One must abide by it.

Many architectures expect certain data types to start at address divisible by some multiple of 2(4, 16, etc.). For example, it’s common to require 4-byte types to be aligned to 4-byte boundaries and 8-byte types on 8-byte boundaries. If multi-byte primitives are stored on an unreasonable boundary, the performance can be significantly impacted. On some architectures the penalty is even greater - the program will crash.

As malloc does not know how the user will use the allocated memory, the pointer returned to the program needs to be aligned for the worst case, which is architecture-dependent.

So, when we do: `(block+1)`, we intend to move the `block` pointer by `sizeof(meta_block)`, which works only if `sizeof(meta_block)` is divisible by the strictest alignment requirement, so else the pointer that we get for the memory won't be aligned, and so the consequences. 

What we currently have:
	meta_block  ->   | size_t size (8 bytes) | int free (4 bytes) | meta_block* next (8 bytes) |

	8+4+8 = 20 bytes {Not Aligned}

	Compiler might add a 4 bytes padding, but what if I want to add a `char flag`, later on?

**NOTE** :- Anytime we change `meta_data`, we might misalign user pointer.

###### Now,
### How do I retrofit alignment into my code?

Whenever we allocate:  `block = request_block(last, size)`
If someone asks for 50 bytes, and we just pass 50 to sbrk as it is, the returned memory won't be aligned.
So, always round up requested size to neared multiple

<hr>
NOTE:

Alignment is essentially about where a memory block "handed to the user" starts and how it's "lined up" in the address space relative to CPU requirements.

- The starting address of a memory block must satisfy a multiple of the alignment requirement.
	We have `block` (ptr) in the actually heap, pointing to the start of the memory, and then `block+1` (ptr+1) pointing to the payload space which the user will receive to work with.
	This must be aligned, i.e. `block+1` must be aligned, not necessarily `block`.

	Though it is the way it is, but safest bet is to align wherever `size` is being dealt with "the raw memory", not at user facing API.
	{We use aligned_size when requesting the sbrk, not when we called the `request_space(NULL, size)`. Why? Coz it is apparently a user facing API, it is not itself dealing with raw memory.} [Being clear with the distinction of Interface and Implementation]

- While the CPU doesn't usually care about the exact end of th block, alignment often forces the block size to be rounded up so the next block also starts on an aligned boundary.
<hr>

***Rounding up for Alignment***:

First define what the alignment requirement is:
```C
define ALIGNMENT 8 // 16 for some systems
```

Now, rounding up of a size:

```
((size) + (ALIGNMENT - 1)) &~ (ALIGNMENT - 1)
```

So, define a MACRO for it
```C
define ALIGN ((size) + (ALIGNMENT - 1)) &~ (ALIGNMENT - 1)
```
It rounds up to the next multiple of `ALIGNMENT`

Start aligning everywhere you deal with `size`.

1) Inside `my_Alloc` and `call_oc`, use `aligned_size` instead of `size`:
```C
size_t aligned_size = ALIGN(size);
```
	& use `aligned_size` wherever you do these (& only for these):
	- call sbrk
	- decide a free block is big enough  *****

2) When we do: `return (block+1)`:
	block: is a `meta_block*`
	(block + 1) : points immediately after the metadata so we want that to be aligned

	There are 2 ways to handle it
	1) Add padding manually: 
		After `block+1` compute offset to next multiple of ALIGNMENT and return that.
	
	2) Force `meta_block` to be multiple of ALIGNMENT
```C
	define META_SIZE ALIGN(sizeof(meta_block))	
```
		Then everywhere we add `META_SIZE` the resulting user pointer is aligned

3) Update `sbrk` requests:
```C
void* request = sbrk(aligned_size + META_SIZE);
```

4) Update free block:
```C
if(curr->free && curr->size >= aligned_size)
```

<hr>
We saw a clear distinction between the usage of `aligned_size` with interface and implementation. 
However, when we reach to the function `request_block()`, we encounter something triggering.
After requesting the new block (with aligned size), we do:
```C
    block->size = size;
    block->next = NULL; // marks it as 'the last block'
    block->free = 0;
```
For example: user asked `size = 13B` allocator aligned it to `aligned_size = 16B` and thus what the request gets in return is a `16B` block. But according to the above excerpt, we still mark the block's meta data with it's size being `13B` and not `16B`. That is not good. Why? Suppose this block is freed after some time, and a new request by the user is of `size = 15B`, again the allocator rounds it up to `aligned_size = 16B`. Now when we `find_free_block`, though we have a 16B block, but since it's marked as 13B we won't ever get that and a new block will be created, despite we had a big enough block.

So, there needs to be some work around:
Two options:
1) Store aligned size in metadata too
```C
typedef struct meta_block {
    size_t size;         // user-requested size
    size_t aligned_size; // actual block size allocated
    struct meta_block *next;
    int free;
} meta_block;
```

2) Recompute aligned size on-the-fly
```C
if (block->free && ALIGN(block->size) >= ALIGN(request_size))
```
	Involves recomputation of alignment every time; performace-overhead.

We opt option-1, as it's clear, safer, better and more realistic.
So, now again going back to the `find_free_block()`
where we compared `curr->size` with `aligned_size`, now we'll compare `curr->alignd_size` with `aligned_size`
```C
while(curr && !(curr->free && curr->aligned_size >= aligned_size))
```

Coming to *`reall_oc()`*:
There's a subtlety in here, considering reallocation and alignment. 
Example:
User had a previous request of 13B, but internally got 16B, now he want the same to be expanded to 15B, again most probably he'll need 16B, but if the allocator doesn't know that the same block is usable to 16B, it would request a new block.
So, it's better to check for same block reuse in `reall_oc()` considering the `aligned_size_request`:

```C
void* reall_oc(void* ptr, size_t size)
{
    size_t aligned_size_request = ALIGN(size);

    //*NOTE:  ptr = NULL  ------> realloc behaves as malloc
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

    if(block_ptr->aligned_size >= aligned_size_request)
    {
        block_ptr->size = size; // update user-visible size
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
```

<hr>

<hr>

## Splitting and Coalescing
<hr>

## Splitting

![alt text](<shapes at 25-10-10 17.11.16.png>)

![alt text](<shapes at 25-10-10 17.19.31.png>)

User calls `freee(ptr3)` {ptr3 is the ptr to the 64B block} :
```C
meta_block* block = (meta_block*)ptr3 - 1;
block->free = 1;
```

now, user calls `my_alloc(7)`
& by chance, we get the 64B payload block at first.
Now instead of giving the 64B completely just for 7 bytes, split it.

So, to divide:

![alt text](<shapes at 25-10-10 17.27.08.png>)

1) Change the meta_data of block
2) Create a new meta_data for soon to be created block (of 56B).

<hr>

```C
new_block = (meta_block*)((char*)(ptr) + aligned_requested_size + META_SIZE)
```

<hr>
#### NOTE:
###### Pointer Arithmetic

When we do:
`ptr+1` we shift the address by `sizeof(*ptr)`
`ptr + n` means we shift by  `n * sizeof(*ptr)`

if `ptr` points to an integer the address shifts by `n * sizeof(int)`
Similarly,
 here `ptr` is of  of type `meta_data`
 so `ptr + req.size`  will move by `meta_data` times `req.size`
 but we don't want to do so,
 we want to move exactly `req.size` bytes,
 i.e. we go byte-by-byte.
 So, we use 
  ```C
      ( char * )
  ```
 coz char is guaranteed to be of 1B

<hr>

```C
    new_block->aligned_size = (ptr)->aligned_size - aligned_requested_size - META_SIZE;
    new_block->free = 1;

	// adjust the next pointers

    new_block->next = (ptr)->next;

    (ptr)->next = new_block;

    (ptr)->aligned_size = aligned_requested_size;

    (ptr)->size = requested_size;

    (ptr)->free = 0;
```
<hr>

## Coalescing

![alt text](<shapes at 25-10-10 17.54.26 1.png>)

If `ptr2` was used, but just now user called `freee(ptr2)` .
Till now, we just used to mark `(ptr2 - 1) -> free = 1` , where {block = ptr2-1}

Now, when freeing, just quickly check if the forward block is free or not. If it is, just coalesce:
```C
next_block = block->next;

block->next = next_block->next;
block->aligned_size = block->aligned_size + next_block->aligned_size + META_SIZE
```

{ This was Forward Coalescing }

<hr>

<hr>

## Implementing Segregated Free Lists

.....to be done

<hr>

<hr>

Next steps:
1) splitting/coalescing [DONE]
2) add segregated-free-list implementation
3) Think of concurrency/thread-awareness


