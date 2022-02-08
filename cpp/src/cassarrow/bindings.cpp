#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <arrow/builder.h>
#include <arrow/python/pyarrow.h>
#include <arrow/table.h>
#include <parquet/api/io.h>

namespace cassarrow {

const uint32_t DATE_OFFSET = 1 << 31;

template <class T>
void reverse_value(T& value) {
  char* data = (char*)&value;
  for (size_t i = 0; i < sizeof(T) / 2; ++i) {
    std::swap(data[i], data[sizeof(T) - i - 1]);
  }
}

template <class T>
arrow::Status readPrimitive(const char* buffer, const size_t size, T& value) {
  if (size != sizeof(T)) {
    return arrow::Status(arrow::StatusCode::CapacityError, "Wrong buffer size");
  } else {
    value = *((T*)buffer);
    if (arrow::Endianness::Native == arrow::Endianness::Little) {
      reverse_value<T>(value);
    }
    return arrow::Status::OK();
  }
}

template <class T>
arrow::Status readPrimitive(std::string const& buffer, T& value) {
  return readPrimitive<T>(buffer.c_str(), buffer.size(), value);
}

template <class T>
arrow::Status readPrimitive(std::istringstream& stream, T& value) {
  char buffer[sizeof(T)];
  stream.read(buffer, sizeof(T));
  return readPrimitive<T>(buffer, sizeof(T), value);
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
    } else {
      int32_t value;
      RETURN_NOT_OK(readPrimitive<int32_t>(buffer, value));
      return _builder->Append(value);
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
    } else {
      uint32_t value;
      RETURN_NOT_OK(readPrimitive<uint32_t>(buffer, value));
      return _builder->Append(value - DATE_OFFSET);
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
    } else {
      int64_t value = 0;
      RETURN_NOT_OK(readPrimitive<int64_t>(buffer, value));
      return _builder->Append(value * 1000000ll);
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
    } else {
      double value;
      ARROW_RETURN_NOT_OK(readPrimitive<double>(buffer, value));
      return _builder->Append(value);
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

  int32_t rows = 0;
  ARROW_RETURN_NOT_OK(readPrimitive<int32_t>(stream, rows));

  std::string buffer;
  for (int32_t row = 0; row < rows; ++row) {
    for (std::shared_ptr<RecordHandler> const& handler : handlers) {
      int32_t size = 0;
      ARROW_RETURN_NOT_OK(readPrimitive<int32_t>(stream, size));
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

PYBIND11_MODULE(bindings, m) {
  m.doc() = "pybind11 example plugin";

  m.def("say_hello", &sayHello, "Test stdout");
  m.def("parse_results", &pyParseResults, "Parse Results from cassandra");
}
