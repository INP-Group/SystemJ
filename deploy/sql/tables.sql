
\c journal_database;


DROP TABLE datachannel CASCADE;
DROP TABLE channels CASCADE;


CREATE TABLE channels
(
  id SERIAL PRIMARY KEY,
  channel_name TEXT NOT NULL
);


CREATE TABLE datachannel (
   time          TIMESTAMP NOT NULL,
   value         DOUBLE PRECISION
   channel_id    INTEGER references channels(id),
);


-- INSERT INTO test_channels (channel_name) VALUES ('linthermcan.ThermosM.in0');
-- INSERT INTO test_channels (channel_name) VALUES ('test_name2');
-- INSERT INTO test_datachannel (channel_id, time, value) VALUES (4, '2015-02-16 20:10:01.709486', 180.089);


-- ALTER TABLE datachannel ADD COLUMN channel_id INTEGER references channels(id);
-- UPDATE datachannel
-- SET channel_id = channels.id
-- FROM channels
-- WHERE datachannel.channel_name = channels.channel_name;
-- ALTER TABLE datachannel DROP COLUMN channel_name;
-- vacuum verbose analyze;
-- vacuum full;