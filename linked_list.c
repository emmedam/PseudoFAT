#include "linked_list.h"

ListPath* list_init(DirectoryEntry *dir_entry) {
  ListPath* list = (ListPath*)malloc(sizeof(ListPath));
  list->dir_entry = dir_entry;
  list->next = NULL;
  return list;
}

void list_insert(ListPath* list, ListPath* new_item) {
    if (list->next == NULL){
        list->next = new_item;
        new_item->next = NULL;
        return;
    }
   list_insert(list->next, new_item);
}

DirectoryEntry* current_dir(ListPath* list){
    if(list->next == NULL)
        return list->dir_entry;
    return current_dir(list->next);
}

void print_path(ListPath* list){
    if(list == NULL)
        return;
    printf(COLOR_BOLD_BLUE "/" COLOR_OFF);
    printf(COLOR_BOLD_BLUE "%s" COLOR_OFF, list->dir_entry->name);
    print_path(list->next);
}

void remove_last(ListPath *list){
    if(list->next == NULL)
        return;
    
    if(list->next->next == NULL){
        free(list->next);
        list->next = NULL;
        return;
    }
    remove_last(list->next);
}

int lenght(ListPath *list){
    if(list == NULL)
        return 0;
    return 1 + lenght(list->next);
}

ListPath* reset_path(ListPath *list){
    ListPath* next = list -> next;
    ListPath* curr = list -> next;
    while(next != NULL){
        next = curr -> next;
        free(curr);
        curr = next;
    }
    list -> next = NULL;
    return list;
}