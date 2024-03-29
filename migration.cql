CREATE KEYSPACE cassarrow
    WITH REPLICATION = {
        'class' : 'SimpleStrategy',
        'replication_factor' : 1
        };

CREATE TABLE cassarrow.time_series
(
    event_date      date,
    instrument_id   int,
    event_timestamp timestamp,
    value           double,
    PRIMARY KEY (event_date, instrument_id, event_timestamp)
);

CREATE TABLE cassarrow.simple_primitives
(
    partition_key varchar,
    secondary_key varchar,
    ascii_col     ascii,
    bigint_col    bigint,
    blob_col      blob,
    boolean_col   boolean,
    date_col      date,
    double_col    double,
    duration_col  duration,
    float_col     float,
    int_col       int,
    smallint_col  smallint,
    text_col      text,
    time_col      time,
    timestamp_col timestamp,
    tinyint_col   tinyint,
    varchar_col   varchar,

    PRIMARY KEY (partition_key, secondary_key)
);


INSERT INTO cassarrow.simple_primitives JSON '{
  "partition_key": "test",
  "secondary_key": "empty"
}';
INSERT INTO cassarrow.simple_primitives JSON '{
  "partition_key": "test",
  "secondary_key": "valid",
  "ascii_col": "hello world",
  "bigint_col": 1234567890,
  "blob_col": "0x0000000000000003",
  "boolean_col": false,
  "date_col": "2021-12-14",
  "double_col": 3.14,
  "duration_col": "P4Y6M3DT12H30M5S",
  "float_col": 2.718,
  "int_col": 1234,
  "smallint_col": 280,
  "text_col": "ABC",
  "time_col": "08:10:22.123",
  "timestamp_col": "2017-05-05 21:01:29.123",
  "tinyint_col": 8,
  "varchar_col": "abc"
}';

CREATE TABLE cassarrow.primitives
(
    partition_key varchar,
    secondary_key varchar,

    ascii_col     ascii,
    bigint_col    bigint,
    blob_col      blob,
    boolean_col   boolean,
    -- counter_col   counter, -- Cannot mix counter and non counter columns in the same table
    date_col      date,
    decimal_col   decimal,
    double_col    double,
    duration_col  duration,
    float_col     float,
    inet_col      inet,
    int_col       int,
    smallint_col  smallint,
    text_col      text,
    time_col      time,
    timestamp_col timestamp,
    timeuuid_col  timeuuid,
    tinyint_col   tinyint,
    uuid_col      uuid,
    varchar_col   varchar,
    varint_col    varint,

    PRIMARY KEY (partition_key, secondary_key)
);


INSERT INTO cassarrow.primitives JSON '{
  "partition_key": "test",
  "secondary_key": "empty",
  "ascii_col": null,
  "bigint_col": null,
  "blob_col": null,
  "boolean_col": null,
  "date_col": null,
  "decimal_col": null,
  "double_col": null,
  "duration_col": null,
  "float_col": null,
  "inet_col": null,
  "int_col": null,
  "smallint_col": null,
  "text_col": null,
  "time_col": null,
  "timestamp_col": null,
  "timeuuid_col": null,
  "tinyint_col": null,
  "uuid_col": null,
  "varchar_col": null,
  "varint_col": null
}';


INSERT INTO cassarrow.primitives JSON '
{
  "partition_key": "test",
  "secondary_key": "valid",
  "ascii_col": "hello world",
  "bigint_col": 1234567890,
  "blob_col": "0x0000000000000003",
  "boolean_col": false,
  "date_col": "2021-12-14",
  "decimal_col": 123.456,
  "double_col": 3.14,
  "duration_col": "P4Y6M3DT12H30M5S",
  "float_col": 2.718,
  "inet_col": "fe80::1ff:fe23:4567:890a",
  "int_col": 1234,
  "smallint_col": 280,
  "text_col": "ABC",
  "time_col": "08:10:22.123",
  "timestamp_col": "2017-05-05 00:00:00.000",
  "timeuuid_col": "50554d6e-29bb-11e5-b345-feff819cdc9f",
  "tinyint_col": 8,
  "uuid_col": "50554d6e-29bb-11e5-b345-feff819cdc9f",
  "varchar_col": "abc",
  "varint_col": 1234567890123
}
';

INSERT INTO cassarrow.simple_primitives JSON '{
  "partition_key": "test",
  "secondary_key": "duration_1",
  "duration_col": "P4Y6M3DT12H30M5S"
}';

INSERT INTO cassarrow.simple_primitives JSON '{
  "partition_key": "test",
  "secondary_key": "duration_2",
  "duration_col": "2mo10d12h"
}';


INSERT INTO cassarrow.simple_primitives JSON '{
  "partition_key": "test",
  "secondary_key": "duration_3",
  "duration_col": "P0004-06-03T12:30:05"
}';


