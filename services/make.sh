set -e

make SOURCE_FILES=lcf_soh_reset.test.c test
make SOURCE_FILES=lcf_cells_reader.test.c test
make misra
