
#ifndef MATRIX_H_
#define MATRIX_H_

#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>
#define REED_SOL_ALL
#include"reedSol.h"


#define MATRIX_SIZE 64
#define MATRIX_MUTATIONS 10
struct Mat{
	int32_t dim;
	uint32_t dummy;
	uint8_t data[];
};

#define MATRIX_SEED_SIZE 25
struct MatSeed{
	uint64_t data[MATRIX_SEED_SIZE];
};
struct MatSeed *matRand_new(void);
struct MatSeed *matRand_newRand(void);
uint64_t matRand_cycle(struct MatSeed *ms,uint64_t dat);
// or just use free
void matRand_free(struct MatSeed *ms);


struct Mat *mat_new(int32_t dim,struct MatSeed *sed);
struct Mat *mat_mul(struct Mat *left, struct Mat *right);
void mat_mutate(struct Mat *mat,int32_t cnt,struct MatSeed *seed);
// or just use free
void mat_free(struct Mat *mat);


void mat_generateCorrection(struct Mat *mat,int32_t mutCnt,
		char **outMsg,int32_t *byteOut);
void mat_applyCorrection   (struct Mat *mat,
		char *outMsg,int32_t byteOut);

#ifdef MATRIX_IMPL
#include <time.h>
#include<byteswap.h>

struct MatSeed *matRand_new(void){
	struct MatSeed *sed = malloc(sizeof(struct MatSeed));
	char *z = (char*)sed;
	for(int32_t idx = 0;idx < sizeof(struct MatSeed);idx++)
		z[idx] = 0;
	return sed;
}
struct MatSeed *matRand_newRand(void){
	struct MatSeed *sed = malloc(sizeof(struct MatSeed));
	
	struct timespec t0;
	timespec_get(&t0, TIME_UTC);
	matRand_cycle(sed,(uint64_t)t0.tv_nsec);
	matRand_cycle(sed,(uint64_t)t0.tv_sec);
	timespec_get(&t0, TIME_MONOTONIC);
	matRand_cycle(sed,(uint64_t)t0.tv_nsec);
	matRand_cycle(sed,(uint64_t)t0.tv_sec);
	timespec_get(&t0, TIME_ACTIVE);
	matRand_cycle(sed,(uint64_t)t0.tv_nsec);
	matRand_cycle(sed,(uint64_t)t0.tv_sec);
	timespec_get(&t0, TIME_THREAD_ACTIVE);
	matRand_cycle(sed,(uint64_t)t0.tv_nsec);
	matRand_cycle(sed,(uint64_t)t0.tv_sec);

	return sed;
}
uint64_t matRand_cycle(struct MatSeed *ms,uint64_t dat){
	uint64_t carry = 1;
	for(int32_t iter = 0;iter < 4;iter++){
		carry ^= dat;
		for(int32_t idx = 0;idx < MATRIX_SEED_SIZE;idx++){
			carry += ms->data[idx];
			carry ^= (carry >> 11) + (carry << 6);
			ms->data[idx] ^= carry;
		}
	}
	return carry;
}
void matRand_free (struct MatSeed *ms){free(ms);}

