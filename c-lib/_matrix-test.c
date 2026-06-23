
#define REED_SOL_IMPL
#define MATRIX_IMPL
#include"reedSol.h"
#include"matrix.h"
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

void matShow(struct Mat *left){
	int32_t pos;
	for(int32_t yover = 0;yover < left->dim;yover++){
		for(int32_t xover = 0;xover < left->dim;xover++){
			pos = xover + yover * left->dim;
			printf("%02x ",left->data[pos]);
		}
		printf("\n");
	}
	printf("\n");
}
void matDiff(struct Mat *left,struct Mat *right){
	if(left->dim != right->dim){
		printf("left-dim:%i right-dim:%i\n",left->dim,right->dim);
		return;
	}
	int32_t pos;
	for(int32_t yover = 0;yover < left->dim;yover++){
		for(int32_t xover = 0;xover < left->dim;xover++){
			pos = xover + yover * left->dim;
			if(left->data[pos] == right->data[pos]){
				putchar('1');
			}else{
				putchar('0');
			}
		}
		putchar('\n');
	}
	putchar('\n');
}

int main(int argc,char **argv){
	int32_t mutations = 8;
	int32_t dimension = 32;
	struct Mat *base;
	struct Mat *matA, *matB, *matAM, *matMB;
	struct Mat *matAM_B, *matMB_A;
	struct MatSeed *rng = matRand_newRand();
	base = mat_new(dimension,rng);
	matRand_cycle(rng,64);
	//
	matA = mat_new(dimension,rng);
	matB = mat_new(dimension,rng);
	//
	//matShow(base);
	//matShow(matA);
	//matShow(matB);
	//
	matAM = mat_mul(matA,base);
	matMB = mat_mul(base,matB);
	// mutate here
	mat_mutate(matAM,mutations,rng);
	mat_mutate(matMB,mutations,rng);
	//
	matAM_B = mat_mul(matAM,matB);
	matMB_A = mat_mul(matA,matMB);
	//matShow(matAM_B);
	//matShow(matMB_A);
	matDiff(matAM_B,matMB_A);

	int32_t leng;
	char *data;

	mat_generateCorrection(matAM_B,mutations,&data,&leng);
	mat_applyCorrection   (matMB_A,data,leng);
	//matShow(matMB_A);
	//matShow(matAM_B);
	matDiff(matAM_B,matMB_A);

	//
	free(base);
	free(matA);
	free(matB);
	free(matAM);
	free(matMB);
	free(matAM_B);
	free(matMB_A);
	free(rng);
}
