
CREATE DATABASE test_database;
CREATE USER test_user WITH password 'qwerty';
GRANT ALL privileges ON DATABASE test_database TO test_user;



DROP TABLE channelsdata CASCADE;


CREATE TABLE channelsdata (
   channel_id    INT NOT NULL,
   time          TIMESTAMP NOT NULL,
   value         REAL
);



CREATE OR REPLACE FUNCTION insert_data_trigger()
    RETURNS TRIGGER AS
$BODY$
DECLARE
    table_master    varchar(255)        := 'channelsdata';
    table_part      varchar(255)        := '';
BEGIN
    -- Даём имя партиции --------------------------------------------------
    table_part := table_master
                    || '_y' || DATE_PART( 'year', NEW.time )::TEXT
                    || '_m' || DATE_PART( 'month', NEW.time )::TEXT;

    -- Проверяем партицию на существование --------------------------------
    PERFORM
        1
    FROM
        pg_class
    WHERE
        relname = table_part
    LIMIT
        1;

    -- Если её ещё нет, то создаём --------------------------------------------
    IF NOT FOUND
    THEN
        -- Cоздаём партицию, наследуя мастер-таблицу --------------------------
        EXECUTE '
            CREATE TABLE ' || table_part || ' ( )
            INHERITS ( ' || table_master || ' )
            WITH ( OIDS=FALSE )';

        -- Создаём индексы для текущей партиции -------------------------------
--         EXECUTE '
--             CREATE INDEX ' || table_part || '_channel_date_index
--             ON ' || table_part || '
--             USING btree
--             (channel_id, date)';
    END IF;

    -- Вставляем данные в партицию --------------------------------------------
    EXECUTE '
        INSERT INTO ' || table_part || '
        SELECT ( (' || QUOTE_LITERAL(NEW) || ')::' || TG_RELNAME || ' ).*';

    RETURN NULL;
END;
$BODY$
LANGUAGE plpgsql VOLATILE
COST 100;


CREATE TRIGGER insert_data_trigger
  BEFORE INSERT
  ON channelsdata
  FOR EACH ROW
  EXECUTE PROCEDURE insert_data_trigger();



INSERT INTO channelsdata VALUES (1, '2001-02-16 20:38:40.01', 2.5),
                              (2, '2001-02-16 20:38:41.08', 1.5),
                              (3, '2001-02-16 20:38:41.08', 4.5);



INSERT INTO channelsdata VALUES (10, '1992-05-16 20:38:40.01', 2.5);

SELECT * FROM channelsdata;