#include "generator.h"

extern void state_transfer8(const char *mod, const char *name, uint8 instance,
	uint8 *data, uint32 size);

extern void state_transfer32(const char *mod, const char *name, uint8 instance,
                      uint32 *data, uint32 size);

#define state_save_register_UINT8(mod, ins, name, val, size) \
  state_transfer8(mod, name, ins, val, size)
#define state_save_register_UINT16(mod, ins, name, val, size) \
  state_transfer16(mod, name, ins, val, size)
#define state_save_register_UINT32(mod, ins, name, val, size) \
  state_transfer32(mod, name, ins, val, size)

#define state_save_register_INT8(mod, ins, name, val, size) \
  state_transfer8(mod, name, ins, val, size)
#define state_save_register_INT16(mod, ins, name, val, size) \
  state_transfer16(mod, name, ins, val, size)
#define state_save_register_INT32(mod, ins, name, val, size)	\
  state_transfer32(mod, name, ins, cast_to_void_ptr(val), size)

#define state_save_register_int(mod, ins, name, val) do {		\
  uint32 val_u32_ = *(val); 						\
  state_transfer32(mod, name, ins, &val_u32_, 1);			\
} while (0)

#define state_save_register_func_postload(a) a()
