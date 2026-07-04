
#include <stdio.h>
#include "lib/build.h"
#include <dirent.h>


int filter(struct dirent *dp){
	if(dp->d_name[1] == '.')return 0;
	return dp->d_name[0] == 'a';
}
int main(int argc,char **argv){
	struct Tasks t = {0};
	task_set_file(&t,"gcc");
	task_add_args_many(&t,"./bootstrap.c","-o","boot","-g",NULL);
	task_exec(&t);
	task_clear_args(&t);
	printf("%s\n",argv[0]);
	if(!string_end_cmp(argv[0],"/boot")){
		if(remove(argv[0]) == 0)
			printf("Init done! use:\n./boot\n");
		else
			fprintf(stdout,"Error: %s couldn't be deleted",argv[0]);
		task_await(&t);
		return 0;
	}
	//
	struct Strings *fls = task_get_direntry_filtered(".",&filter);
	//struct Strings *fls = task_get_direntry_filtered("..",&filter);
	for(int32_t idx = 0;idx < fls->count;idx++){
		char *outPath = malloc(sizeof(char) * 512);
		printf("- %s\n",fls->strs[idx]);
		snprintf(outPath,512,"./build/%s.so",fls->strs[idx]);
		//
		task_add_args_many(&t, fls->strs[idx],"-fPIC",
				"-shared","-o",outPath,NULL);
		task_exec(&t);
		free(outPath);
		task_clear_args(&t);
	}
	strings_free(&fls);
	//
	task_add_args_many(&t,"./main.c","-o","run",NULL);
	task_exec(&t);
	task_clear_args(&t);
	//
	task_await(&t);
	return 0;
}
