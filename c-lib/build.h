#ifndef BUILD_H_
#define BUILD_H_

//#include"strings.h"
#include<stdarg.h>
#include<stdlib.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<sys/wait.h>
#include<stdint.h>
#include<string.h>
#include<dirent.h>

#ifndef STRINGS_H_
struct Strings{
	int32_t size;
	int32_t count;
	char **strs;
};
#endif
struct Tasks{
	struct Strings *args;
	struct Strings *env;
	char *file;
	pid_t *tasks;
	int32_t length;
	int32_t capacity;
};
// true for, yes I want this.
typedef int(*FILTER_DIR)(struct dirent*);

void task_set_file(struct Tasks *t,char *file);
void task_add_args(struct Tasks *t,char *arg);
void task_add_args_many(struct Tasks *t,...);
void task_add_env(struct Tasks *t,char *arg);
// will not free elements!
void task_clear_args(struct Tasks *t);
// will not free elements!
void task_clear_env(struct Tasks *t);
void task_add_args_merge(struct Tasks *t,struct Strings *str);
void task_add_env_merge(struct Tasks *t,struct Strings *str);
void task_exec(struct Tasks *t);
void task_await(struct Tasks *t);
void task_touch(struct Tasks *t);
// only shows the name, not paths
struct Strings *task_get_direntry         (char *path);
// only shows the name, not paths
struct Strings *task_get_direntry_filtered(char *path,FILTER_DIR filter);

#ifndef STRINGS_H_
void    strings_append  (struct Strings **str,              char *chars);
char   *strings_get     (struct Strings **str,int32_t   idx            );
void    strings_set     (struct Strings **str,int32_t   idx,char *chars);
char   *strings_pop     (struct Strings **str                          );
char   *strings_remove  (struct Strings **str,int32_t   idx            );
void    strings_insert  (struct Strings **str,int32_t   idx,char *chars);
char   *strings_join    (struct Strings **str,char   *  sep            );
// will not free elements!
void    strings_clear   (struct Strings **str                          );
// will not free elements!
void    strings_free    (struct Strings **str                          );
int32_t strings_count   (struct Strings **str                          );
char  **strings_get_list(struct Strings **str                          );
void    strings_compact (struct Strings **str                          );
//
void    strings_append_many(struct Strings **str,            char *chars,...);
void    strings_insert_many(struct Strings **str,int32_t idx,char *chars,...);

void strings_append_list(struct Strings **str,            struct Strings **other);
void strings_insert_list(struct Strings **str,int32_t idx,struct Strings **other);
void strings_insert_sublist(struct Strings **str,
		int32_t idx,struct Strings **other,
		int32_t start,int32_t end);

// this is destructive! free(*str);
void string_append(char **str,char *from);
// 1 for "matching"
int string_end_cmp(char *basis,char *match);
char* string_copy(char *str);
#endif

void task_set_file(struct Tasks *t,char *file){
	if(t == NULL){return;} t->file = file;
	if(t->args == NULL)strings_append(&(t->args), file);
}
void task_add_args(struct Tasks *t,char *arg){
	if(t == NULL){return;}
	if(t->args == NULL)
		strings_append(&(t->args), t->file);
	strings_append(&(t->args), arg);
}
void task_add_args_many(struct Tasks *t,...){
	va_list ap;
	va_start(ap, t);
	char *get;
	if(t == NULL){return;}
	if(t->args == NULL) strings_append(&(t->args), t->file);
	do{
		get = va_arg(ap,char*);
		if(get == NULL)break;
		strings_append(&(t->args), get);
	}while(get);
}
void task_add_env(struct Tasks *t,char *arg){
	if(t == NULL){return;} strings_append(&(t->env), arg);
}
void task_clear_args(struct Tasks *t){strings_clear(&(t->args));}
void task_clear_env(struct Tasks *t){strings_clear(&(t->env));}
void task_add_args_merge(struct Tasks *t,struct Strings *str){
	if(t == NULL){return;}
	if(t->args == NULL) strings_append(&(t->args), t->file);
	strings_append_list(&(t->args), &str);
}
void task_add_env_merge(struct Tasks *t,struct Strings *str){
	if(t == NULL){return;}
	//if(t->args == NULL) strings_append(&(t->args), "");
	strings_append_list(&(t->env), &str);
}

