OUTDIR=build

.DEFAULT_GOAL := build

builddir:
	mkdir -p $(OUTDIR)

$(OUTDIR)/chi: chi.c
	gcc -g -o $(OUTDIR)/chi $<

build: builddir $(OUTDIR)/chi

run: build
	env RUST_BACKTRACE=1 $(OUTDIR)/chi

clean:
	rm -rf $(OUTDIR)
