#ifndef REED_SOL_H_
#define REED_SOL_H_

#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>

struct ReedSol_Array{
	int32_t length;
	uint8_t data[256];
};
//
struct ReedSol_Array *reedSol_HoleEncode(
		struct ReedSol_Array *msg,int32_t redCnt);
struct ReedSol_Array *reedSol_RedEncode(
		struct ReedSol_Array *msg,int32_t redCnt);
struct ReedSol_Array *reedSol_Decode(
		struct ReedSol_Array *msg,int32_t redCnt);
struct ReedSol_Array *reedSol_SplitDecode(
		struct ReedSol_Array *msg,
		struct ReedSol_Array *red,int32_t redCnt);
struct ReedSol_Array *reedSol_Copy(struct ReedSol_Array *from);
void reedSol_Zero(struct ReedSol_Array *from);
//

#ifdef REED_SOL_DO_TEST
#define REED_SOL_IMPL
#endif
#ifdef REED_SOL_IMPL
#define REED_SOL_ALL
#endif

#ifdef REED_SOL_ALL
///////
void reedsol_Init(void);
uint8_t reedsol_Add(uint8_t left,uint8_t right);
uint8_t reedsol_Sub(uint8_t left,uint8_t right);
uint8_t reedsol_Mul(uint8_t left,uint8_t right);
uint8_t reedsol_Div(uint8_t left,uint8_t right);
uint8_t reedsol_Pow(uint8_t left,uint8_t right);

struct ReedSol_Array *reedSol_genGenerator(uint8_t length);
struct ReedSol_Array *reedSol_polMod(
		struct ReedSol_Array *msg,struct ReedSol_Array *generator);
uint8_t reedSol_testFor(struct ReedSol_Array *msg,uint8_t pos);
struct ReedSol_Array *reedSol_errorArray(
		struct ReedSol_Array *msg,uint8_t redundend);
void reedSol_polDivision(
		struct ReedSol_Array *left,struct ReedSol_Array *right,
		struct ReedSol_Array **hole,struct ReedSol_Array **frac
		);
// this will free (*error)
void reedSol_Euclidean(
		struct ReedSol_Array *error,uint8_t errCount,
		struct ReedSol_Array **errVal,struct ReedSol_Array **errLoc
		);
struct ReedSol_Array *reedSol_findErr(
		struct ReedSol_Array *pol,int8_t length);
struct ReedSol_Array *reedSol_findErrIdx(
		struct ReedSol_Array *pol,int8_t length);
uint8_t reedSol_testForDerive(struct ReedSol_Array *pol,uint8_t pos);
uint8_t reedSol_getErrDiff(
		struct ReedSol_Array *val,struct ReedSol_Array *loc,
		uint8_t pos);
struct ReedSol_Array *reedSol_errDiffs(
		struct ReedSol_Array *val,struct ReedSol_Array *loc,
		struct ReedSol_Array *pos);

struct ReedSol_Array *reedSol_Encode(
		struct ReedSol_Array *msg,int32_t redCnt);
struct ReedSol_Array *reedSol_SplitEncode(
		struct ReedSol_Array *msg,int32_t redCnt);
struct ReedSol_Array *reedSol_Decode(
		struct ReedSol_Array *msg,int32_t redCnt);
struct ReedSol_Array *reedSol_SplitDecode(
		struct ReedSol_Array *msg,
		struct ReedSol_Array *red,int32_t redCnt);
#endif



#ifdef REED_SOL_IMPL

// REED-SOLOMON LITTLE-ENDIAN
// Pro: same order as polynomial
// Con: redundancy before message
// Note: exactly one byte big
const uint8_t REED_SOL_Prime = 29;
uint8_t REED_SOL_Exp[256];
uint8_t REED_SOL_Log[256];
struct ReedSol_Array *reedSol_Copy(struct ReedSol_Array *from){
	size_t sz = sizeof(uint8_t) * (256 - from->length);
	if(256 > from->length)sz = 0;
	sz += sizeof(struct ReedSol_Array);
	//
	struct ReedSol_Array *arr = malloc(sz);
	arr->length = from->length;
	int32_t i;
	for(i = 0;i < from->length;i++)
		arr->data[i] = from->data[i];
	for(;i < 256;i++)
		arr->data[i] = 0;
	return arr;
}
void reedSol_Zero(struct ReedSol_Array *from){
	for(int32_t i = 0;i < from->length || i < 256;i++){
		from->data[i] = 0;
	}
}
char *reedSol_ToString(struct ReedSol_Array *arr){
	size_t sz = sizeof(char) * (arr->length * 4 + 16);
	char *all = malloc(sz);
	int32_t offset = 0;
	offset += snprintf(&(all[offset]),sz-offset,"[");
	for(int32_t idx = 0;idx < arr->length;idx++){
		offset += snprintf(&(all[offset]),sz-offset,"%i,",arr->data[idx]);
	}
	offset += snprintf(&(all[offset]),sz-offset,"]");
	return all;
}

