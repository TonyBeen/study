/*************************************************************************
    > File Name: hash.c
    > Author: hsz
    > Brief:
    > Created Time: Mon 25 Jul 2022 02:36:12 PM CST
 ************************************************************************/

#include "hash.h"
#include <assert.h>

#define LIBUTP_HASH_UNUSED ((utp_link_t)-1)
#define BASE_SIZE(bc) (sizeof(utp_hash_t) + sizeof(utp_link_t) * ((bc) + 1))
#define get_bep(h) ((uint8_t*)(h)) + BASE_SIZE((h)->N)
#define ptr_to_link(p) (utp_link_t *) (((uint8_t *) (p)) + hash->E - sizeof(utp_link_t))
#define ALLOCATION_SIZE(bc, ts, sc) (BASE_SIZE((bc)) + (ts) * (sc))


uint32_t Read32(const void *p)
{
	uint32_t tmp;
	memcpy(&tmp, p, sizeof tmp);
	return tmp;
}

utp_hash_t *utp_hash_create(int N, int key_size, int total_size, int initial, utp_hash_compute_t hashfun, utp_hash_equal_t compfun)
{
	// Must have odd number of hash buckets (prime number is best)
	assert(N % 2);
	// Ensure structures will be at aligned memory addresses
	// TODO:  make this 64-bit clean
	assert(0 == (total_size % 4));

	// ((sizeof(utp_hash_t) + sizeof(utp_link_t) * (N + 1)) + (total_size) * (initial))
	int size = ALLOCATION_SIZE(N, total_size, initial);
	utp_hash_t *hash = (utp_hash_t *) malloc( size );
	memset( hash, 0, size );

	for (int i = 0; i < N + 1; ++i)
		hash->inits[i] = LIBUTP_HASH_UNUSED;
	hash->N = N;
	hash->K = key_size;
	hash->E = total_size;
	hash->hash_compute = hashfun;
	hash->hash_equal = compfun;
	hash->allocated = initial;
	hash->count = 0;
	hash->used = 0;
	hash->free = LIBUTP_HASH_UNUSED;
	return hash;
}


uint32_t utp_hash_mem(const void *keyp, size_t keysize)
{
	uint32_t hash = 0;
	uint32_t n = keysize;
	while (n >= 4) {
		hash ^= Read32(keyp);
		keyp = (uint8_t*)keyp + sizeof(uint32_t);
		hash = (hash << 13) | (hash >> 19);
		n -= 4;
	}
	while (n != 0) {
		hash ^= *(uint8_t*)keyp;
		keyp = (uint8_t*)keyp + sizeof(uint8_t);
		hash = (hash << 8) | (hash >> 24);
		n--;
	}
	printf("hash id = %d\n", hash);
	return hash;
}

uint32_t utp_hash_mkidx(utp_hash_t *hash, const void *keyp)
{
	// Generate a key from the hash
	return hash->hash_compute(keyp, hash->K) % hash->N;
}

static inline bool compare(uint8_t *a, uint8_t *b,int n)
{
	assert(n >= 4);
	if (Read32(a) != Read32(b)) return false;
	return memcmp(a+4, b+4, n-4) == 0;
}

#define COMPARE(h,k1,k2,ks) (((h)->hash_equal) ? (h)->hash_equal((void*)k1,(void*)k2,ks) : compare(k1,k2,ks))

// Look-up a key in the hash table.
// Returns NULL if not found
void *utp_hash_lookup(utp_hash_t *hash, const void *key)
{
	utp_link_t idx = utp_hash_mkidx(hash, key);

	// base pointer
	uint8_t *bep = get_bep(hash);

	utp_link_t cur = hash->inits[idx];
	while (cur != LIBUTP_HASH_UNUSED) {
		uint8_t *key2 = bep + (cur * hash->E);
		if (COMPARE(hash, (uint8_t*)key, key2, hash->K))
			return key2;
		cur = *ptr_to_link(key2);
	}

	return NULL;
}

// Add a new element to the hash table.
// Returns a pointer to the new element.
// This assumes the element is not already present!
void *utp_hash_add(utp_hash_t **hashp, const void *key)
{
	//Allocate a new entry
	uint8_t *elemp;
	utp_link_t elem;
	utp_hash_t *hash = *hashp;
	utp_link_t idx = utp_hash_mkidx(hash, key);

	if ((elem=hash->free) == LIBUTP_HASH_UNUSED) {
		utp_link_t all = hash->allocated;
		if (hash->used == all) {
			utp_hash_t *nhash;
			if (all <= (LIBUTP_HASH_UNUSED/2)) {
				all *= 2;
			} else if (all != LIBUTP_HASH_UNUSED) {
				all  = LIBUTP_HASH_UNUSED;
			} else {
				// too many items! can't grow!
				assert(0);
				return NULL;
			}
			// otherwise need to allocate.
			nhash = (utp_hash_t*)realloc(hash, ALLOCATION_SIZE(hash->N, hash->E, all));
			if (!nhash) {
				// out of memory (or too big to allocate)
				assert(nhash);
				return NULL;
			}
			hash = *hashp = nhash;
			hash->allocated = all;
		}

		elem = hash->used++;
		elemp = get_bep(hash) + elem * hash->E;
	} else {
		elemp = get_bep(hash) + elem * hash->E;
		hash->free = *ptr_to_link(elemp);
	}

	*ptr_to_link(elemp) = hash->inits[idx];
	hash->inits[idx] = elem;
	hash->count++;

	// copy key into it
	memcpy(elemp, key, hash->K);
	return elemp;
}

// Delete an element from the utp_hash_t
// Returns a pointer to the already deleted element.
void *utp_hash_del(utp_hash_t *hash, const void *key)
{
	utp_link_t idx = utp_hash_mkidx(hash, key);

	// base pointer
	uint8_t *bep = get_bep(hash);

	utp_link_t *curp = &hash->inits[idx];
	utp_link_t cur;
	while ((cur=*curp) != LIBUTP_HASH_UNUSED) {
		uint8_t *key2 = bep + (cur * hash->E);
		if (COMPARE(hash,(uint8_t*)key,(uint8_t*)key2, hash->K )) {
			// found an item that matched. unlink it
			*curp = *ptr_to_link(key2);
			// Insert into freelist
			*ptr_to_link(key2) = hash->free;
			hash->free = cur;
			hash->count--;
			return key2;
		}
		curp = ptr_to_link(key2);
	}

	return NULL;
}

void *utp_hash_iterate(utp_hash_t *hash, utp_hash_iterator_t *iter)
{
	utp_link_t elem;

	if ((elem=iter->elem) == LIBUTP_HASH_UNUSED) {
		// Find a bucket with an element
		utp_link_t buck = iter->bucket + 1;
		for(;;) {
			if (buck >= hash->N)
				return NULL;
			if ((elem = hash->inits[buck]) != LIBUTP_HASH_UNUSED)
				break;
			buck++;
		}
		iter->bucket = buck;
	}

	uint8_t *elemp = get_bep(hash) + (elem * hash->E);
	iter->elem = *ptr_to_link(elemp);
	return elemp;
}

void utp_hash_free_mem(utp_hash_t* hash)
{
	free(hash);
}