// #include "linked_list.h"
// #include "common.h"

// ListPath* list_init(DirectoryEntry *dir_entry) {
//   ListPath* list = (ListPath*)malloc(sizeof(ListPath));
//   list->dir_entry = dir_entry;
//   list->next = NULL;
//   return list;
// }

// void list_insert(ListPath* list, DirectoryEntry* dir_entry) {
//   ListPath* new_item = (ListPath*)malloc(sizeof(ListPath));
//   new_item->dir_entry = dir_entry;
//   list->next = new_item;
//   new_item->next = NULL;
//   return;
// }

// DirectoryEntry* current_dir(ListPath* list){
//   if(list->next == NULL)
//     return list->dir_entry;

//   current_dir(list->next);
// }

// void print_path(ListPath* list){
//   if(list == NULL)
//     return;
//   print("%s ", list->dir_entry->name);
//   print_path(list->next);

// }