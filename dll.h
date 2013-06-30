#include <stdio.h>
#include <stdlib.h>

struct list_head {
	struct list_head *next, *prev;
};


/* initialize "shortcut links" for empty list */
void list_init(struct list_head *head){
	head->next = head;
	head->prev = head;
	
};

/* insert new entry after the specified head */
void list_add(struct list_head *new, struct list_head *head){

	new->prev = head;
	new->next = head->next;
	new->next->prev = new;
	head->next = new;
};

/* insert new entry before the specified head */
void list_add_tail(struct list_head *new, struct list_head *head){
	new->prev = head->prev;
	new->next = head;
	new->prev->next = new;
	head->prev = new;
};

/* deletes entry from list, reinitializes it (next = prev = 0),
and returns pointer to entry */
struct list_head* list_del(struct list_head *entry){
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
	
	entry->prev = NULL;
	entry->next = NULL; 
	return entry;
};

/* delete entry from one list and insert after the specified head */
void list_move(struct list_head *entry, struct list_head *head){
	struct list_head *tmp = list_del(entry);
	list_add(tmp,head);
};

/* delete entry from one list and insert before the specified head */
void list_move_tail(struct list_head *entry, struct list_head *head){
	struct list_head *tmp = list_del(entry);
	list_add_tail(tmp,head);
};

/* tests whether a list is empty */
int list_empty(struct list_head *head){
	if (head->prev == head && head->next == head)
	{
	return 1;
	printf("%s\n", "Liste ist leer" );
	} else {
	printf("%s\n", "Liste ist nicht leer" );
	return 0;
	}
};

void print_list(struct list_head *list){
	
	struct list_head *start = list;
	struct list_head *temp = list->next;	
	
	while (temp != start)
	{
		temp = temp->next;
	}

};

