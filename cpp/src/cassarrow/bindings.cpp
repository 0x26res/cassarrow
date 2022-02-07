#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <arrow/builder.h>
#include <arrow/python/pyarrow.h>
#include <arrow/table.h>
#include <parquet/api/io.h>
#include <arpa/inet.h>

namespace cassarrow {

int32_t read_int(std::istringstream& stream) {
  char buffer[sizeof(int32_t)];
  stream.read(buffer, sizeof(int32_t));
  return ntohl(*((int32_t*)buffer));
}

template<class T>
void reverse_value(T& value)
{
  char* data = (char*)&value;
  for(int i=0; i< sizeof(T) /2; i++)
  {
    std::swap(data[i], data[sizeof(T)-i-1]);
  }
}

class RecordHandler {
public:
  virtual ~RecordHandler() = default;

  virtual arrow::Status append(std::string const& buffer) = 0;

  virtual arrow::Result<std::shared_ptr<arrow::Array>> build() = 0;
};

class Int32Handler : public RecordHandler {
public:
  Int32Handler() : _builder(std::make_shared<arrow::Int32Builder>()) {}

  virtual arrow::Status append(std::string const& buffer) {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else if (buffer.size() == sizeof(int32_t)) {
      const int32_t value = ntohl(*((int32_t*)buffer.c_str()));
      return _builder->Append(value);
    } else {
      return arrow::Status(arrow::StatusCode::CapacityError, "Wrong buffer size");
    }
  }

  virtual arrow::Result<std::shared_ptr<arrow::Array>> build() { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::Int32Builder> _builder;
};

class Date32Handler : public RecordHandler {
public:
  Date32Handler() : _builder(std::make_shared<arrow::Date32Builder>()) {}

  virtual arrow::Status append(std::string const& buffer) {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else if (buffer.size() == sizeof(int32_t)) {
      const uint32_t value = uint32_t((unsigned char)(buffer[0]) << 24 | (unsigned char)(buffer[1]) << 16 |
                                      (unsigned char)(buffer[2]) << 8 | (unsigned char)(buffer[3]));
      const uint32_t offset = 1 << 31;
      return _builder->Append(value - offset);
    } else {
      return arrow::Status(arrow::StatusCode::CapacityError, "Wrong buffer size");
    }
  }

  virtual arrow::Result<std::shared_ptr<arrow::Array>> build() { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::Date32Builder> _builder;
};

class TimestampHandler : public RecordHandler {
public:
  TimestampHandler() :
      _builder(std::make_shared<arrow::TimestampBuilder>(arrow::timestamp(arrow::TimeUnit::NANO),
                                                         arrow::default_memory_pool())) {}

  virtual arrow::Status append(std::string const& buffer) {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else if (buffer.size() == sizeof(int64_t)) {
      const int64_t value = *((int64_t*)buffer.c_str());
      reverse_value(value);
      return _builder->Append(value);
    } else {
      return arrow::Status(arrow::StatusCode::CapacityError, "Wrong buffer size");
    }
  }

  virtual arrow::Result<std::shared_ptr<arrow::Array>> build() { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::TimestampBuilder> _builder;
};

class DoubleHandler : public RecordHandler {
public:
  DoubleHandler() : _builder(std::make_shared<arrow::DoubleBuilder>()) {}

  virtual arrow::Status append(std::string const& buffer) {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else if (buffer.size() == sizeof(double)) {
      double value;
      memcpy(&value, buffer.c_str(), sizeof(double));
      reverse_value(value);
      return _builder->Append(value);
    } else {
      return arrow::Status(arrow::StatusCode::CapacityError, "Wrong buffer size");
    }
  }

  virtual arrow::Result<std::shared_ptr<arrow::Array>> build() { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::DoubleBuilder> _builder;
};

arrow::Status createHandler(std::shared_ptr<arrow::Field> const& field, std::shared_ptr<RecordHandler>& results) {
  switch (field->type()->id()) {
  case arrow::Type::type::INT32:
    results = std::make_shared<Int32Handler>();
    return arrow::Status::OK();
  case arrow::Type::type::DOUBLE:
    results = std::make_shared<DoubleHandler>();
    return arrow::Status::OK();
  case arrow::Type::type::DATE32:
    results = std::make_shared<Date32Handler>();
    return arrow::Status::OK();
  case arrow::Type::type::TIMESTAMP:
    results = std::make_shared<TimestampHandler>();
    return arrow::Status::OK();
  default:
    return arrow::Status(arrow::StatusCode::TypeError,
                         "Type not supported: " + field->name() + " " + field->type()->name());
  }
}


arrow::Status parseResults(std::string const& bytes,
                           std::shared_ptr<arrow::Schema> const& schema,
                           std::shared_ptr<arrow::RecordBatch>& results) {

  std::vector<std::shared_ptr<RecordHandler>> handlers(schema->num_fields());
  for (int index = 0; index < schema->num_fields(); ++index) {
    ARROW_RETURN_NOT_OK(createHandler(schema->field(index), handlers[index]));
  }
  std::istringstream stream{bytes};

  const int32_t rows = read_int(stream);
  std::cout << "rows " << rows << std::endl;

  std::string buffer;
  for (int32_t row = 0; row < rows; ++row) {
    for (std::shared_ptr<RecordHandler> const& handler : handlers) {
      const int32_t size = read_int(stream);
      if (size == 0) {
        buffer.clear();
      } else {
        buffer.resize(size);
        stream.readsome(&buffer[0], size);
      }
      ARROW_RETURN_NOT_OK(handler->append(buffer));
    }
  }

  arrow::ArrayVector arrays;
  for (std::shared_ptr<RecordHandler> const& handler : handlers) {
    auto array = handler->build();
    ARROW_RETURN_NOT_OK(array.status());
    arrays.push_back(array.ValueOrDie());
  }
  results = arrow::RecordBatch::Make(schema, rows, arrays);

  return arrow::Status::OK();
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
