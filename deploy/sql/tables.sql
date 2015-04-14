
\c journal_database;


DROP TABLE datachannel CASCADE;
DROP TABLE channels CASCADE;


CREATE TABLE channels
(
  id SERIAL PRIMARY KEY,
  channel_name TEXT NOT NULL
);


CREATE TABLE datachannel (
   channel_id    INTEGER references channels(id),
   time          TIMESTAMP NOT NULL,
   value         DOUBLE PRECISION
);


-- INSERT INTO test_channels (channel_name) VALUES ('test_name1');
-- INSERT INTO test_channels (channel_name) VALUES ('test_name2');
-- INSERT INTO test_datachannel (channel_id, time, value) VALUES (4, '2015-02-16 20:10:01.709486', 180.089);


