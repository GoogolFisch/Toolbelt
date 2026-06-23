
#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_
#include<stdint.h>
#include<stdlib.h>

struct RingBuffer;


//
struct RingBuffer{
	int32_t size;
	int32_t wrap;
	int32_t next;
	int32_t last;
	int32_t lower;
	int32_t upper;
	void ** list;
};


void ringb_push(struct RingBuffer **ring,void *ptr);
void* ringb_fetch(struct RingBuffer **ring);
void* ringb_pop(struct RingBuffer **ring);
void ringb_compact(struct RingBuffer **ring);
void ringb_clear(struct RingBuffer **ring);
void ringb_free(struct RingBuffer **ring);
int32_t ringb_count(struct RingBuffer **ring);

#ifdef RING_BUFFER_IMPL

void ringb_push(struct RingBuffer **ring,void *ptr){
	if(*ring == NULL){
		*ring = malloc(sizeof(struct RingBuffer));
		(*ring)->size = 64;
		(*ring)->wrap = 64;
		(*ring)->next = 0;
		(*ring)->last = 0;
		(*ring)->upper = 0;
		(*ring)->lower = 0;
		(*ring)->list = malloc(sizeof(void*) * (*ring)->size);
	}
	//
	if((*ring)->lower == 0){
		// inside ring buffer
		int32_t nex = (*ring)->last + 1;
		if(nex >= (*ring)->wrap)
			nex -= (*ring)->wrap;
		if(nex == (*ring)->next){
			(*ring)->lower = (*ring)->wrap;
			(*ring)->upper = (*ring)->wrap + 1;
			(*ring)->size *= 2;
			(*ring)->list = realloc(
					(*ring)->list,
					sizeof(void*) * (*ring)->size);
			(*ring)->list[(*ring)->lower] = ptr;
			return;
		}
		(*ring)->list[(*ring)->last] = ptr;
		(*ring)->last = nex;
		return;
	}
	// appending buffer
	if((*ring)->upper >= (*ring)->size){
		(*ring)->size *= 2;
		(*ring)->list = realloc(
				(*ring)->list,
				sizeof(void*) * (*ring)->size);
	}
	(*ring)->list[(*ring)->upper] = ptr;
	(*ring)->upper++;
	return;
}
void* ringb_fetch(struct RingBuffer **ring){
	if(*ring == NULL)return NULL;
	void *fet = NULL;
	if((*ring)->next == (*ring)->last)
		fet = (*ring)->list[(*ring)->lower];
	else	fet = (*ring)->list[(*ring)->next];
	return fet;
}
void* ringb_pop(struct RingBuffer **ring){
	if(*ring == NULL)return NULL;
	void *fet = NULL;
	if((*ring)->next == (*ring)->last){
		if((*ring)->upper == (*ring)->lower)
			return NULL;
		//
		fet = (*ring)->list[(*ring)->lower];
		(*ring)->lower++;
		if((*ring)->lower == (*ring)->upper){
			(*ring)->lower = 0;
			(*ring)->upper = 0;
			(*ring)->next = 0;
			(*ring)->last = 0;
			(*ring)->wrap = (*ring)->size;
		}
	}else{
		fet = (*ring)->list[(*ring)->next];
		(*ring)->next++;
		if((*ring)->next == (*ring)->wrap){
			(*ring)->next = 0;
		}
		if((*ring)->next == (*ring)->last){
			(*ring)->next = (*ring)->lower;
			(*ring)->last = (*ring)->upper;
			(*ring)->wrap = (*ring)->size;
			(*ring)->lower = 0;
			(*ring)->upper = 0;
		}
	}
	return fet;
}
void ringb_compact(struct RingBuffer **ring){
	if(*ring == NULL)return;
	if((*ring)->upper == (*ring)->lower) return;
	if((*ring)->last   > (*ring)->next ) return;
	////
	(*ring)->wrap = (*ring)->last;
	(*ring)->size = (*ring)->wrap;
	if((*ring)->size < 64)
		(*ring)->size = 64;
	(*ring)->list = realloc((*ring)->list,sizeof(void*) * (*ring)->size);
}
void ringb_clear(struct RingBuffer **ring){
	if(*ring == NULL)return;
	(*ring)->wrap = (*ring)->size;
	(*ring)->next = 0;
	(*ring)->last = 0;
	(*ring)->lower = 0;
	(*ring)->upper = 0;
}
void ringb_free(struct RingBuffer **ring){
	if(*ring == NULL)return;
	free((*ring)->list);
	free(*ring);
	*ring = NULL;
}
int32_t ringb_count(struct RingBuffer **ring){
	if(*ring == NULL)return 0;
	int32_t count = 0;
	count += (*ring)->last - (*ring)->next;
	if(count < 0)count += (*ring)->wrap;
	count += (*ring)->upper - (*ring)->lower;
	return count;
}
#endif

#endif
