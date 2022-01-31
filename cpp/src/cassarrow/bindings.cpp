#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <arrow/builder.h>
#include <arrow/python/pyarrow.h>
#include <arrow/table.h>
#include <parquet/api/io.h>


#include <cassarrow/cassarrow.h>

namespace {
    void sayHello() { std::cout << "HELLO" << std::endl; }


    std::shared_ptr<arrow::Table> getTable() {
      arrow::StringBuilder builder;
      PARQUET_THROW_NOT_OK(builder.Append("FOO"));
      arrow::ArrayVector arrays = {builder.Finish().ValueOrDie()};
      std::shared_ptr<arrow::Schema> schema = arrow::schema({arrow::field("foo", arrow::utf8())});

      std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, arrays);
      return table;
    }

    void testArrow() {
      arrow::py::import_pyarrow();

      auto table = getTable();

      std::cout << *table->schema() << std::endl;
      std::cout << table->num_rows() << std::endl;
    }


    pybind11::object pyGetTable() {

      auto table = getTable();
      return pybind11::reinterpret_steal<pybind11::object>(arrow::py::wrap_table(table));
    }

} // namespace

PYBIND11_MODULE(bindings, m) {
  m.doc() = "pybind11 example plugin";

  m.def("say_hello", &sayHello, "Test stdout");
  m.def("test_arrow", &testArrow, "Test arrow");
  m.def("get_table", &pyGetTable, "Test arrow return table");
}