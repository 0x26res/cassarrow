#define BOOST_TEST_MODULE CassarrowTest

#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test.hpp>

#include <cassarrow/cassarrow.h>

namespace cassarrow {
BOOST_AUTO_TEST_SUITE(CassarrowTest)

BOOST_AUTO_TEST_CASE(TestParseResultsEmptyDataEmptySchema) {

  std::shared_ptr<arrow::RecordBatch> batch;
  BOOST_CHECK_EQUAL(arrow::Status::CapacityError("Data not available 4 vs 0"), parseResults(std::string(), arrow::schema({}), batch));
}


BOOST_AUTO_TEST_CASE(TestParseResultsEmptyDataWithSchema) {

  std::shared_ptr<arrow::RecordBatch> batch;
  auto schema = arrow::schema({arrow::field("column_1", arrow::int32())});
  BOOST_CHECK_EQUAL(arrow::Status::CapacityError("Data not available 4 vs 0"), parseResults(std::string(), schema, batch));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace cassarrow