void reedsol_Init(void){
	uint32_t x = 1;
	for(int32_t cnt = 0;cnt < 255;cnt++){
		REED_SOL_Exp[cnt] = x;
		REED_SOL_Log[x] = cnt;
		x = ((x << 1) & 0xff) ^ ((x & 0x80) ? REED_SOL_Prime : 0);
	}
}
uint8_t reedsol_Add(uint8_t left,uint8_t right){return left ^ right;}
uint8_t reedsol_Sub(uint8_t left,uint8_t right){return left ^ right;}
uint8_t reedsol_Mul(uint8_t left,uint8_t right){
	if(left == 0 || right == 0)return 0;
	uint32_t pos = REED_SOL_Log[left] + REED_SOL_Log[right];
	return REED_SOL_Exp[pos % 255];
}
uint8_t reedsol_Div(uint8_t left,uint8_t right){
	if(left == 0 || right == 0)return 0;
	uint32_t pos = 255 + REED_SOL_Log[left] - REED_SOL_Log[right];
	return REED_SOL_Exp[pos % 255];
}
uint8_t reedsol_Pow(uint8_t left,uint8_t right){
	uint32_t pos = REED_SOL_Log[left] * right;
	return REED_SOL_Exp[pos % 255];
}

struct ReedSol_Array *reedSol_genGenerator(uint8_t length){
	struct ReedSol_Array *outArr = malloc(sizeof(struct ReedSol_Array));
	outArr->length = length + 1;
	for(int32_t countup = 0;countup >> length == 0;countup++){
		uint8_t akku = 1;
		for(int32_t x = 0;x < length;x++){
			if((countup >> x) & 1)continue;
			akku = reedsol_Mul(akku,reedsol_Pow(2,x));
		}
		int32_t pos = __builtin_popcount(countup);
		outArr->data[pos] = reedsol_Add(outArr->data[pos],akku);
	}
	return outArr;
}
struct ReedSol_Array *reedSol_polMod(
		struct ReedSol_Array *msg,struct ReedSol_Array *generator
){
	struct ReedSol_Array *cp = reedSol_Copy(msg); 
	struct ReedSol_Array *outArr = malloc(sizeof(struct ReedSol_Array));
	outArr->length = generator->length - 1;
	reedSol_Zero(outArr);
	uint8_t v;
	uint8_t genMul;
	int32_t pos;
	for(int32_t idx = msg->length - 1;idx >= 0;idx--){
		v = cp->data[idx];
		// MAYBE XXX the div
		v = reedsol_Div(v,generator->data[generator->length - 1]);
		for(int32_t x = 0;x < generator->length - 1;x++){
			// this to keep a from b apart
			// idx as v
			// idx - 1 to test...
			pos = idx + x - generator->length + 1;
			genMul = reedsol_Mul(generator->data[x],v);
			if(pos >= 0){
				cp->data[pos] =
					reedsol_Add(genMul,cp->data[pos]);
			}else{
				pos = generator->length + pos - 1;
				outArr->data[pos] =
					reedsol_Add(genMul,outArr->data[pos]);
			}
		}
	}
	free(cp);
	return outArr;
}
uint8_t reedSol_testFor(struct ReedSol_Array *msg,uint8_t pos){
	uint8_t akku = 0;
	uint8_t powing = 1;
	for(int32_t idx = 0;idx < msg->length;idx++){
		uint8_t mue = reedsol_Mul(powing,msg->data[idx]);
		akku = reedsol_Add(akku,mue);
		powing = reedsol_Mul(powing,pos);
	}
	return akku;
}
struct ReedSol_Array *reedSol_errorArray(
		struct ReedSol_Array *msg,uint8_t redundend
){
	struct ReedSol_Array *arr = malloc(sizeof(struct ReedSol_Array));
	arr->length = redundend;
	for(int32_t idx = 0;idx < redundend;idx++){
		uint8_t pow = reedsol_Pow(2,idx);
		arr->data[idx] = reedSol_testFor(msg,pow);
	}
	return arr;
}
void reedSol_polDivision(
		struct ReedSol_Array *left,struct ReedSol_Array *right,
		struct ReedSol_Array **hole,struct ReedSol_Array **frac
){
	struct ReedSol_Array *outF = reedSol_Copy(left);
	struct ReedSol_Array *outH = malloc(sizeof(struct ReedSol_Array));
	outF->length = right->length - 1;
	outH->length = left->length - outF->length;
	//reedSol_Zero(outF);
	//outH->length = generator->length - 1;
	reedSol_Zero(outH);
	uint8_t v;
	uint8_t genMul;
	int32_t pos;
	int32_t holdx = left->length - right->length;
	for(int32_t idx = left->length - 1;idx >= right->length - 1;idx--){
		v = outF->data[idx];
		v = reedsol_Div(v,right->data[right->length - 1]);
		outH->data[holdx--] = v;
		for(int32_t x = 0;x < right->length - 1;x++){
			// this to keep a from b apart
			pos = idx + x - right->length + 1;
			genMul = reedsol_Mul(right->data[x],v);
			outF->data[pos] =
				reedsol_Add(genMul,outF->data[pos]);
		}
	}
	//
	*hole = outH;
	*frac = outF;
}

