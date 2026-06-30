-- ============================================================
-- PC5 - Indice Hash en PostgreSQL
-- Ejecutar:  psql -d <bd> -f pc5/hash_index.sql
--
-- Aplica una funcion hash a la clave y reparte las filas en buckets
-- -> busqueda por igualdad O(1) promedio.
-- Sirve SOLO para "=": no soporta rangos (<, >) ni ORDER BY.
-- (Misma idea de la HashTable de la PC4: funcion hash + manejo de colisiones.)
-- Nota: desde PostgreSQL 10 el indice hash es crash-safe / replicado por WAL.
-- ============================================================

DROP TABLE IF EXISTS sesiones;
CREATE TABLE sesiones (
    id      SERIAL PRIMARY KEY,
    token   TEXT     NOT NULL,      -- token tipo MD5 (32 hex); TEXT para que
                                    -- "= md5(...)" (text) use la opclass del hash
    activa  BOOLEAN  DEFAULT TRUE
);

-- 100 000 tokens. Buscaremos md5('sesion-50000').
INSERT INTO sesiones (token)
SELECT md5('sesion-' || g)
FROM generate_series(1, 100000) AS g;

ANALYZE sesiones;

-- ------------------------------------------------------------
-- (1) SIN indice -> Seq Scan sobre las 100 000 filas
-- ------------------------------------------------------------
EXPLAIN (ANALYZE, BUFFERS)
SELECT * FROM sesiones WHERE token = md5('sesion-50000');

-- Hash NO es el defecto: hay que pedirlo con "USING hash"
CREATE INDEX idx_ses_token ON sesiones USING hash (token);
ANALYZE sesiones;

-- ------------------------------------------------------------
-- (2) CON indice hash -> Index Scan por igualdad exacta
-- ------------------------------------------------------------
EXPLAIN (ANALYZE, BUFFERS)
SELECT * FROM sesiones WHERE token = md5('sesion-50000');

-- ------------------------------------------------------------
-- (3) El hash NO sirve para rangos: esto IGNORA el indice (Seq Scan)
-- ------------------------------------------------------------
EXPLAIN (ANALYZE, BUFFERS)
SELECT * FROM sesiones WHERE token > md5('sesion-50000');

SELECT indexname, indexdef FROM pg_indexes WHERE tablename = 'sesiones';
