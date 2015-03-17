
CREATE DATABASE journal_database;

DROP TABLE datachannel CASCADE;

CREATE TABLE datachannel (
   channel_name    TEXT NOT NULL,
   time          TIMESTAMP NOT NULL,
   value         REAL
);


CREATE OR REPLACE FUNCTION insert_data_trigger()
    RETURNS TRIGGER AS
$BODY$
DECLARE
    table_master    varchar(255)        := 'datachannel';
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
  ON datachannel
  FOR EACH ROW
  EXECUTE PROCEDURE insert_data_trigger();


