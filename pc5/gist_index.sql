-- ============================================================
-- PC5 - Indice Espacial R-Tree (via GiST) en PostgreSQL
-- Ejecutar:  psql -d <bd> -f pc5/gist_index.sql
--
-- PostgreSQL no tiene un access method "rtree" nativo desde la version 8.2:
-- la estructura R-Tree vive dentro de GiST (Generalized Search Tree).
-- Indexa "cajas de delimitacion minima" (bounding boxes) que se superponen
-- -> permite buscar lo que esta DENTRO de, INTERSECA o esta CERCA de un area.
-- Operadores: <@ (contenido en), @> (contiene), && (interseca),
--             ~= (igualdad espacial), <-> (distancia / KNN).
-- ============================================================

DROP TABLE IF EXISTS lugares;
CREATE TABLE lugares (
    id      SERIAL PRIMARY KEY,
    nombre  VARCHAR(50) NOT NULL,
    pos     POINT       NOT NULL
);

-- 50 000 puntos aleatorios en un plano 0..1000
INSERT INTO lugares (nombre, pos)
SELECT 'p_' || g,
       point((random()*1000)::int, (random()*1000)::int)
FROM generate_series(1, 50000) AS g;

ANALYZE lugares;

-- ------------------------------------------------------------
-- (1) SIN indice -> Seq Scan sobre los 50 000 puntos
-- ------------------------------------------------------------
EXPLAIN (ANALYZE, BUFFERS)
SELECT * FROM lugares WHERE pos <@ box(point(0,0), point(50,50));

-- R-Tree = GiST: hay que pedirlo con "USING gist"
CREATE INDEX idx_lugares_pos ON lugares USING gist (pos);
ANALYZE lugares;

-- ------------------------------------------------------------
-- (2) Contencion: puntos DENTRO de un area -> Index Scan (GiST)
-- ------------------------------------------------------------
EXPLAIN (ANALYZE, BUFFERS)
SELECT * FROM lugares WHERE pos <@ box(point(0,0), point(50,50));

-- ------------------------------------------------------------
-- (3) KNN: los 5 puntos MAS CERCANOS a (500,500)
--     GiST devuelve ya ordenado por <-> (vecino mas cercano)
-- ------------------------------------------------------------
EXPLAIN (ANALYZE, BUFFERS)
SELECT nombre, pos, pos <-> point(500,500) AS dist
FROM lugares
ORDER BY pos <-> point(500,500)
LIMIT 5;

SELECT indexname, indexdef FROM pg_indexes WHERE tablename = 'lugares';

-- Para datos geograficos reales (GPS, lat/long, poligonos) se usa la
-- extension PostGIS (tipo geometry/geography), que tambien se apoya en GiST.