CREATE TABLE cassarrow.simple_list
(
    partition_key varchar,
    secondary_key varchar,

    ascii_col     list<ascii>,
    bigint_col    list<bigint>,
    blob_col      list<blob>,
    boolean_col   list<boolean>,
    date_col      list<date>,
    double_col    list<double>,
    duration_col  list<duration>,
    float_col     list<float>,
    int_col       list<int>,
    smallint_col  list<smallint>,
    text_col      list<text>,
    time_col      list<time>,
    timestamp_col list<timestamp>,
    tinyint_col   list<tinyint>,
    varchar_col   list<varchar>,

    PRIMARY KEY (partition_key, secondary_key)
);


INSERT INTO cassarrow.simple_list JSON '
{
  "partition_key": "test",
  "secondary_key": "empty"
}';

INSERT INTO cassarrow.simple_list JSON '
{
  "partition_key": "test",
  "secondary_key": "valid",
  "ascii_col": [
    "hello world"
  ],
  "bigint_col": [
    1234567890
  ],
  "blob_col": [
    "0x0000000000000003"
  ],
  "boolean_col": [
    false
  ],
  "date_col": [
    "2021-12-14"
  ],
  "double_col": [
    3.14
  ],
  "duration_col": [
    "P4Y6M3DT12H30M5S"
  ],
  "float_col": [
    2.718
  ],
  "int_col": [
    1234
  ],
  "smallint_col": [
    280
  ],
  "text_col": [
    "ABC"
  ],
  "time_col": [
    "08:10:22.123"
  ],
  "timestamp_col": [
    "2017-05-05 00:00:00.000"
  ],
  "tinyint_col": [
    8
  ],
  "varchar_col": [
    "abc"
  ]
}
';


CREATE TABLE cassarrow.simple_set
(
    partition_key varchar,
    secondary_key varchar,

    ascii_col     set<ascii>,
    bigint_col    set<bigint>,
    blob_col      set<blob>,
    boolean_col   set<boolean>,
    date_col      set<date>,
    double_col    set<double>,
    float_col     set<float>,
    int_col       set<int>,
    smallint_col  set<smallint>,
    text_col      set<text>,
    time_col      set<time>,
    timestamp_col set<timestamp>,
    tinyint_col   set<tinyint>,
    varchar_col   set<varchar>,

    PRIMARY KEY (partition_key, secondary_key)
);

INSERT INTO cassarrow.simple_set JSON '
{
  "partition_key": "test",
  "secondary_key": "valid",
  "ascii_col": [
    "hello world"
  ],
  "bigint_col": [
    1234567890
  ],
  "blob_col": [
    "0x0000000000000003"
  ],
  "boolean_col": [
    false
  ],
  "date_col": [
    "2021-12-14"
  ],
  "double_col": [
    3.14
  ],
  "float_col": [
    2.718
  ],
  "int_col": [
    1234
  ],
  "smallint_col": [
    280
  ],
  "text_col": [
    "ABC"
  ],
  "time_col": [
    "08:10:22.123"
  ],
  "timestamp_col": [
    "2017-05-05 00:00:00.000"
  ],
  "tinyint_col": [
    8
  ],
  "varchar_col": [
    "abc"
  ]
}
';


INSERT INTO cassarrow.simple_set JSON '{
  "partition_key": "test",
  "secondary_key": "empty"
}';


CREATE TYPE cassarrow.basic_info (
    birthday timestamp,
    nationality text,
    weight text,
    height text,
    phones frozen<list<text>>,
    );

CREATE TABLE cassarrow.cyclist_stats
(
    id       uuid PRIMARY KEY,
    lastname text,
    basics   FROZEN<basic_info>
);


INSERT INTO cassarrow.cyclist_stats JSON '
{
  "id": "50554d6e-29bb-11e5-b345-feff819cdc9f",
  "lastname": "Smith",
  "basics": {
    "birthday": "2017-05-05 00:00:00.000",
    "nationality": "British",
    "weight": "70kg",
    "height": "173cm",
    "phones": [
      "00447123123123",
      "00447456456456"
    ]
  }
}
';



INSERT INTO cassarrow.cyclist_stats JSON '
{
  "id": "50554d6e-29bb-11e5-b345-feff819cdc9a",
  "lastname": "Missing",
  "basics": null
}
';


CREATE TABLE cassarrow.simple_map
(
    partition_key varchar,
    secondary_key varchar,

    ascii_col     map<ascii, ascii>,
    bigint_col    map<bigint, bigint>,
    blob_col      map<blob, blob>,
    boolean_col   map<boolean, boolean>,
    date_col      map<date, date>,
    double_col    map<double, double>,
    float_col     map<float, float>,
    int_col       map<int, int>,
    smallint_col  map<smallint, smallint>,
    text_col      map<text, text>,
    time_col      map<time, time>,
    timestamp_col map<timestamp, timestamp>,
    tinyint_col   map<tinyint, tinyint>,
    varchar_col   map<varchar, varchar>,

    PRIMARY KEY (partition_key, secondary_key)
);

INSERT INTO cassarrow.simple_map JSON '
{
  "partition_key": "test",
  "secondary_key": "valid",
  "ascii_col": {
    "hello world":"hello world value"
  },
  "bigint_col": {
    "1234567890": 1234567890
  }
}
';
