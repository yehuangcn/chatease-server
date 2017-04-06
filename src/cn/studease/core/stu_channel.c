/*
 * stu_channel.c
 *
 *  Created on: 2016-12-5
 *      Author: Tony Lau
 */

#include "stu_config.h"
#include "stu_core.h"

extern stu_cycle_t *stu_cycle;

static void stu_channel_remove_user_locked(stu_channel_t *ch, stu_connection_t *c);


stu_int_t
stu_channel_init(stu_channel_t *ch, stu_str_t *id) {
	ch->id.data = stu_slab_calloc(stu_cycle->slab_pool, id->len + 1);
	memcpy(ch->id.data, id->data, id->len);
	ch->id.len = id->len;

	return STU_OK;
}

stu_int_t
stu_channel_insert(stu_channel_t *ch, stu_connection_t *c) {
	stu_int_t  rc;

	stu_spin_lock(&ch->userlist.lock);
	rc = stu_channel_insert_locked(ch, c);
	stu_spin_unlock(&ch->userlist.lock);

	return rc;
}

stu_int_t
stu_channel_insert_locked(stu_channel_t *ch, stu_connection_t *c) {
	stu_str_t *key;

	key = &c->user.strid;

	if (stu_hash_insert_locked(&ch->userlist, key, c, STU_HASH_LOWCASE) == STU_ERROR) {
		stu_log_error(0, "Failed to insert user \"%s\" into channel \"%s\", total=%lu.", key->data, ch->id.data, ch->userlist.length);
		return STU_ERROR;
	}

	stu_log_debug(4, "Inserted user \"%s\" into channel \"%s\", total=%lu.", key->data, ch->id.data, ch->userlist.length);

	return STU_OK;
}

void
stu_channel_remove(stu_channel_t *ch, stu_connection_t *c) {
	stu_spin_lock(&ch->userlist.lock);
	stu_channel_remove_locked(ch, c);
	stu_spin_unlock(&ch->userlist.lock);
}

void
stu_channel_remove_locked(stu_channel_t *ch, stu_connection_t *c) {
	stu_str_t     *key;
	stu_uint_t     kh;

	stu_channel_remove_user_locked(ch, c);

	if (ch->userlist.length == 0) {
		if (ch->userlist.free) {
			ch->userlist.free(ch->userlist.pool, ch->userlist.buckets);
		}
		ch->userlist.buckets = NULL;

		key = &ch->id;
		kh = stu_hash_key_lc(key->data, key->len);

		stu_hash_remove(&stu_cycle->channels, kh, key->data, key->len);

		stu_slab_free(stu_cycle->slab_pool, key->data);
		stu_slab_free(stu_cycle->slab_pool, ch);

		stu_log_debug(4, "removed channel \"%s\", total=%lu.", key->data, ch->userlist.length);
	}
}

static void
stu_channel_remove_user_locked(stu_channel_t *ch, stu_connection_t *c) {
	stu_str_t        *key;
	stu_uint_t        kh, i;
	stu_hash_t       *hash;
	stu_hash_elt_t   *elts, *e;
	stu_queue_t      *q;
	stu_connection_t *vc;

	hash = &ch->userlist;
	key = &c->user.strid;
	kh = stu_hash_key_lc(key->data, key->len);
	i = kh % hash->size;

	elts = hash->buckets[i];
	if (elts == NULL) {
		stu_log_error(0, "Failed to remove from hash: key=%lu, i=%lu, name=%s.", kh, i, key->data);
		return;
	}

	for (q = stu_queue_head(&elts->queue); q != stu_queue_sentinel(&elts->queue); q = stu_queue_next(q)) {
		e = stu_queue_data(q, stu_hash_elt_t, queue);
		vc = (stu_connection_t *) e->value;
		if (e->key_hash != kh || e->key.len != key->len || vc->fd != c->fd) {
			continue;
		}
		if (stu_strncmp(e->key.data, key->data, key->len) == 0) {
			stu_queue_remove(&e->queue);
			stu_queue_remove(&e->q);

			if (hash->free) {
				hash->free(hash->pool, e->key.data);
				hash->free(hash->pool, e);
			}

			hash->length--;

			stu_log_debug(1, "Removed %p from hash: key=%lu, i=%lu, name=%s.", e->value, kh, i, key->data);

			break;
		}
	}

	if (elts->queue.next == stu_queue_sentinel(&elts->queue)) {
		if (hash->free) {
			hash->free(hash->pool, elts);
		}

		hash->buckets[i] = NULL;

		//stu_log_debug(1, "Removed sentinel from hash: key=%lu, i=%lu, name=%s.", key, i, name);
	}

	stu_log_debug(4, "removed user \"%s\" from channel \"%s\", total=%lu.", key->data, ch->id.data, ch->userlist.length);
}


void
stu_channel_broadcast(stu_str_t *id, void *ch) {
	stu_log_debug(4, "broadcast in channel \"%s\".", id->data);
}

