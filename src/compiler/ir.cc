#include "compiler/ir.h"

#include "core/assert.h"

namespace fell {

const IrValue& GetIrValue(const IrProcedure& procedure, IrValueId id) {
  FELL_ASSERT(id.value < procedure.values.size());
  return procedure.values[id.value];
}

}  // namespace fell
