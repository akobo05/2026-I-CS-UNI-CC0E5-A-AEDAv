# PC5 — Exploración de 3 tipos de índice en PostgreSQL

Exploración de los tres índices pedidos: **B-Tree**, **Hash** y **Espacial R-Tree (GiST)**.
Cada uno tiene su script `.sql` autocontenido (crea tabla, carga datos con volumen real,
crea el índice y mide con `EXPLAIN (ANALYZE, BUFFERS)`).

| Script | Índice | Demuestra |
| --- | --- | --- |
| `btree_index.sql` | B-Tree | rango, `BETWEEN`, `ORDER BY` sin Sort |
| `hash_index.sql`  | Hash   | igualdad exacta `=` (y por qué NO sirve para rangos) |
| `gist_index.sql`  | GiST   | contención `<@` y vecino más cercano `<->` (KNN) |

Ejecutar (cada uno por separado):

```bash
psql -d mibd -f pc5/btree_index.sql
psql -d mibd -f pc5/hash_index.sql
psql -d mibd -f pc5/gist_index.sql
```

---

## 1. Índice B-Tree

Índice **por defecto** de `CREATE INDEX`. Mantiene las claves ordenadas en un árbol
balanceado → búsqueda **O(log n)**.

* **Sirve para:** `=`, `<`, `<=`, `>=`, `>`, `BETWEEN`, `IN`, `IS NULL` y para acelerar `ORDER BY`.
* **Operadores:** los de orden total.
* **Ejemplo:** filtrar empleados por rango de salario y devolverlos ordenados.

## 2. Índice Hash

Aplica una función hash a la clave y reparte las filas en *buckets* → búsqueda por
igualdad **O(1) promedio**.

* **Sirve SOLO para:** igualdad estricta `=`. **No** soporta rangos ni `ORDER BY`.
* **Operadores:** únicamente `=`.
* **Nota:** desde PostgreSQL 10 el índice hash es *crash-safe* (se registra en el WAL).
* **Ejemplo:** buscar un token de sesión exacto.

## 3. Índice Espacial R-Tree (vía GiST)

PostgreSQL **no** tiene un access method `rtree` nativo desde la versión 8.2: la estructura
R-Tree vive dentro de **GiST** (Generalized Search Tree). Indexa *bounding boxes* que se
superponen → permite buscar lo que está dentro de / interseca / cerca de un área.

* **Sirve para:** datos geométricos/geográficos y rangos multidimensionales.
* **Operadores:** `<@` (contenido en), `@>` (contiene), `&&` (interseca), `~=` (igualdad espacial), `<->` (distancia/KNN).
* **Ejemplo:** los N lugares más cercanos a una coordenada. Para GPS/polígonos reales se usa **PostGIS** (también sobre GiST).

---

## Tabla comparativa

| Característica | B-Tree | Hash | R-Tree (GiST) |
| --- | --- | --- | --- |
| Caso de uso | rangos, orden e igualdad | igualdad exacta | datos espaciales / multidimensionales |
| Búsqueda | O(log n) | O(1) promedio | rápida en consultas espaciales |
| Rangos `<` `>` | Sí | **No** | inclusión / proximidad |
| `ORDER BY` | Sí | No | sí, por distancia (`<->`) |
| Sintaxis | `USING btree` (defecto) | `USING hash` | `USING gist` |

---

## Cómo se evidencia que el índice SÍ se usa

Con tablas pequeñas (5–10 filas) el planificador **siempre** elige `Seq Scan` porque es
más barato; el índice "no se nota". Por eso cada script:

1. Carga **volumen real** con `generate_series` (50 000–100 000 filas).
2. Corre `EXPLAIN (ANALYZE, BUFFERS)` **antes** de crear el índice → se ve `Seq Scan`.
3. Crea el índice + `ANALYZE` y repite el `EXPLAIN` → aparece `Index Scan` /
   `Bitmap Index Scan` (o `Index Scan` GiST), con menos tiempo y menos *buffers*.

Así la comparación *antes/después* prueba el uso del índice, no solo se asume.

---

## Puente con el código C++ del curso

Cada índice de Postgres es, por dentro, una estructura que ya implementamos en el curso:

* **B-Tree** → el `CBTreePage` del **EF-P1**: el mismo split/merge de páginas y la búsqueda O(log n).
* **Hash** → la **HashTable de la PC4**: función hash + manejo de colisiones, igualdad O(1).
* **R-Tree (GiST)** → la estructura espacial: árbol de *bounding boxes* (el equivalente multidimensional del B-Tree).
