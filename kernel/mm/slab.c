#include <crescent/common.h>
#include <crescent/compiler.h>
#include <crescent/mm/slab.h>
#include <crescent/mm/vmm.h>
#include <crescent/core/printk.h>
#include <crescent/core/trace.h>
#include <crescent/lib/string.h>

/* The maximum size a slab can be before the object count goes to SLAB_AFTER_CUTOFF_COUNT */
#define SLAB_SIZE_CUTOFF 512
#define SLAB_AFTER_CUTOFF_OBJ_COUNT 16

static int slab_init(struct slab_cache* cache, struct slab* slab) {
	size_t map_size = (cache->obj_count + 7) / 8;
	size_t slab_size = cache->obj_size * cache->obj_count;

	/* Allocate free list bitmap */
	slab->free = vmap(NULL, map_size, MMU_READ | MMU_WRITE, VMM_ALLOC, NULL);
	if (!slab->free)
		return -ENOMEM;

	/* Allocate a virtual address for the slab base */
	slab->base = vmap(NULL, slab_size, MMU_READ | MMU_WRITE, VMM_ALLOC, &cache->mm_flags);
	if (!slab->base) {
		int err = vunmap(slab->free, map_size, 0);
		if (unlikely(err)) {
			printk(PRINTK_CRIT "mm: Failed to unmap free list in %s (%i)\n", __func__, err);
			dump_stack();
		}
		return -ENOMEM;
	}

	slab->in_use = 0;
	return 0;
}

static int slab_cache_grow(struct slab_cache* cache) {
	struct slab* slab = vmap(NULL, sizeof(*slab), MMU_READ | MMU_WRITE, VMM_ALLOC, NULL);
	if (!slab)
		return -ENOMEM;

	int err = slab_init(cache, slab);
	if (err) {
		int verr = vunmap(slab, sizeof(*slab), 0);
		if (unlikely(verr)) {
			printk(PRINTK_CRIT "mm: Failed to unmap new slab: %i\n", verr);
			dump_stack();
		}
		return err;
	}

	/* Add the slab into the list of empty slabs */
	slab->next = cache->empty;
	if (slab->next)
		slab->next->prev = slab;

	slab->prev = NULL;
	cache->empty = slab;
	return 0;
}

static void* slab_take(struct slab_cache* cache, struct slab* slab) {
	size_t obj_num = SIZE_MAX;

	/* Simply find a free block within the bitmap */
	for (size_t i = 0; i < cache->obj_count; i++) {
		size_t byte_index = i / 8;
		unsigned int bit_index = i % 8;

		if (!(slab->free[byte_index] & (1 << bit_index))) {
			slab->free[byte_index] |= (1 << bit_index);
			obj_num = i;
			break;
		}
	}

	if (obj_num == SIZE_MAX)
		return NULL;

	/* Now just get the object's pointer and call its constructor */
	void* obj = (u8*)slab->base + cache->obj_size * obj_num;
	if (cache->ctor)
		cache->ctor(obj);

	slab->in_use++;
	return obj;
}

static struct slab* slab_release(struct slab_cache* cache, void* obj) {
	bool partial = !cache->full;

	/* First find the slab that the block is in */
	struct slab* slab = partial ? cache->partial : cache->full;
	while (slab) {
		void* top = (void*)((uintptr_t)slab->base + cache->obj_count * cache->obj_size);
		if (obj >= slab->base && obj < top)
			break;
		if (!slab->next && !partial) {
			slab = cache->partial;
			partial = true;
		} else {
			slab = slab->next;
		}
	}

	/* This means a valid address wasn't passed to the function */
	if (!slab)
		return NULL;

	/* Get the object number and then mark it as free */
	size_t obj_num = ((uintptr_t)obj - (uintptr_t)slab->base) / cache->obj_size;
	size_t byte_index = obj_num / 8;
	size_t bit_index = obj_num % 8;
	slab->free[byte_index] &= ~(1 << bit_index);

	/* Now call the destructor and return the slab the free happened on */
	if (cache->dtor)
		cache->dtor(obj);
	slab->in_use--;
	return slab;
}

