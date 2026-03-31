#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H
#include <stdlib.h>
#include <stdbool.h>


typedef struct array_list
{
    void* data;
    size_t capacity;
    size_t size;
    size_t value_size;

} array_list;

// initialisation et libération :
void array_list_init(array_list *a, size_t value_size);
void array_list_deinit(array_list *a);

//réservation de mémoire :
void array_list_reserve(array_list *a, size_t nb_values);

//manipulation des données :
// retourne un pointeur const sur la case d'index index
void const *array_list_get_pointer(array_list const *a, size_t index);
// retourne un pointeur mutable (non-const) sur la case d'index index
// (cette fonction sert expliciter les intentions et à éviter d'avoir à faire
// des cast dans le code appelant)
void *array_list_get_pointer_mut(array_list *a, size_t index);
// copie la valeur contenue dans la case d'index index à l'adresse pointée par value
void array_list_get_value(array_list const *a, size_t index, void *value);
// copie la mémoire pointée par value dans la case d'index index
void array_list_set_value(array_list *a, size_t index, void const *value);
// copie la mémoire pointée par value dans une nouvelle case à la fin de l'array_list a
void array_list_push_back(array_list *a, void const *value);
// copie la mémoire pointée par value dans une nouvelle case au début de l'array_list a
void array_list_push_front(array_list *a, void const *value);
// supprime la case d'index index (effectue un décalage des cases suivantes)
void array_list_erase(array_list *a, size_t index);

// échange de donnée, recherche, . . .
void array_list_swap(array_list *a, size_t index1, size_t index2);
size_t array_list_find(array_list const *a, void const *value, bool (*equals)(void const *a, void const *b));

#endif
