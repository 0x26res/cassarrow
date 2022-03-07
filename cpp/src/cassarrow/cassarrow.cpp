#include <cassarrow/cassarrow.h>

#include <iostream>

#include <arrow/builder.h>
#include <arrow/table.h>
#include <parquet/api/io.h>

namespace cassarrow {

class RecordHandler {
public:
  virtual ~RecordHandler() = default;

  virtual std::shared_ptr<arrow::ArrayBuilder> builder() = 0;
  virtual arrow::Status append(std::string const& buffer) = 0;
  virtual arrow::Status appendNull() = 0;
  virtual arrow::Result<std::shared_ptr<arrow::Array>> build() = 0;
};

arrow::Status createHandler(std::shared_ptr<arrow::Field> const& field, std::shared_ptr<RecordHandler>& results);

inline size_t num_leading_zeros(int64_t value) {
  if (value == 0)
    return 64;

#if defined(_MSC_VER)
  unsigned long index;
#if defined(_M_AMD64)
  _BitScanReverse64(&index, value);
#else
  // On 32-bit this needs to be split into two operations
  char isNonzero = _BitScanReverse(&index, (unsigned long)(value >> 32));

  if (isNonzero)
    // The most significant 4 bytes has a bit set, and our index is relative to that.
    // Add 32 to account for the lower 4 bytes that make up our 64-bit number.
    index += 32;
  else {
    // Scan the last 32 bits by truncating the 64-bit value
    _BitScanReverse(&index, (unsigned long)value);
  }
#endif
  // index is the (zero based) index, counting from lsb, of the most-significant 1 bit.
  // For example, a value of 12 (b1100) would return 3. The 4th bit is set, so there are
  // 60 leading zeros.
  return 64 - index - 1;
#else
  return __builtin_clzll(value);
#endif
}

arrow::Status readSome(std::istringstream& stream, const int32_t size, std::string& output) {

  output.resize(size);
  const size_t read_size_raw = stream.readsome(&output[0], size);
  const int read_size = int(read_size_raw);
  if (read_size == size) {
    return arrow::Status::OK();
  } else {
    return arrow::Status::CapacityError("Not enough data to read " + std::to_string(size) + " vs " +
                                        std::to_string(read_size) + " raw " + std::to_string(read_size_raw));
  }
}

arrow::Status readByte(std::istringstream& buffer, uint8_t& value) {
  char charValue = 0;
  if (buffer.readsome(&charValue, 1) != 1) {
    return arrow::Status(arrow::StatusCode::CapacityError, "Could not read byte");
  } else {
    value = static_cast<uint8_t>(charValue);
    return arrow::Status::OK();
  }
}

arrow::Status decode_vint(std::istringstream& buffer, uint64_t& output) {

  uint8_t first_byte;
  ARROW_RETURN_NOT_OK(readByte(buffer, first_byte));

  if (first_byte <= 127) {
    // If this is a multibyte vint, at least the MSB of the first byte
    // will be set. Since that's not the case, this is a one-byte value.
    output = first_byte;
    return arrow::Status::OK();
  } else {
    // The number of consecutive most significant bits of the first-byte tell us how
    // many additional bytes are in this vint. Count them like this:
    // 1. Invert the firstByte so that all leading 1s become 0s.
    // 2. Count the number of leading zeros; num_leading_zeros assumes a 64-bit long.
    // 3. We care about leading 0s in the byte, not int, so subtract out the
    //    appropriate number of extra bits (56 for a 64-bit int).

    // We mask out high-order bits to prevent sign-extension as the value is placed in a 64-bit
    // arg to the num_leading_zeros function.
    int extra_bytes = num_leading_zeros(~first_byte & 0xff) - 56;

    // Build up the vint value one byte at a time from the data bytes.
    // The firstByte contains size as well as the most significant bits of
    // the value. Extract just the value.
    output = first_byte & (0xff >> extra_bytes);
    for (int i = 0; i < extra_bytes; ++i) {
      uint8_t vint_byte = 0;
      ARROW_RETURN_NOT_OK(readByte(buffer, vint_byte));
      output <<= 8;
      output |= vint_byte & 0xff;
    }
    return arrow::Status::OK();
  }
}

inline int64_t decode_zig_zag(uint64_t n) {
  // n is an unsigned long because we want a logical shift right
  // (it should 0-fill high order bits), not arithmetic shift right.
  return (n >> 1) ^ -static_cast<int64_t>(n & 1);
}

inline arrow::Status get_duration(std::string const& buffer, int64_t& nanos) {
  uint64_t decoded;
  std::istringstream stream{buffer};

  ARROW_RETURN_NOT_OK(decode_vint(stream, decoded));
  const int64_t months = static_cast<int64_t>(decode_zig_zag(decoded));

  ARROW_RETURN_NOT_OK(decode_vint(stream, decoded));
  const int64_t days = static_cast<int64_t>(decode_zig_zag(decoded));

  ARROW_RETURN_NOT_OK(decode_vint(stream, decoded));
  const int64_t day_nanos = static_cast<int64_t>(decode_zig_zag(decoded));

  nanos = day_nanos + (days + months * 30) * 86400000000000ll;
  return arrow::Status::OK();
}

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
    return arrow::Status(arrow::StatusCode::CapacityError,
                         "readPrimitive Wrong buffer size " + std::to_string(size) + " vs " +
                             std::to_string(sizeof(T)));
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
  const size_t read_size = stream.readsome(buffer, sizeof(T));
  if (read_size != sizeof(T)) {
    return arrow::Status::CapacityError("Data not available " + std::to_string(sizeof(T)) + " vs " +
                                        std::to_string(read_size));
  }
  return readPrimitive<T>(buffer, sizeof(T), value);
}


