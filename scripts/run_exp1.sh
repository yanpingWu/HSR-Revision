#!/bin/bash
# Reproduce Exp-1: run all algorithms on one dataset with the given k value.
#
# Usage:   ./scripts/run_exp1.sh <dataset> <k>
# Example: ./scripts/run_exp1.sh bitcoin 8
#
# Each algorithm reads <dataset>/query_exp1.txt (1000 queries) and APPENDS
# its timing/answer summary to <dataset>/<ALGO>_Exp1.txt.

set -e

if [ $# -ne 2 ]; then
    echo "Usage: $0 <dataset> <k>"
    echo "  <dataset>: bitcoin | epinions | slashdot | wikiconflict | wikisign"
    echo "  <k>:       hop limit (integer)"
    exit 1
fi

DATASET=$1
K=$2
QUERY_COUNT=1000

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DATASET_DIR="$ROOT/datasets/$DATASET"

if [ ! -d "$DATASET_DIR" ]; then
    echo "Dataset directory not found: $DATASET_DIR"
    exit 1
fi

echo "===== Exp-1: dataset=$DATASET, k=$K, queries=$QUERY_COUNT ====="

echo
echo "--- [1/9] BS-BiBFS ---"
"$ROOT/algorithms/bbfs/bbfs" "$DATASET_DIR" "$K"

echo
echo "--- [2/9] BS-A* ---"
"$ROOT/algorithms/asearch/asearch" "$DATASET_DIR" "$K"

echo
echo "--- [3/9] BS-PathEnum ---"
"$ROOT/algorithms/bs_pathenum/bs_pathenum" "$DATASET_DIR" "$K"

echo
echo "--- [4/9] BS-RSPQ ---"
"$ROOT/algorithms/bs_rspq/bs_rspq_exact" "$DATASET_DIR" "$K"

echo
echo "--- [5/9] BS-Hansen ---"
"$ROOT/algorithms/bs_hansen/bs_hansen" "$DATASET_DIR" "$K"

echo
echo "--- [6/9] BS-BMAS ---"
"$ROOT/algorithms/bs_bmas/bs_bmas" "$DATASET_DIR" "$K"

echo
echo "--- [7/9] Index ---"
"$ROOT/algorithms/index/Index" "$DATASET_DIR" index "$K" "$QUERY_COUNT"

echo
echo "--- [8/9] LVO-I ---"
"$ROOT/algorithms/lvo_I/LVOI" "$DATASET_DIR" online "$K" "$QUERY_COUNT"

echo
echo "--- [9/9] LVO-II ---"
"$ROOT/algorithms/lvo_II/LVOII" "$DATASET_DIR" online "$K" "$QUERY_COUNT"

echo
echo "===== Done. Per-algorithm summaries appended to: ====="
echo "  $DATASET_DIR/BBFS_Exp1.txt"
echo "  $DATASET_DIR/Asearch_Exp1.txt"
echo "  $DATASET_DIR/BS_PATHENUM_Exp1.txt"
echo "  $DATASET_DIR/BS_RSPQ_EXACT_Exp1.txt"
echo "  $DATASET_DIR/BS_HANSEN_BCT_Exp1.txt"
echo "  $DATASET_DIR/BS_BMAS_Exp1.txt"
echo "  $DATASET_DIR/Index_I_Exp1.txt"
echo "  $DATASET_DIR/LVO_I_Exp1.txt"
echo "  $DATASET_DIR/LVO_II_Exp1.txt"
