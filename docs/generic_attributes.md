# Generic attribute map

## Data types

- humblenet_AttributeMap* &mdash; an opaque type for holding the attributes
- humblenet_AttributeMapIT* &mdash; an opaque object representing iterating the attributes

## Methods

### Allocation
`humblenet_AttributeMap* humblenet_attributes_new`
- Creates a new attribute set

`void humblenet_attributes_free(humblenet_AttributeMap*);`
- frees the attribute set

### Manipulating

`void humblenet_attributes_set(humblenet_AttributeMap*, const char* key, const char* value);`
- set or replace an attribute on the attribute set

`void humblenet_attributes_unset(humblenet_AttributeMap*, const char* key);`
- removes a key from the attribute set

### Querying

`ha_bool humblenet_attributes_get(humblenet_AttributeMap*, const char* key, size_t buff_size, char* value_buff);`
- fetches the value for a key from the attribute set into the specified buffer
- returns true if the key exists, false otherwise

`ha_bool humblenet_attributes_contains(humblenet_AttributeMap*, const char* key);`
- returns true if the attribute set contains the specified key

`size_t humblenet_attributes_size(humblenet_AttributeMap*);`
- return how many attributes are in the attribute set

### Iterating

`humblenet_AttributeMapIT* humblenet_attributes_iterate(humblenet_AttributeMap*);`
- starts an iteration of the values

`ha_bool humblenet_attributes_iterate_get_key(humblenet_AttributeMapIT*, size_t buff_size, char* key_buff);`
- gets the key for the current entry

`humblenet_AttributeMapIT* humblenet_attributes_iterate_next(humblenet_AttributeMapIT*);`
- moves to the next entry
- returns NULL when there are no more entries (auto frees)

`void humblenet_attributes_iterate_free(humblenet_AttributeMapIT*);`
- frees the iterator
- only needed if you stop iterating before the end