arrow::Status readHandler(
    std::istringstream& stream,
    std::shared_ptr<RecordHandler> const& handler,
    std::string& buffer) {
  int32_t elementSize{0};
  ARROW_RETURN_NOT_OK(readPrimitive<int32_t>(stream, elementSize));
  if (elementSize < 0) {
    ARROW_RETURN_NOT_OK(handler->appendNull());
  } else {
    ARROW_RETURN_NOT_OK(readSome(stream, elementSize, buffer));
    ARROW_RETURN_NOT_OK(handler->append(buffer));
  }
  return arrow::Status::OK();
}

class ListHandler : public RecordHandler {
public:
  ListHandler(std::shared_ptr<arrow::ListType> const& listDataType, std::shared_ptr<RecordHandler> const& inner) :
      _inner(inner),
      _builder(std::make_shared<arrow::ListBuilder>(arrow::default_memory_pool(), inner->builder(), listDataType)) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override {
    std::istringstream stream(buffer);
    int32_t elements;
    ARROW_RETURN_NOT_OK(readPrimitive<int32_t>(stream, elements));
    if (elements < 0) {
      ARROW_RETURN_NOT_OK(_builder->AppendNull());
    } else {
      ARROW_RETURN_NOT_OK(_builder->Append());
      for (int32_t element = 0; element < elements; ++element) {
        ARROW_RETURN_NOT_OK(readHandler(stream, _inner, _buffer));
      }
    }
    return arrow::Status::OK();
  }

  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<RecordHandler> _inner;
  std::shared_ptr<arrow::ListBuilder> _builder;
  std::string _buffer;
};


class MapHandler : public RecordHandler {
public:
  MapHandler(std::shared_ptr<arrow::MapType> const& mapDataType,
             std::shared_ptr<RecordHandler> const& keyHandler,
             std::shared_ptr<RecordHandler> const& valueHandler) :
      _keyHandler(keyHandler),
      _valueHandler(valueHandler),
      _builder(std::make_shared<arrow::MapBuilder>(
          arrow::default_memory_pool(), keyHandler->builder(), valueHandler->builder(), mapDataType)) {
  }

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override {
    std::istringstream stream{buffer};
    int32_t elements;
    ARROW_RETURN_NOT_OK(readPrimitive<int32_t>(stream, elements));
    if (elements < 0) {
      ARROW_RETURN_NOT_OK(_builder->AppendNull());
    } else {
      ARROW_RETURN_NOT_OK(_builder->Append());
      for (int32_t element = 0; element < elements; ++element) {
        ARROW_RETURN_NOT_OK(readHandler(stream, _keyHandler, _buffer));
        ARROW_RETURN_NOT_OK(readHandler(stream, _valueHandler, _buffer));
      }
    }
    return arrow::Status::OK();
  }

  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<RecordHandler> const _keyHandler;
  std::shared_ptr<RecordHandler> const _valueHandler;
  std::shared_ptr<arrow::MapBuilder> const _builder;
  std::string _buffer;
};

