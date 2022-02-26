#pragma once

#include <arrow/builder.h>

namespace cassarrow {

arrow::Status parseResults(std::string const& bytes,
                           std::shared_ptr<arrow::Schema> const& schema,
                           std::shared_ptr<arrow::RecordBatch>& results);
} // namespace cassarrow