struct Mat *mat_new(int32_t dim,struct MatSeed *sed){
	size_t sz = sizeof(uint8_t) * dim * dim;
	sz = (0xf | (sz - 1)) + 1;
	sz += sizeof(struct Mat);
	sz = (0xff | (sz - 1)) + 1;
	struct Mat *mat = malloc(sizeof(char) * sz);
	//
	char *z = (char*)mat;
	for(int32_t idx = 0;idx < sizeof(struct MatSeed);idx++)
		z[idx] = 0;
	//
	mat->dim = dim;
	uint64_t *fill = (uint64_t*)(&(mat->data[0]));
	int32_t maxFlood = (sz / 8) - 1;
	int32_t tst = 1;
	tst = *((int8_t*)(&tst));
	uint64_t val;
	for(int32_t idx = 0;idx < maxFlood;idx++){
		val = matRand_cycle(sed,0);
		if(tst)val = bswap_64(val);
		fill[idx] = val;
	}
	return mat;
}
struct Mat *mat_mul(struct Mat *left, struct Mat *right){
	if(left->dim != right->dim)
		return NULL;
	reedsol_Init();
	size_t sz = sizeof(uint8_t) * left->dim * left->dim;
	sz = (0xf | (sz - 1)) + 1;
	sz += sizeof(struct Mat);
	struct Mat *mat = malloc(sz);
	mat->dim = left->dim;
	int32_t pos;
	uint8_t val1,val2;
	for(int32_t i = 0;i < left->dim;i++){
		for(int32_t j = 0;j < left->dim;j++){
			pos = i * left->dim + j;
			mat->data[pos] = 0;
			for(int32_t k = 0;k < left->dim;k++){
				val1 = left ->data[k + i * left->dim];
				val2 = right->data[j + k * left->dim];
				val1 = reedsol_Mul(val1,val2);
				val2 = mat->data[pos];
				mat->data[pos] = reedsol_Add(val1,val2);
			}
		}
	}
	return mat;
}
void mat_mutate(struct Mat *mat,int32_t cnt,struct MatSeed *seed){
	uint64_t dat;
	uint32_t pos;
	for(;cnt > 0;cnt--){
		dat = matRand_cycle(seed,0);
		pos = *((int32_t*)(&dat));
		pos %= mat->dim * mat->dim;
		mat->data[pos] = ((int8_t*)(&dat))[1];
	}
}
void mat_free(struct Mat *mat){free(mat);}


void mat_generateCorrection(struct Mat *mat,int32_t mutCnt,
		char **outMsg,int32_t *byteOut){
	int32_t redCnt = mutCnt * 2;
	struct ReedSol_Array *arr;
	struct ReedSol_Array *red;
	int32_t sz = sizeof(uint8_t) * (mat->dim * redCnt * 2);
	uint8_t *o = malloc(sz);
	*outMsg = o;
	*byteOut = sz;
	int32_t oidx = 0;

	arr = malloc(sizeof(struct ReedSol_Array));
	arr->length = mat->dim;
	for(int32_t row = 0;row < mat->dim;row++){
		for(int32_t idx = 0;idx < mat->dim;idx++)
			arr->data[idx] = mat->data[idx + row * mat->dim];
		red = reedSol_SplitEncode(arr,redCnt);
		for(int32_t idx = 0;idx < red->length;idx++)
			o[oidx++] = red->data[idx];
		free(red);
	}
	for(int32_t col = 0;col < mat->dim;col++){
		for(int32_t idx = 0;idx < mat->dim;idx++)
			arr->data[idx] = mat->data[col + idx * mat->dim];
		red = reedSol_SplitEncode(arr,redCnt);
		for(int32_t idx = 0;idx < red->length;idx++)
			o[oidx++] = red->data[idx];
		free(red);
	}
	free(arr);
}
void mat_applyCorrection   (struct Mat *mat,
		char *outMsg,int32_t byteOut){
	int32_t redCnt = (byteOut / mat->dim) >> 1;
	struct ReedSol_Array *arr;
	struct ReedSol_Array *red;
	struct ReedSol_Array *fix;
	arr = malloc(sizeof(struct ReedSol_Array));
	arr->length = mat->dim;
	red = malloc(sizeof(struct ReedSol_Array));
	red->length = redCnt;
	int32_t oidx = 0;
	for(int32_t row = 0;row < mat->dim;row++){
		for(int32_t idx = 0;idx < mat->dim;idx++)
			arr->data[idx] = mat->data[idx + row * mat->dim];
		for(int32_t idx = 0;idx < red->length;idx++)
			red->data[idx] = outMsg[oidx++];
		fix = reedSol_SplitDecode(arr,red,redCnt);
		for(int32_t idx = 0;idx < mat->dim;idx++)
			mat->data[idx + row * mat->dim] = fix->data[idx];

		free(fix);
	}
	for(int32_t col = 0;col < mat->dim;col++){
		for(int32_t idx = 0;idx < mat->dim;idx++)
			arr->data[idx] = mat->data[col + idx * mat->dim];
		for(int32_t idx = 0;idx < red->length;idx++)
			red->data[idx] = outMsg[oidx++];
		fix = reedSol_SplitDecode(arr,red,redCnt);
		for(int32_t idx = 0;idx < mat->dim;idx++)
			mat->data[col + idx * mat->dim] = fix->data[idx];

		free(fix);
	}
	free(arr);
	free(red);
	
}
#endif

#endif
