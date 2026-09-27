# HSR

## Directory layout

```
HSR/
├── Appendix.pdf             # appendix of the paper
├── Makefile                 # builds all algorithms
├── scripts/
│   └── run_exp1.sh          # one-command Exp-1 driver
├── algorithms/
│   ├── bbfs/                # BS-BiBFS
│   ├── asearch/             # BS-A*
│   ├── bs_pathenum/         # BS-PathEnum
│   ├── bs_rspq/             # BS-RSPQ
│   ├── bs_hansen/           # BS-Hansen
│   ├── bs_bmas/             # BS-BMAS
│   ├── brute_force/         # exhaustive checker
│   ├── index/               # Index
│   ├── lvo_I/               # LVO-I
│   └── lvo_II/              # LVO-II
└── datasets/
    ├── bitcoin/
    ├── epinions/
    ├── slashdot/
    ├── wikiconflict/
    └── wikisign/
```

Each dataset folder ships exactly two files:

- `graph.txt`  — the directed signed graph (one edge per line: `source target sign`,
  with `sign ≥ 1` for a positive edge and `sign ≤ 0` for a negative edge).
- `query_exp1.txt` — the query workload used in Exp-1 of the paper
  (1000 queries; one per line: `source,target,sign`, where `sign = 1` is
  a positive-reachability query and `sign = 0` is a negative-reachability
  query).

## Build

A C++11-capable compiler is required (tested with `g++` ≥ 7).

```bash
make 
make clean 
```

The Makefile compiles with `-O3 -std=c++11`.

### Manual build (alternative to `make`)

These are exactly what `make` invokes under the hood:

```bash
# BS-BiBFS
g++ -O3 -std=c++11 algorithms/bbfs/bbfs.cpp                   -o algorithms/bbfs/bbfs

# BS-A*
g++ -O3 -std=c++11 algorithms/asearch/asearch.cpp             -o algorithms/asearch/asearch

# BS-PathEnum
g++ -O3 -std=c++11 algorithms/bs_pathenum/bs_pathenum.cpp     -o algorithms/bs_pathenum/bs_pathenum

# BS-RSPQ
g++ -O3 -std=c++11 algorithms/bs_rspq/bs_rspq_exact.cpp       -o algorithms/bs_rspq/bs_rspq_exact

# BS-Hansen
g++ -O3 -std=c++11 algorithms/bs_hansen/bs_hansen.cpp         -o algorithms/bs_hansen/bs_hansen

# BS-BMAS
g++ -O3 -std=c++11 algorithms/bs_bmas/bs_bmas.cpp             -o algorithms/bs_bmas/bs_bmas

# Brute-force checker
g++ -O3 -std=c++11 algorithms/brute_force/brute_force.cpp     -o algorithms/brute_force/brute_force

# Index
g++ -O3 -std=c++11 algorithms/index/hqa.cpp                   -o algorithms/index/Index

# LVO-I
g++ -O3 -std=c++11 algorithms/lvo_I/lvo_i.cpp                 -o algorithms/lvo_I/LVOI

# LVO-II
g++ -O3 -std=c++11 algorithms/lvo_II/lvo_ii.cpp               -o algorithms/lvo_II/LVOII
```

## Reproduce Exp-1 (one command per dataset)

```bash
./scripts/run_exp1.sh <dataset> <k>
```

`<dataset>` is one of `bitcoin | epinions | slashdot | wikiconflict | wikisign`
and `<k>` is the hop constraint (a positive integer). Each algorithm reads the
1000 queries in `datasets/<dataset>/query_exp1.txt` and **appends** its
per-run summary to `datasets/<dataset>/<ALGO>_Exp1.txt`:

| Algorithm | Output file (under `datasets/<dataset>/`) |
|-----------|--------------------------------------------|
| BS-BiBFS  | `BBFS_Exp1.txt`                            |
| BS-A*     | `Asearch_Exp1.txt`                         |
| BS-PathEnum | `BS_PATHENUM_Exp1.txt`                   |
| BS-RSPQ   | `BS_RSPQ_EXACT_Exp1.txt`                   |
| BS-Hansen | `BS_HANSEN_BCT_Exp1.txt`                   |
| BS-BMAS   | `BS_BMAS_Exp1.txt`                         |
| Index     | `Index_I_Exp1.txt`                         |
| LVO-I     | `LVO_I_Exp1.txt`                           |
| LVO-II    | `LVO_II_Exp1.txt`                          |

Examples:

```bash
make
./scripts/run_exp1.sh bitcoin       6
./scripts/run_exp1.sh epinions      6
./scripts/run_exp1.sh slashdot      6
./scripts/run_exp1.sh wikiconflict  6
./scripts/run_exp1.sh wikisign      6
```

The output files are opened in append mode, so re-running the same
(dataset, k) pair leaves earlier results in place.

## Run a single algorithm manually

