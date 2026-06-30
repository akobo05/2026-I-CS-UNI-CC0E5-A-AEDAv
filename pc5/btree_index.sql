-- ============================================================
-- PC5 - Indice B-Tree en PostgreSQL
-- Ejecutar:  psql -d <bd> -f pc5/btree_index.sql
--
-- B-Tree = indice por defecto. Mantiene las claves ordenadas en
-- un arbol balanceado -> busqueda O(log n).
-- Sirve para: =, <, <=, >=, >, BETWEEN, IN, IS NULL y ORDER BY.
-- (Es la misma estructura del CBTreePage del EF-P1: split/merge de paginas.)
-- ============================================================

DROP TABLE IF EXISTS empleados;
CREATE TABLE empleados (
    id        SERIAL PRIMARY KEY,
    nombre    VARCHAR(50) NOT NULL,
    salario   INTEGER     NOT NULL,
    ingreso   DATE        NOT NULL
);

-- Volumen real: 100 000 filas.
-- Sin volumen el planner prefiere Seq Scan y el indice "no se nota".
INSERT INTO empleados (nombre, salario, ingreso)
SELECT 'emp_' || g,
       1000 + (random() * 9000)::int,
       DATE '2015-01-01' + (random() * 3650)::int
FROM generate_series(1, 100000) AS g;

ANALYZE empleados;

-- ------------------------------------------------------------
-- (1) SIN indice -> Seq Scan: recorre las 100 000 filas
-- ------------------------------------------------------------
EXPLAIN (ANALYZE, BUFFERS)
SELECT * FROM empleados WHERE salario BETWEEN 8000 AND 8050;

-- Creamos el B-Tree ("USING btree" es opcional: es el tipo por defecto)
CREATE INDEX idx_emp_salario ON empleados USING btree (salario);
ANALYZE empleados;

-- ------------------------------------------------------------
-- (2) CON indice -> Index Scan / Bitmap Index Scan en el rango
-- ------------------------------------------------------------
EXPLAIN (ANALYZE, BUFFERS)
SELECT * FROM empleados WHERE salario BETWEEN 8000 AND 8050;

-- ------------------------------------------------------------
-- (3) ORDER BY: el orden del B-Tree evita el paso de Sort
-- ------------------------------------------------------------
EXPLAIN (ANALYZE, BUFFERS)
SELECT id, nombre, salario FROM empleados ORDER BY salario LIMIT 10;

-- Indices existentes sobre la tabla
SELECT indexname, indexdef FROM pg_indexes WHERE tablename = 'empleados';
