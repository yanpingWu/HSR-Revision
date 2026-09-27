CXX      = g++
CXXFLAGS = -O3 -std=c++11

BBFS        = algorithms/bbfs/bbfs
ASEARCH     = algorithms/asearch/asearch
PATHENUM    = algorithms/bs_pathenum/bs_pathenum
RSPQ        = algorithms/bs_rspq/bs_rspq_exact
HANSEN      = algorithms/bs_hansen/bs_hansen
BMAS        = algorithms/bs_bmas/bs_bmas
BRUTE       = algorithms/brute_force/brute_force
INDEX       = algorithms/index/Index
LVO_I       = algorithms/lvo_I/LVOI
LVO_II      = algorithms/lvo_II/LVOII

BINARIES = $(BBFS) $(ASEARCH) $(PATHENUM) $(RSPQ) $(HANSEN) $(BMAS) $(BRUTE) \
           $(INDEX) $(LVO_I) $(LVO_II)

.PHONY: all clean
all: $(BINARIES)

$(BBFS): algorithms/bbfs/bbfs.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(ASEARCH): algorithms/asearch/asearch.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(PATHENUM): algorithms/bs_pathenum/bs_pathenum.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(RSPQ): algorithms/bs_rspq/bs_rspq_exact.cpp algorithms/bs_rspq/Struct_type.h
	$(CXX) $(CXXFLAGS) $< -o $@

$(HANSEN): algorithms/bs_hansen/bs_hansen.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(BMAS): algorithms/bs_bmas/bs_bmas.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(BRUTE): algorithms/brute_force/brute_force.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(INDEX): algorithms/index/hqa.cpp algorithms/index/hqa.h
	$(CXX) $(CXXFLAGS) $< -o $@

$(LVO_I): algorithms/lvo_I/lvo_i.cpp algorithms/lvo_I/lvo_i.h
	$(CXX) $(CXXFLAGS) $< -o $@

$(LVO_II): algorithms/lvo_II/lvo_ii.cpp algorithms/lvo_II/lvo_ii.h
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(BINARIES)