std::vector<std::shared_ptr<arrow::ArrayBuilder>>
getBuilders(std::vector<std::shared_ptr<RecordHandler>> const& handlers) {
  std::vector<std::shared_ptr<arrow::ArrayBuilder>> results;
  for (std::shared_ptr<RecordHandler> const& handler : handlers) {
    results.push_back(handler->builder());
  }
  return results;
}

class StructHandler : public RecordHandler {
public:
  StructHandler(std::shared_ptr<arrow::StructType> const& structType,
                std::vector<std::shared_ptr<RecordHandler>> const& fieldsHandlers) :
      _fieldsHandlers(fieldsHandlers),
      _builder(std::make_shared<arrow::StructBuilder>(
          structType, arrow::default_memory_pool(), getBuilders(fieldsHandlers))) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override {
    // TODO: Append null to each element?
    return _builder->AppendNull();
  }
  arrow::Status append(std::string const& buffer) override {
    std::istringstream stream(buffer);

    ARROW_RETURN_NOT_OK(_builder->Append());
    for (std::shared_ptr<RecordHandler> const& handler : _fieldsHandlers) {
      ARROW_RETURN_NOT_OK(readHandler(stream, handler, _buffer));
    }

    return arrow::Status::OK();
  }
  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::vector<std::shared_ptr<RecordHandler>> _fieldsHandlers;
  std::shared_ptr<arrow::StructBuilder> _builder;
  std::string _buffer;
};

class Int8Handler : public RecordHandler {
public:
  Int8Handler() : _builder(std::make_shared<arrow::Int8Builder>()) {}
  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else {
      int8_t value{0};
      RETURN_NOT_OK(readPrimitive<int8_t>(buffer, value));
      return _builder->Append(value);
    }
  }
  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::Int8Builder> _builder;
};

class Int16Handler : public RecordHandler {
public:
  Int16Handler() : _builder(std::make_shared<arrow::Int16Builder>()) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else {
      int16_t value;
      RETURN_NOT_OK(readPrimitive<int16_t>(buffer, value));
      return _builder->Append(value);
    }
  }
  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::Int16Builder> _builder;
};

class Int32Handler : public RecordHandler {
public:
  Int32Handler() : _builder(std::make_shared<arrow::Int32Builder>()) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else {
      int32_t value;
      RETURN_NOT_OK(readPrimitive<int32_t>(buffer, value));
      return _builder->Append(value);
    }
  }
  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::Int32Builder> _builder;
};

class BooleanHandler : public RecordHandler {
public:
  BooleanHandler() : _builder(std::make_shared<arrow::BooleanBuilder>()) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  virtual arrow::Status appendNull() override { return _builder->AppendNull(); }
  virtual arrow::Status append(std::string const& buffer) override {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else if (buffer.size() != 1) {
      return arrow::Status(arrow::StatusCode::CapacityError, "Expected one byte");
    } else {
      return _builder->Append(buffer[0] != 0);
    }
  }

  virtual arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::BooleanBuilder> _builder;
};

class Int64Handler : public RecordHandler {
public:
  Int64Handler() : _builder(std::make_shared<arrow::Int64Builder>()) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else {
      int64_t value;
      RETURN_NOT_OK(readPrimitive<int64_t>(buffer, value));
      return _builder->Append(value);
    }
  }

  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::Int64Builder> _builder;
};

class Date32Handler : public RecordHandler {
public:
  Date32Handler() : _builder(std::make_shared<arrow::Date32Builder>()) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else {
      uint32_t value;
      RETURN_NOT_OK(readPrimitive<uint32_t>(buffer, value));
      return _builder->Append(value - DATE_OFFSET);
    }
  }

  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::Date32Builder> _builder;
};

