#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <arrow/python/pyarrow.h>

#include <cassarrow/cassarrow.h>

namespace {
void sayHello() { std::cout << "HELLO" << std::endl; }

pybind11::object pyParseResults(pybind11::bytes const& bytes, pybind11::object schema) {
  arrow::Result<std::shared_ptr<arrow::Schema>> arrowSchema = arrow::py::unwrap_schema(schema.ptr());

  if (!arrowSchema.ok()) {
    throw std::runtime_error("Couldn't read schema");
  }

  std::shared_ptr<arrow::RecordBatch> recordBatch;
  arrow::Status status = cassarrow::parseResults(bytes, arrowSchema.ValueOrDie(), recordBatch);
  if (!status.ok()) {
    throw std::runtime_error(status.ToString());
  } else {
    return pybind11::reinterpret_steal<pybind11::object>(arrow::py::wrap_batch(recordBatch));
  }
}

} // namespace

PYBIND11_MODULE(_cassarrow, m) {
  m.doc() = "pybind11 example plugin";

  m.def("say_hello", &sayHello, "Test stdout");
  m.def("parse_results", &pyParseResults, "Parse Results from cassandra");
}
