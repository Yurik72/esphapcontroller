/*
 * storage_ex.h
 *
 *  Created on: 2 дек. 2019 г.
 *      Author: yurik
 */

#ifndef COMPONENTS_COMMON_HOMEKIT_INCLUDE_HOMEKIT_STORAGE_EX_H_
#define COMPONENTS_COMMON_HOMEKIT_INCLUDE_HOMEKIT_STORAGE_EX_H_



#ifdef EX_STORAGE_CHAR

char * get_ex_storage();
int get_ex_storage_size();
int init_storage_ex(char* szdata,int size);
typedef void(*callback_function)(void);

void set_callback_storage(callback_function val);
void on_storage_change();
#endif

#endif /* COMPONENTS_COMMON_HOMEKIT_INCLUDE_HOMEKIT_STORAGE_EX_H_ */