class DurationHandler : public RecordHandler {
public:
  DurationHandler() :
      _builder(std::make_shared<arrow::DurationBuilder>(arrow::duration(arrow::TimeUnit::NANO),
                                                        arrow::default_memory_pool())) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else {
      int64_t nanos;
      ARROW_RETURN_NOT_OK(get_duration(buffer, nanos));
      return _builder->Append(nanos);
    }
  }
  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::DurationBuilder> _builder;
};

class Time64Handler : public RecordHandler {
public:
  Time64Handler() :
      _builder(
          std::make_shared<arrow::Time64Builder>(arrow::time64(arrow::TimeUnit::NANO), arrow::default_memory_pool())) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else {
      int64_t value;
      RETURN_NOT_OK(readPrimitive<int64_t>(buffer, value));
      return _builder->Append(value);
    }
  }
  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::Time64Builder> _builder;
};

class TimestampHandler : public RecordHandler {
public:
  TimestampHandler() :
      _builder(std::make_shared<arrow::TimestampBuilder>(arrow::timestamp(arrow::TimeUnit::MILLI),
                                                         arrow::default_memory_pool())) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else {
      int64_t value = 0;
      RETURN_NOT_OK(readPrimitive<int64_t>(buffer, value));
      return _builder->Append(value);
    }
  }
  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::TimestampBuilder> _builder;
};

class StringHandler : public RecordHandler {
public:
  StringHandler() : _builder(std::make_shared<arrow::StringBuilder>()) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override { return _builder->Append(buffer); }
  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::StringBuilder> _builder;
};

class BinaryHandler : public RecordHandler {
public:
  BinaryHandler() : _builder(std::make_shared<arrow::BinaryBuilder>()) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override { return _builder->Append(buffer); }
  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::BinaryBuilder> _builder;
};

class FixedSizeBinaryHandler : public RecordHandler {
public:
  FixedSizeBinaryHandler(std::shared_ptr<arrow::FixedSizeBinaryType> const& fixedSizeBinaryType) :
      _builder(std::make_shared<arrow::FixedSizeBinaryBuilder>(fixedSizeBinaryType)) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override { return _builder->Append(buffer); }
  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::FixedSizeBinaryBuilder> _builder;
};

class DoubleHandler : public RecordHandler {
public:
  DoubleHandler() : _builder(std::make_shared<arrow::DoubleBuilder>()) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else {
      double value;
      ARROW_RETURN_NOT_OK(readPrimitive<double>(buffer, value));
      return _builder->Append(value);
    }
  }
  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::DoubleBuilder> _builder;
};

class FloatHandler : public RecordHandler {
public:
  FloatHandler() : _builder(std::make_shared<arrow::FloatBuilder>()) {}

  std::shared_ptr<arrow::ArrayBuilder> builder() override { return _builder; }
  arrow::Status appendNull() override { return _builder->AppendNull(); }
  arrow::Status append(std::string const& buffer) override {
    if (buffer.empty()) {
      return _builder->AppendNull();
    } else {
      float value;
      ARROW_RETURN_NOT_OK(readPrimitive<float>(buffer, value));
      return _builder->Append(value);
    }
  }
  arrow::Result<std::shared_ptr<arrow::Array>> build() override { return _builder->Finish(); }

private:
  std::shared_ptr<arrow::FloatBuilder> _builder;
};

