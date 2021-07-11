#ifndef HUMBLENET_ATTRIBUTES_INTERNAL_H
#define HUMBLENET_ATTRIBUTES_INTERNAL_H

#include "humblenet_attributes.h"
#include "humblenet_types_internal.h"

humblenet_AttributeMap* humblenet_attributes_new_from(const humblenet::AttributeMap&);

humblenet::AttributeMap& humblenet_attributes_get_map(humblenet_AttributeMap*);

#endif /* HUMBLENET_ATTRIBUTES_INTERNAL_H */