```bash
# BS-BiBFS baseline
./algorithms/bbfs/bbfs        <dataset_dir> <k>

# BS-A* baseline
./algorithms/asearch/asearch  <dataset_dir> <k>

# BS-PathEnum baseline
./algorithms/bs_pathenum/bs_pathenum   <dataset_dir> <k>

# BS-RSPQ baseline
./algorithms/bs_rspq/bs_rspq_exact     <dataset_dir> <k>

# BS-Hansen baseline
./algorithms/bs_hansen/bs_hansen       <dataset_dir> <k>

# BS-BMAS baseline
./algorithms/bs_bmas/bs_bmas           <dataset_dir> <k>

# Brute-force checker
./algorithms/brute_force/brute_force   <dataset_dir> <k>

# Index
./algorithms/index/Index      <dataset_dir> index  <k> 1000

# LVO-I
./algorithms/lvo_I/LVOI       <dataset_dir> online <k> 1000

# LVO-II
./algorithms/lvo_II/LVOII     <dataset_dir> online <k> 1000
```

`<dataset_dir>` is a path such as `datasets/bitcoin`. All binaries
read `query_exp1.txt` from that directory. `query_exp1.txt`
contains exactly 1000 queries.

## Per-algorithm examples

Each block below shows how to build one algorithm and immediately run it
on the `bitcoin` dataset with `k=6` over the 1000 queries in
`query_exp1.txt`.

```bash
# Build the BS-BiBFS executable
g++ -O3 -std=c++11 algorithms/bbfs/bbfs.cpp -o algorithms/bbfs/bbfs
# Run BS-BiBFS on the bitcoin dataset, reading query_exp1.txt with k=6 (1000 queries)
./algorithms/bbfs/bbfs datasets/bitcoin 6

# Build the BS-A* executable
g++ -O3 -std=c++11 algorithms/asearch/asearch.cpp -o algorithms/asearch/asearch
# Run BS-A* on the bitcoin dataset, reading query_exp1.txt with k=6 (1000 queries)
./algorithms/asearch/asearch datasets/bitcoin 6

# Build the BS-PathEnum executable
g++ -O3 -std=c++11 algorithms/bs_pathenum/bs_pathenum.cpp -o algorithms/bs_pathenum/bs_pathenum
# Run BS-PathEnum on the bitcoin dataset, reading query_exp1.txt with k=6 (1000 queries)
./algorithms/bs_pathenum/bs_pathenum datasets/bitcoin 6

# Build the BS-RSPQ executable
g++ -O3 -std=c++11 algorithms/bs_rspq/bs_rspq_exact.cpp -o algorithms/bs_rspq/bs_rspq_exact
# Run BS-RSPQ on the bitcoin dataset, reading query_exp1.txt with k=6 (1000 queries)
./algorithms/bs_rspq/bs_rspq_exact datasets/bitcoin 6

# Build the BS-Hansen executable
g++ -O3 -std=c++11 algorithms/bs_hansen/bs_hansen.cpp -o algorithms/bs_hansen/bs_hansen
# Run BS-Hansen on the bitcoin dataset, reading query_exp1.txt with k=6 (1000 queries)
./algorithms/bs_hansen/bs_hansen datasets/bitcoin 6

# Build the BS-BMAS executable
g++ -O3 -std=c++11 algorithms/bs_bmas/bs_bmas.cpp -o algorithms/bs_bmas/bs_bmas
# Run BS-BMAS on the bitcoin dataset, reading query_exp1.txt with k=6 (1000 queries)
./algorithms/bs_bmas/bs_bmas datasets/bitcoin 6

# Build the brute-force checker
g++ -O3 -std=c++11 algorithms/brute_force/brute_force.cpp -o algorithms/brute_force/brute_force
# Run the brute-force checker on the bitcoin dataset, reading query_exp1.txt with k=6 (1000 queries)
./algorithms/brute_force/brute_force datasets/bitcoin 6

# Build the Index executable
g++ -O3 -std=c++11 algorithms/index/hqa.cpp -o algorithms/index/Index
# Run Index on the bitcoin dataset, reading query_exp1.txt with k=6 (1000 queries)
./algorithms/index/Index datasets/bitcoin index 6 1000

# Build the LVO-I executable
g++ -O3 -std=c++11 algorithms/lvo_I/lvo_i.cpp -o algorithms/lvo_I/LVOI
# Run LVO-I on the bitcoin dataset, reading query_exp1.txt with k=6 (1000 queries)
./algorithms/lvo_I/LVOI datasets/bitcoin online 6 1000

# Build the LVO-II executable
g++ -O3 -std=c++11 algorithms/lvo_II/lvo_ii.cpp -o algorithms/lvo_II/LVOII
# Run LVO-II on the bitcoin dataset, reading query_exp1.txt with k=6 (1000 queries)
./algorithms/lvo_II/LVOII datasets/bitcoin online 6 1000
```
