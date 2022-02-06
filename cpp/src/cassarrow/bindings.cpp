#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <arrow/builder.h>
#include <arrow/python/pyarrow.h>
#include <arrow/table.h>
#include <parquet/api/io.h>

namespace cassarrow {
    class RecordHandler {
    public:
        virtual ~RecordHandler() = default;

        virtual arrow::Status append(std::string const &buffer) = 0;

        virtual arrow::Result<std::shared_ptr<arrow::Array>> build() = 0;
    };

    class Int32Handler : public RecordHandler {
    public:
        Int32Handler() : _builder(std::make_shared<arrow::Int32Builder>()) {}

        virtual arrow::Status append(std::string const &buffer) {
          if (buffer.size() != sizeof(int32_t)) {
            return arrow::Status(arrow::StatusCode::CapacityError, "Wrong buffer size");
          }
          const int32_t value = int32_t((unsigned char) (buffer[0]) << 24 | (unsigned char) (buffer[1]) << 16 |
                                        (unsigned char) (buffer[2]) << 8 | (unsigned char) (buffer[3]));
          return _builder->Append(value);
        }

        virtual arrow::Result<std::shared_ptr<arrow::Array>> build() { return _builder->Finish(); }

    private:
        std::shared_ptr<arrow::Int32Builder> _builder;
    };


    class DoubleHandler : public RecordHandler {
    public:
        DoubleHandler() : _builder(std::make_shared<arrow::DoubleBuilder>()) {}

        virtual arrow::Status append(std::string const &buffer) {
          if (buffer.size() != sizeof(double)) {
            return arrow::Status(arrow::StatusCode::CapacityError, "Wrong buffer size");
          }
          const double value = *((double*)buffer.c_str());
          return _builder->Append(value);
        }

        virtual arrow::Result<std::shared_ptr<arrow::Array>> build() { return _builder->Finish(); }

    private:
        std::shared_ptr<arrow::DoubleBuilder> _builder;
    };

    arrow::Status createHandler(std::shared_ptr<arrow::Field> const &field, std::shared_ptr<RecordHandler> &results) {
      switch (field->type()->id()) {
        case arrow::Type::type::INT32:
          results = std::make_shared<Int32Handler>();
          return arrow::Status::OK();
        case arrow::Type::type::DOUBLE:
          results = std::make_shared<DoubleHandler>();
          return arrow::Status::OK();
        default:
          return arrow::Status(arrow::StatusCode::TypeError,
                               "Type not supported: " + field->name() + " " + field->type()->name());
      }
    }

    arrow::Status parseResults(std::string const &bytes,
                               std::shared_ptr<arrow::Schema> const &schema,
                               std::shared_ptr<arrow::RecordBatch> &results) {

      std::vector<std::shared_ptr<RecordHandler>> handlers(schema->num_fields());
      for (int index = 0; index < schema->num_fields(); ++index) {
        ARROW_RETURN_NOT_OK(createHandler(schema->field(index), handlers[index]));
      }

      return arrow::Status(arrow::StatusCode::NotImplemented, "This function isn't finished");
    }

} // namespace cassarrow

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

    pybind11::object pyParseResults(pybind11::bytes const &bytes, pybind11::object schema) {
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

PYBIND11_MODULE(bindings, m) {
  m.doc() = "pybind11 example plugin";

  m.def("say_hello", &sayHello, "Test stdout");
  m.def("test_arrow", &testArrow, "Test arrow");
  m.def("get_table", &pyGetTable, "Test arrow return table");
  m.def("parse_results", &pyParseResults, "Test arrow parse results");
}