// this will free (*error)
void reedSol_Euclidean(
		struct ReedSol_Array *error,uint8_t errCount,
		struct ReedSol_Array **errVal,struct ReedSol_Array **errLoc
){
	struct ReedSol_Array *a0 = malloc(sizeof(struct ReedSol_Array));
	a0->length = error->length + 1;
	reedSol_Zero(a0);
	a0->data[a0->length - 1] = 1;
	struct ReedSol_Array *a1 = error;
	struct ReedSol_Array *anew;
	struct ReedSol_Array *b0 = malloc(sizeof(struct ReedSol_Array));
	b0->length = 1;
	reedSol_Zero(b0);
	struct ReedSol_Array *b1 = malloc(sizeof(struct ReedSol_Array));
	reedSol_Zero(b1);
	b1->length = 1;
	b0->data[0] = 1;
	struct ReedSol_Array *full;
	uint8_t calc;
	while(a1->length > errCount){
		reedSol_polDivision(a0,a1,&full,&anew);

		for(int32_t x = 0;x < b0->length;x++){
			calc = reedsol_Mul(full->data[0],b0->data[x]);
			b1->data[x] = reedsol_Add(calc,b1->data[x]);
			calc = reedsol_Mul(full->data[1],b0->data[x]);
			b1->data[x + 1] = reedsol_Add(calc,b1->data[x + 1]);
		}
		b1->length = b0->length + 1;
		*(size_t*)(&b1) ^= ((uintptr_t)b0);
		*(size_t*)(&b0) ^= ((uintptr_t)b1);
		*(size_t*)(&b1) ^= ((uintptr_t)b0);
		free(full);
		free(a0);
		a0 = a1;
		a1 = anew;
	}
	free(a0);
	free(b1);
	*errVal = a1;
	*errLoc = b0;
}
struct ReedSol_Array *reedSol_findErr(
		struct ReedSol_Array *pol,int8_t length
){
	struct ReedSol_Array *out = malloc(sizeof(struct ReedSol_Array));
	out->length = length;
	reedSol_Zero(out);
	for(int32_t x = 0;x < length;x++){
		uint8_t overPow = reedsol_Pow(2,x);
		out->data[x] = reedSol_testFor(pol,overPow);
	}
	return out;
}
struct ReedSol_Array *reedSol_findErrIdx(
		struct ReedSol_Array *pol,int8_t length
){
	struct ReedSol_Array *out = malloc(sizeof(struct ReedSol_Array));
	out->length = 0;
	int32_t outIdx = 0;
	reedSol_Zero(out);
	for(int32_t x = 0;x < length;x++){
		uint8_t overPow = reedsol_Div(1,reedsol_Pow(2,x));
		uint8_t val = reedSol_testFor(pol,overPow);
		//printf("for %i => %i\n",x,val);
		if(val == 0)
			out->data[outIdx++] = x;
	}
	out->length = outIdx;
	return out;
}
struct ReedSol_Array *reedSol_testInMsg(
		struct ReedSol_Array *pol,struct ReedSol_Array *pos
){
	struct ReedSol_Array *out = malloc(sizeof(struct ReedSol_Array));
	out->length = pos->length;
	int32_t pos_;
	for(int32_t idx = 0;idx < pos->length;idx++){
		pos_ = pos->data[idx]; 
		out->data[idx] = reedSol_testFor(pol,pos_);
	}
	return out; 
}
uint8_t reedSol_testForDerive(struct ReedSol_Array *pol,uint8_t pos){
	uint8_t akku = 0;
	uint8_t possq = reedsol_Pow(2,pos);
	possq = reedsol_Div(1,reedsol_Mul(possq,possq));
	uint8_t posak = 1;
	for(int32_t x = 1;x < pol->length;x += 2){
		akku = reedsol_Add(akku,reedsol_Mul(pol->data[x],posak));
		posak = reedsol_Mul(posak,possq);
	}
	return akku; 
}
uint8_t reedSol_getErrDiff(
		struct ReedSol_Array *val,struct ReedSol_Array *loc,
		uint8_t pos
){
	uint8_t pow2 = reedsol_Pow(2,pos);
	uint8_t inver = reedsol_Div(1,pow2);
	uint8_t akku = reedsol_Mul(pow2,reedSol_testFor(val,inver));
	return reedsol_Div(akku,reedSol_testForDerive(loc,pos));
}
struct ReedSol_Array *reedSol_errDiffs(
		struct ReedSol_Array *val,struct ReedSol_Array *loc,
		struct ReedSol_Array *pos
){
	struct ReedSol_Array *outArr = malloc(sizeof(struct ReedSol_Array));
	outArr->length = pos->length;
	for(int32_t idx = 0;idx < pos->length;idx++){
		outArr->data[idx] = reedSol_getErrDiff(val,loc,pos->data[idx]);
	}
	return outArr;
}


