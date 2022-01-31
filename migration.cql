CREATE KEYSPACE cassarrow
  WITH REPLICATION = { 
   'class' : 'SimpleStrategy', 
   'replication_factor' : 1 
  };

CREATE TABLE cassarrow.time_series (
  event_date date,
  instrument_id int,
  event_timestamp timestamp,
  value double,
  PRIMARY KEY (event_date, instrument_id, event_timestamp)
);