void task_exec(struct Tasks *t){
	if(t->capacity == 0){
		t->capacity = 64;
		t->tasks = malloc(sizeof(pid_t) * t->capacity);
	}
	if(t->length >= t->capacity){
		t->capacity *= 2;
		t->tasks = realloc(t->tasks, sizeof(pid_t) * t->capacity);
	}
	pid_t added = fork();
	if(added < 0){
		perror("No Fork!\n");
		return;
	}
	if(added == 0){
		char **largs = strings_get_list(&(t->args));
		char **lenv = strings_get_list(&(t->env));
		//execve(t->file,largs,lenv);
		execvp(t->file,largs);
		free(largs);
		free(lenv);
		exit(0);
		return;
	}
	//printf("%s ",t->file);
	for(int idx = 0;idx < t->args->count;idx++)
		printf("%s ",t->args->strs[idx]);
	printf("\n");
	t->tasks[t->length++] = added;
}
void task_await(struct Tasks *t){
	for(int idx = 0;idx < t->length;idx++){
		waitpid(t->tasks[idx],NULL,0);
	}
	printf("Finished %d Tasks\n",t->length);
	t->length = 0;
}
void task_touch(struct Tasks *t){
	int cnt = 0;
	for(int idx = 0;idx < t->length;idx++){
		pid_t p = waitpid(t->tasks[idx],NULL,WNOHANG);
		if(p == 0)continue;
		cnt++;
		t->length--;
		t->tasks[idx] = t->tasks[t->length];
		idx--;
	}
	printf("Finished %d Tasks, %d left\n",cnt,t->length);
}
struct Strings *task_get_direntry(char *path){
	struct Strings *sout = NULL;
	DIR *d;
	struct dirent *dp;
	d = opendir(path);
	while((dp = readdir(d)) != NULL){
		strings_append(&sout,string_copy(dp->d_name));
	}
	closedir(d);
	return sout;
}
struct Strings *task_get_direntry_filtered(char *path,FILTER_DIR filter){
	struct Strings *sout = NULL;
	DIR *d;
	struct dirent *dp;
	d = opendir(path);
	while((dp = readdir(d)) != NULL){
		if(filter(dp))
			strings_append(&sout,string_copy(dp->d_name));
	}
	closedir(d);
	return sout;
}


#ifndef STRINGS_H_
#define STRINGS_H_
void    strings_append  (struct Strings **str,char   *chars){
	if(*str == NULL){
		*str = malloc(sizeof(struct Strings));
		(*str)->size  = 64;
		(*str)->count = 0;
		(*str)->strs  = malloc(sizeof(char*) * (*str)->size);
		(*str)->strs[0] = NULL;
	}
	else if((*str)->count + 1 >= (*str)->size){
		(*str)->size *= 2;
		(*str)->strs  = malloc(sizeof(char*) * (*str)->size);
	}
	(*str)->strs[(*str)->count] = chars;
	(*str)->count++;
	(*str)->strs[(*str)->count] = NULL;
}
char   *strings_get     (struct Strings **str,int32_t   idx){
	if(*str == NULL)return NULL;
	if(idx < 0)return NULL;
	if((*str)->count < idx)return NULL;
	return (*str)->strs[idx];
}
void    strings_set     (struct Strings **str,int32_t   idx,char *chars){
	if(*str == NULL)return;
	if(idx < 0)return;
	if((*str)->count < idx)return;
	(*str)->strs[idx] = chars;
	if((*str)->count == idx){(*str)->count++;}
}
char   *strings_pop     (struct Strings **str){
	if(*str == NULL)return NULL;
	if((*str)->count <= 0)return NULL;
	(*str)->count--;
	char *ch = (*str)->strs[(*str)->count];
	(*str)->strs[(*str)->count] = NULL;
	return ch;
}
char   *strings_remove  (struct Strings **str,int32_t   idx){
	if(*str == NULL)return NULL;
	if(idx < 0)return NULL;
	if((*str)->count < idx)return NULL;
	if((*str)->count <= 0)return NULL;
	return NULL;
}
void    strings_insert  (struct Strings **str,int32_t   idx,char *chars){
	if(*str == NULL){
		*str = malloc(sizeof(struct Strings));
		(*str)->size  = 64;
		(*str)->count = 0;
		(*str)->strs  = malloc(sizeof(char*) * (*str)->size);
		(*str)->strs[0] = NULL;
	}
	else if((*str)->count + 1 >= (*str)->size){
		(*str)->size *= 2;
		(*str)->strs  = malloc(sizeof(char*) * (*str)->size);
	}
	if(idx < 0)idx = 0;
	if(idx > (*str)->count)idx = (*str)->count;
	//
	for(int32_t off = (*str)->count;off >= idx;off--)
		(*str)->strs[off + 1] = (*str)->strs[off];
	(*str)->strs[idx] = chars;
	(*str)->count++;
}

