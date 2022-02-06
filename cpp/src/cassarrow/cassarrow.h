#pragma once

#include <arrow/builder.h>

namespace cassarrow {
class Int32Handler {
public:
  Int32Handler();

  void append(std::string const& bytes);

private:
  std::shared_ptr<arrow::Int32Builder> _builder;
};

} // namespace cassarrow
