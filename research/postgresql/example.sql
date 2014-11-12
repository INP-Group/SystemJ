

CREATE TABLE measurement (
   city_id          int not null,
   logdate          date not null,
   peaktemp         int,
   unitsales        int
);


CREATE TABLE measurement_y2006m02 ( ) INHERITS (measurement);
CREATE TABLE measurement_y2006m03 ( ) INHERITS (measurement);
CREATE TABLE measurement_y2007m11 ( ) INHERITS (measurement);
CREATE TABLE measurement_y2007m12 ( ) INHERITS (measurement);
CREATE TABLE measurement_y2008m01 ( ) INHERITS (measurement);


CREATE TABLE measurement_y2006m02 (
    CHECK ( logdate >= DATE '2006-02-01' AND logdate < DATE '2006-03-01' )
) INHERITS (measurement);
CREATE TABLE measurement_y2006m03 (
    CHECK ( logdate >= DATE '2006-03-01' AND logdate < DATE '2006-04-01' )
) INHERITS (measurement);

CREATE TABLE measurement_y2007m11 (
    CHECK ( logdate >= DATE '2007-11-01' AND logdate < DATE '2007-12-01' )
) INHERITS (measurement);
CREATE TABLE measurement_y2007m12 (
    CHECK ( logdate >= DATE '2007-12-01' AND logdate < DATE '2008-01-01' )
) INHERITS (measurement);
CREATE TABLE measurement_y2008m01 (
    CHECK ( logdate >= DATE '2008-01-01' AND logdate < DATE '2008-02-01' )
) INHERITS (measurement);


CREATE INDEX measurement_y2006m02_logdate ON measurement_y2006m02 (logdate);
CREATE INDEX measurement_y2006m03_logdate ON measurement_y2006m03 (logdate);
CREATE INDEX measurement_y2007m11_logdate ON measurement_y2007m11 (logdate);
CREATE INDEX measurement_y2007m12_logdate ON measurement_y2007m12 (logdate);
CREATE INDEX measurement_y2008m01_logdate ON measurement_y2008m01 (logdate);


CREATE OR REPLACE FUNCTION measurement_insert_trigger()
RETURNS TRIGGER AS $$
BEGIN
    INSERT INTO measurement_y2008m01 VALUES (NEW.*);
    RETURN NULL;
END;
$$
LANGUAGE plpgsql;


CREATE TRIGGER insert_measurement_trigger
    BEFORE INSERT ON measurement
    FOR EACH ROW EXECUTE PROCEDURE measurement_insert_trigger();


CREATE OR REPLACE FUNCTION measurement_insert_trigger()
RETURNS TRIGGER AS $$
BEGIN
    IF ( NEW.logdate >= DATE '2006-02-01' AND
         NEW.logdate < DATE '2006-03-01' ) THEN
        INSERT INTO measurement_y2006m02 VALUES (NEW.*);
    ELSIF ( NEW.logdate >= DATE '2006-03-01' AND
            NEW.logdate < DATE '2006-04-01' ) THEN
        INSERT INTO measurement_y2006m03 VALUES (NEW.*);

    ELSIF ( NEW.logdate >= DATE '2008-01-01' AND
            NEW.logdate < DATE '2008-02-01' ) THEN
        INSERT INTO measurement_y2008m01 VALUES (NEW.*);
    ELSE
        RAISE EXCEPTION 'Date out of range.  Fix the measurement_insert_trigger() function!';
    END IF;
    RETURN NULL;
END;
$$
LANGUAGE plpgsql;


----------------------

CREATE DATABASE test_database;

CREATE TABLE tabletext (
   city_id          int not null,
   DATE          date not null,
   peaktemp         int,
   unitsales        int
);



CREATE OR REPLACE FUNCTION insert_trigger()
    RETURNS TRIGGER AS
$BODY$
DECLARE
    table_master    varchar(255)        := 'tabletext';
    table_part      varchar(255)        := '';
BEGIN
    -- Даём имя партиции --------------------------------------------------
    table_part := table_master
                    || '_y' || DATE_PART( 'year', NEW.DATE )::TEXT
                    || '_m' || DATE_PART( 'month', NEW.DATE )::TEXT
                    || '_d' || DATE_PART( 'day', NEW.DATE )::TEXT;

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

--         -- Создаём индексы для текущей партиции -------------------------------
--         EXECUTE '
--             CREATE INDEX ' || table_part || '_adid_date_index
--             ON ' || table_part || '
--             USING btree
--             (ad_id, date)';
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

CREATE TRIGGER insert_trigger
  BEFORE INSERT
  ON tabletext
  FOR EACH ROW
  EXECUTE PROCEDURE insert_trigger();


INSERT INTO tabletext VALUES (1, '08-Jan-1992', 1, 1),
                              (1, '08-Jan-2000', 1, 1);