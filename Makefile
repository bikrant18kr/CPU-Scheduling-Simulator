CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
BINDIR = bin

.PHONY: all clean test

all: $(BINDIR)/scheduler

$(BINDIR)/scheduler: src/main.cpp
	@mkdir -p $(BINDIR) 2>/dev/null || mkdir $(BINDIR) 2>nul || true
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BINDIR)/test_scheduler: tests/test_schedulers.cpp
	@mkdir -p $(BINDIR) 2>/dev/null || mkdir $(BINDIR) 2>nul || true
	$(CXX) $(CXXFLAGS) -o $@ $<

test: $(BINDIR)/test_scheduler
	./$(BINDIR)/test_scheduler

clean:
	rm -rf $(BINDIR)