void* slab_cache_alloc(struct slab_cache* cache) {
	unsigned long flags;
	spinlock_lock_irq_save(&cache->lock, &flags);

	/* See if we have any partial or empty caches */
	struct slab* slab = NULL;
	if (cache->partial)
		slab = cache->partial;
	else if (cache->empty)
		slab = cache->empty;

	void* ret = NULL;
	/* If no slabs are available, try growing the cache */
	if (!slab) {
		if (slab_cache_grow(cache) == 0)
			slab = cache->empty;
		else
			goto out;
	}

	ret = slab_take(cache, slab);
	if (unlikely(!ret)) {
		printk(PRINTK_ERR "mm: slab_take failed even though the slab should have been big enough\n");
		goto out;
	}

	/* 
	 * If the slab was empty (slab == cache->empty), move it to the partial list.
	 * If the slab is now full (slab->in_use == cache->obj_count), move it to the full list.
	 */
	if (slab == cache->empty) {
		cache->empty = slab->next;
		if (slab->next)
			slab->next->prev = NULL;
		if (cache->partial)
			cache->partial->prev = slab;

		slab->next = cache->partial;
		slab->prev = NULL;
		cache->partial = slab;
	} else if (slab->in_use == cache->obj_count) {
		cache->partial = slab->next;
		if (slab->next)
			slab->next->prev = NULL;
		if (cache->full)
			cache->full->prev = slab;
		slab->next = cache->full;
		slab->prev = NULL;
		cache->full = slab;
	}

out:
	spinlock_unlock_irq_restore(&cache->lock, &flags);
	return ret;
}

void slab_cache_free(struct slab_cache* cache, void* obj) {
	unsigned long flags;
	spinlock_lock_irq_save(&cache->lock, &flags);

	struct slab* slab = slab_release(cache, obj);
	if (!slab) {
		printk(PRINTK_ERR "mm: slab_release returned NULL, invalid object? obj: %p\n", obj);
		dump_stack();
		goto out;
	}

	/* If this happens, the slab is now empty, so move it to the empty list */
	if (slab->in_use == 0) {
		if (slab->prev == NULL)
			cache->partial = slab->next;
		else
			slab->prev->next = slab->next;

		if (slab->next)
			slab->next->prev = slab->prev;

		slab->next = cache->empty;
		slab->prev = NULL;
		if (slab->next)
			slab->next->prev = slab;
		cache->empty = slab;
	}

	/* If the slab was previously full, then move it back to the partial list */
	if (slab->in_use == cache->obj_count - 1) {
		if (slab->prev == NULL)
			cache->full = slab->next;
		else
			slab->prev->next = slab->next;

		if (slab->next)
			slab->next->prev = slab->prev;

		slab->next = cache->partial;
		slab->prev = NULL;
		if (slab->next)
			slab->next->prev = slab;
		cache->partial = slab;
	}

out:
	spinlock_unlock_irq_restore(&cache->lock, &flags);
}

struct slab_cache* slab_cache_create(size_t obj_size, size_t align, 
		mm_t mm_flags, void (*ctor)(void*), void (*dtor)(void*)) {
	if (align == 0)
		align = 8;
	else if (align & (align - 1))
		return NULL;

	struct slab_cache* cache = vmap(NULL, sizeof(*cache), MMU_READ | MMU_WRITE, VMM_ALLOC, NULL);
	if (!cache)
		return NULL;

	cache->ctor = ctor;
	cache->dtor = dtor;
	cache->full = NULL;
	cache->partial = NULL;
	cache->empty = NULL;
	cache->obj_size = ROUND_UP(obj_size, align);
	cache->obj_count = cache->obj_size < SLAB_SIZE_CUTOFF ? (PAGE_SIZE * 2) / cache->obj_size : SLAB_AFTER_CUTOFF_OBJ_COUNT;
	cache->align = align;
	cache->mm_flags = mm_flags;
	atomic_store(&cache->lock, SPINLOCK_INITIALIZER, ATOMIC_RELAXED);

	return cache;
}

int slab_cache_destroy(struct slab_cache* cache) {
	if (cache->partial || cache->full)
		return -EEXIST;

	int err;

	struct slab* slab = cache->empty;
	while (slab) {
		struct slab* next = slab->next;
		if (next)
			next->prev = NULL;

		int base_err = vunmap(slab->base, cache->obj_size * cache->obj_count, 0);
		int free_list_err = vunmap(slab->free, (cache->obj_count + 7) / 8, 0);
		int slab_err = vunmap(slab, sizeof(*slab), 0);
		if (unlikely(base_err))
			err = base_err;
		if (unlikely(free_list_err))
			err = free_list_err;
		if (unlikely(slab_err))
			err = slab_err;

		if (!(base_err == free_list_err && free_list_err == slab_err)) {
			printk(PRINTK_WARN "mm: Different error codes in %s, base_err: %i, free_list_err: %i, slab_err: %i, choosing %i\n",
					__func__, base_err, free_list_err, slab_err, err);
		}

		slab = next;
	}

	err = vunmap(cache, sizeof(*cache), 0);
	return err;
}
