#ifndef LINK_LIST_H
#define LINK_LIST_H

#include "list.h"

typedef struct customer_info_obj
{
    int id;
    int remain_time;
    bool has_scanner;
    struct list_head list;
} customer_info_obj_t;

typedef struct list_object_struct
{
	int    id;
    int    meta_data;
    int    counter;
    struct list_head list;
    struct list_head customer_list;
} list_object_struct_t;

list_object_struct_t*  create_list(struct list_head* head, int id, int metadata);
customer_info_obj_t* list_count_down(struct list_head* head, list_object_struct_t* ptr);
void  add_to_list(list_object_struct_t* list_head, int id, bool has_scanner);
void delete_from_list(struct list_head* head, list_object_struct_t *list_ptr, customer_info_obj_t* ptr);
void print_list(struct list_head* head);
void  add_to_list_shopping(list_object_struct_t* list_head, int id, bool has_scanner, int shopping_time);

#endif 