char   *strings_join    (struct Strings **str,char   *  sep){
	if(*str == NULL)return NULL;
	if((*str)->count == 0)return NULL;
	int32_t len = strlen(sep);
	int32_t cnt = ((*str)->count - 1) * len;
	for(int32_t idx = 0;idx < (*str)->count;idx++){
		cnt += strlen((*str)->strs[idx]);
	}
	cnt++;
	//
	char *out = malloc(sizeof(char) * cnt);
	int32_t oidx = 0;
	for(int32_t j = 0;(*str)->strs[0][j];j++)
		out[oidx++] = (*str)->strs[0][j];
	for(int32_t idx = 1;idx < (*str)->count;idx++){
		for(int32_t j = 0;sep[j];j++)
			out[oidx++] = sep[j];
		for(int32_t j = 0;(*str)->strs[idx][j];j++)
			out[oidx++] = (*str)->strs[idx][j];
	}
	out[cnt - 1] = 0;
	return out;
}
void    strings_clear   (struct Strings **str){
	if(*str == NULL)return;
	free((*str)->strs);
	free(*str);
	*str = NULL;
}
void    strings_free    (struct Strings **str){ strings_clear(str); }
int32_t strings_count   (struct Strings **str){
	if(*str == NULL)return 0;
	return (*str)->count;
}
char  **strings_get_list(struct Strings **str){
	if(*str == NULL)return NULL;
	return (*str)->strs;
}
void    strings_compact (struct Strings **str){
	if(*str == NULL)return;
	int32_t count = 0;
	for(int32_t idx = 0;idx < (*str)->count;idx++){
		if((*str)->strs[idx] != NULL){
			(*str)->strs[count] = (*str)->strs[idx];
			count++;
		}
	}
}

void strings_append_many(struct Strings **str,            char *chars,...){
	va_list ap;
	va_start(ap, chars);
	char *get = chars;
	while (get) {
		strings_append(str,get);
		get = va_arg(ap,char*);
	}
}
void strings_insert_many(struct Strings **str,int32_t idx,char *chars,...){
	chars = chars;
	va_list ap;
	va_start(ap, chars);
	char *get = chars;
	while (get) {
		strings_insert(str,idx,get);
		get = va_arg(ap,char*);
		idx++;
	}
}

void strings_append_list(struct Strings **str,            struct Strings **other){
	if(*other == NULL)return;
	for(int32_t jdx = 0;jdx < (*other)->count;jdx++){
		strings_append(str,(*other)->strs[jdx]);
	}
}
void strings_insert_list(struct Strings **str,int32_t idx,struct Strings **other){
	if(*other == NULL)return;
	if(idx < 0)idx = 0;
	if(*str == NULL)idx = 0;
	else if(idx > (*str)->count)idx = (*str)->count;
	for(int32_t jdx = 0;jdx < (*other)->count;jdx++){
		strings_insert(str,idx,(*other)->strs[jdx]);
		idx++;
	}
}
void strings_insert_sublist(struct Strings **str,
		int32_t idx,struct Strings **other,
		int32_t start,int32_t end){
	if(*other == NULL)return;
	if(idx < 0)idx = 0;
	if(*str == NULL)idx = 0;
	else if(idx > (*str)->count)idx = (*str)->count;
	// asdf
	if(start > end)return;
	if(start < 0)start = 0;
	if(end > (*other)->count)end = (*other)->count;
	for(int32_t jdx = start;jdx < end;jdx++){
		strings_insert(str,idx,(*other)->strs[jdx]);
		idx++;
	}
}

void string_append(char **str,char *from){
	int32_t len = strlen(*str);
	len += strlen(from);
	len++;
	char *out = malloc(sizeof(char) * len);
	int32_t idx = 0;
	while((out[idx] = (*str)[idx]))idx++;
	int32_t jdx = 0;
	while((out[idx++] = from[jdx++]));
	out[idx] = 0;
	free(*str);
	*str = out;
}
int string_end_cmp(char *basis,char *match){
	char *e1,*e2;
	for(e1=basis;*e1 != 0;e1++);
	for(e2=match;*e2 != 0;e2++);
	for(;e1 >= basis && e2 >= match;e2--,e1--){
		if(*e1 == *e2)continue;
		return 0;
	}
	if(e2 < match)return 1;
	return 0;
}
char* string_copy(char *str){
	int len = strlen(str) + 1;
	char *ou = malloc(sizeof(char) * len);
	memcpy(ou,str,len);
	return ou;
}
#endif


#endif
