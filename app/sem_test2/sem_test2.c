#include <hellfire.h>


void taskX(void)
{
	while(1)
		printf("%s \n", hf_selfname());
}

void app_main(void){
	hf_spawn(taskX, 7, 2, 7, "task a", 1024);//1 perido, 2 capacidade 3 dead (1 e 3 iguais)
	hf_spawn(taskX, 10, 4, 10, "task b", 1024);
	hf_spawn(taskX, 15, 4, 15, "task c", 1024);
	hf_spawn(taskX, 25, 2, 25, "task d", 1024);
	hf_spawn(taskX, 32, 1, 32, "task e", 1024);
}
