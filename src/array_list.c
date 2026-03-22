#include "../include/array_list.h"

#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

void array_list_init(array_list *a, size_t value_size)
{
    // initialise les propriétés de l'array_list a
    // réserve un espace mémoire initial de 8 éléments
    a->capacity = 8;
    a->size = 0;
    a->value_size = value_size;
    a->data = malloc(a->capacity * value_size);
}

void array_list_deinit(array_list *a)
{
    // libère la mémoire allouée
    // réinitialise les propriétés de l'array_list a
    free(a->data);
}

void array_list_reserve(array_list *a, size_t nb_values)
{
    // fait en sorte que la capacité de l'array_list a soit d'au moins nb_values
    if (a->capacity < nb_values)
        a->data = realloc(a->data, nb_values * a->value_size);
    a->capacity = nb_values;
}

void const *array_list_get_pointer(array_list const *a, size_t index)
{
    // retourne un pointeur const sur la case d'index index
    return (char *)a->data + index * a->value_size;
}

void *array_list_get_pointer_mut(array_list *a, size_t index)
{
    // retourne un pointeur mutable (non-const) sur la case d'index index
    // (cette fonction sert expliciter les intentions et à éviter d'avoir à faire
    // des cast dans le code appelant)

    return (char *)a->data + index * a->value_size;
}

void array_list_get_value(array_list const *a, size_t index, void *value)
{
    // copie la valeur contenue dans la case d'index index à l'adresse pointée par value
    memcpy(value, array_list_get_pointer(a, index), a->value_size);
}

void array_list_set_value(array_list *a, size_t index, void const *value)
{
    // copie la mémoire pointée par value dans la case d'index index
    memcpy(array_list_get_pointer_mut(a, index), value, a->value_size);
}

void array_list_push_back(array_list *a, void const *value)
{
    // copie la mémoire pointée par value dans une nouvelle case à la fin de l'array_list a
    array_list_reserve(a, (a->capacity) + 1);
    array_list_set_value(a, a->size, value);
    a->size++;
}
void array_list_push_front(array_list *a, void const *value)
{
    // copie la mémoire pointée par value dans une nouvelle case au début de l'array_list a
    array_list_reserve(a, (a->capacity) + 1);
    memmove((char *)a->data + a->value_size, a->data, a->size * a->value_size);
    array_list_set_value(a, 0, value);
    a->size++;
}

void array_list_erase(array_list *a, size_t index)
{
    // supprime la case d'index index (effectue un décalage des cases suivantes)
    memmove(array_list_get_pointer_mut(a, index), array_list_get_pointer_mut(a, index + 1), (a->size - index - 1) * a->value_size);
    a->size--;
    memset(array_list_get_pointer_mut(a, a->size), 0, a->value_size);
}

void array_list_swap(array_list *a, size_t index1, size_t index2)
{
    // échange les données qui se trouvent aux index index1 et index2
    if (index1 == index2)
        return;

    void *p1 = array_list_get_pointer_mut(a, index1);
    void *p2 = array_list_get_pointer_mut(a, index2);

    char buffer[a->value_size];

    memcpy(buffer, p1, a->value_size);
    memcpy(p1, p2, a->value_size);
    memcpy(p2, buffer, a->value_size);
}

size_t array_list_find(array_list const *a, void const *value, bool (*equals)(void const *a, void const *b))
{
    for (size_t i = 0; i < a->size; i++)
    {
        if (equals(value, array_list_get_pointer(a, i)))
            return i;
    }
    return (size_t)-1;
}