arrow::Status createHandler(std::shared_ptr<arrow::DataType> const& dtype, std::shared_ptr<RecordHandler>& results) {
  switch (dtype->id()) {
  case arrow::Type::type::BOOL:
    results = std::make_shared<BooleanHandler>();
    return arrow::Status::OK();
  case arrow::Type::type::INT8:
    results = std::make_shared<Int8Handler>();
    return arrow::Status::OK();
  case arrow::Type::type::INT16:
    results = std::make_shared<Int16Handler>();
    return arrow::Status::OK();
  case arrow::Type::type::INT32:
    results = std::make_shared<Int32Handler>();
    return arrow::Status::OK();
  case arrow::Type::type::INT64:
    results = std::make_shared<Int64Handler>();
    return arrow::Status::OK();
  case arrow::Type::type::DOUBLE:
    results = std::make_shared<DoubleHandler>();
    return arrow::Status::OK();
  case arrow::Type::type::FLOAT:
    results = std::make_shared<FloatHandler>();
    return arrow::Status::OK();
  case arrow::Type::type::DATE32:
    results = std::make_shared<Date32Handler>();
    return arrow::Status::OK();
  case arrow::Type::type::DURATION:
    results = std::make_shared<DurationHandler>();
    return arrow::Status::OK();
  case arrow::Type::type::TIME64:
    results = std::make_shared<Time64Handler>();
    return arrow::Status::OK();
  case arrow::Type::type::TIMESTAMP:
    results = std::make_shared<TimestampHandler>();
    return arrow::Status::OK();
  case arrow::Type::type::STRING:
    results = std::make_shared<StringHandler>();
    return arrow::Status::OK();
  case arrow::Type::type::BINARY:
    results = std::make_shared<BinaryHandler>();
    return arrow::Status::OK();
  case arrow::Type::type::FIXED_SIZE_BINARY: {
    const std::shared_ptr<arrow::FixedSizeBinaryType> fixedSizeBinaryType =
        std::static_pointer_cast<arrow::FixedSizeBinaryType>(dtype);
    results = std::make_shared<FixedSizeBinaryHandler>(fixedSizeBinaryType);
    return arrow::Status::OK();
  }
  case arrow::Type::type::LIST: {
    const std::shared_ptr<arrow::ListType> listType = std::static_pointer_cast<arrow::ListType>(dtype);
    std::shared_ptr<RecordHandler> itemHandler;
    ARROW_RETURN_NOT_OK(createHandler(listType->value_type(), itemHandler));
    results = std::make_shared<ListHandler>(listType, itemHandler);
    return arrow::Status::OK();
  }
  case arrow::Type::type::MAP: {
    const std::shared_ptr<arrow::MapType> mapType = std::static_pointer_cast<arrow::MapType>(dtype);
    std::shared_ptr<RecordHandler> keysHandler;
    std::shared_ptr<RecordHandler> valuesHandler;
    ARROW_RETURN_NOT_OK(createHandler(mapType->key_type(), keysHandler));
    ARROW_RETURN_NOT_OK(createHandler(mapType->item_type(), valuesHandler));
    results = std::make_shared<MapHandler>(mapType, keysHandler, valuesHandler);
    return arrow::Status::OK();
  }
  case arrow::Type::type::STRUCT: {
    std::shared_ptr<arrow::StructType> structType = std::static_pointer_cast<arrow::StructType>(dtype);
    std::vector<std::shared_ptr<RecordHandler>> handlers;
    for (std::shared_ptr<arrow::Field> const& field : structType->fields()) {
      std::shared_ptr<RecordHandler> fieldHandler;
      ARROW_RETURN_NOT_OK(createHandler(field->type(), fieldHandler));
      handlers.push_back(fieldHandler);
    }
    results = std::make_shared<StructHandler>(structType, handlers);
    return arrow::Status::OK();
  }
  default:
    return arrow::Status(arrow::StatusCode::TypeError, "Type not supported: " + dtype->name());
  }
}

arrow::Status parseResults(std::string const& bytes,
                           std::shared_ptr<arrow::Schema> const& schema,
                           std::shared_ptr<arrow::RecordBatch>& results) {

  std::vector<std::shared_ptr<RecordHandler>> handlers(schema->num_fields());
  for (int index = 0; index < schema->num_fields(); ++index) {
    ARROW_RETURN_NOT_OK(createHandler(schema->field(index)->type(), handlers[index]));
  }
  std::istringstream stream{bytes};

  int32_t rows = 0;
  ARROW_RETURN_NOT_OK(readPrimitive<int32_t>(stream, rows));

  std::string buffer;
  for (int32_t row = 0; row < rows; ++row) {
    for (std::shared_ptr<RecordHandler> const& handler : handlers) {
      ARROW_RETURN_NOT_OK(readHandler(stream, handler, buffer));
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
