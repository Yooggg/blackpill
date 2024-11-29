#include "FXRTOS.h"

#include "task_main.h"

extern void FX_Init();
extern void Task_Flash_Func();
void fx_app_init(void)
{
	FX_Init();	// Функция инициализации периферийных устройств

	printf("\r\n----------------------------------\r\n");

	//Task_Main_Init();

	//Task_Web_Init();

	//Task_Flash_Init();
	static fx_thread_t task_flash;
	static int stack_flash[8192*8 / sizeof(int)];

	//fx_mutex_init(&mutex1, FX_MUTEX_CEILING_DISABLED, FX_SYNC_POLICY_DEFAULT);

	fx_thread_init(&task_flash, Task_Flash_Func, NULL, 11, (void*)stack_flash, sizeof(stack_flash), false);
//    static fx_thread_t t0;
//    static uint32_t stk0[512 / sizeof(uint32_t)];
//    fx_thread_init(&t0, blink_fn, NULL, 11, (void*)stk0, sizeof(stk0), false);
//
//    static fx_thread_t t1;
//    static uint32_t stk1[512 / sizeof(uint32_t)];
//
//    fx_sem_init(&sem, 0, 10, FX_SYNC_POLICY_FIFO);
//    fx_thread_init(&t1, timer_fn, NULL, 10, (void*)stk1, sizeof(stk1), false);
//
//    fx_dpc_init(&timer_dpc);
}