struct ReedSol_Array *reedSol_Encode(
		struct ReedSol_Array *msg,int32_t redCnt){
	reedsol_Init();
	struct ReedSol_Array *generator = reedSol_genGenerator(redCnt);
	struct ReedSol_Array *redund = reedSol_polMod(msg,generator);
	for(int32_t idx = 0;idx < msg->length;idx++)
		redund->data[redund->length + idx] = msg->data[idx];
	redund->length += msg->length;
	free(generator);
	return redund;
}
struct ReedSol_Array *reedSol_SplitEncode(
		struct ReedSol_Array *msg,int32_t redCnt){
	reedsol_Init();
	struct ReedSol_Array *generator = reedSol_genGenerator(redCnt);
	struct ReedSol_Array *redund = reedSol_polMod(msg,generator);
	free(generator);
	return redund;
}
struct ReedSol_Array *reedSol_Decode(
		struct ReedSol_Array *msg,int32_t redCnt){
	struct ReedSol_Array *cp = reedSol_Copy(msg);
	struct ReedSol_Array *tst = reedSol_findErr(msg,redCnt);
	for(int32_t idx = 0;idx < tst->length;idx++){
		if(tst->data[idx] != 0)
			goto Reed_Sol_Decode_Tamper;
	}
	free(tst);
	return cp;
Reed_Sol_Decode_Tamper:
	struct ReedSol_Array *errVal,*errLoc, *errPos,*errDif;
	// free(tst)
	reedSol_Euclidean(tst,redCnt / 2,&errVal,&errLoc);
	uint8_t invert = errLoc->data[0];
	for(int32_t idx = 0;idx < errVal->length;idx++)
		errVal->data[idx] = reedsol_Mul(errVal->data[idx],invert);
	for(int32_t idx = 0;idx < errLoc->length;idx++)
		errLoc->data[idx] = reedsol_Mul(errLoc->data[idx],invert);
	errPos = reedSol_findErrIdx(errLoc,msg->length);
	errDif = reedSol_errDiffs(errVal,errLoc,errPos);

	int32_t pos;
	for(int32_t idx = 0;idx < errPos->length;idx++){
		pos = errPos->data[idx];
		cp->data[pos] = reedsol_Add(cp->data[pos],errDif->data[idx]);
	}
	free(errVal);
	free(errLoc);
	free(errPos);
	free(errDif);
	//
	return cp;
}
struct ReedSol_Array *reedSol_SplitDecode(
		struct ReedSol_Array *msg,
		struct ReedSol_Array *red,int32_t redCnt);



#ifdef REED_SOL_DO_TEST
int main(int argc,char **argv){
	reedsol_Init();

	struct ReedSol_Array *tst = malloc(sizeof(struct ReedSol_Array));
	int32_t red = 2;
	tst->length = 4;
	char *string = "Hallo Welt order so";
	for(int32_t p = 0;string[p];p++){
		tst->data[p] = string[p];
		tst->length = p;
	}
	struct ReedSol_Array *enc = reedSol_Encode(tst,red);
	printf("%s\n",reedSol_ToString(enc));
	enc->data[3] = 0;
	struct ReedSol_Array *dec = reedSol_Decode(enc,red);
	printf("%s\n",reedSol_ToString(dec));
	/*/
	printf("tst: %s\n",reedSol_ToString(tst));
	//struct ReedSol_Array *gn = reedSol_genGenerator(3);
	struct ReedSol_Array *hol,*frac;
	//reedSol_polDivision(tst,gn,&hol,&frac);
	reedSol_Euclidean(tst,red / 2,&hol,&frac);
	//printf("gn: %s\n",reedSol_ToString(gn));
	printf("hol: %s\n",reedSol_ToString(hol));
	printf("fra: %s\n",reedSol_ToString(frac));
	//  **/
	return 0;
}
#endif

#endif
#endif
