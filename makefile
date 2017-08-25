CXXFLAGS = `pkg-config --cflags opencv`
LDFLAGS = `pkg-config --libs opencv`

all: fht.pdf Tecumseh_Building_front.out.jpg

fht.pdf: fht.tex fht.bib
	pdflatex fht
	bibtex fht
	pdflatex fht
	pdflatex fht

Tecumseh_Building_front.out.jpg: fht Tecumseh_Building_front.jpg
	./fht Tecumseh_Building_front.jpg Tecumseh_Building_front.out.jpg

fht: fht.cc -lopencv_core -lopencv_highgui -lopencv_imgproc

clean:
	rm -f Tecumseh_Building_front.out.jpg fht
	rm -f *.aux *.bbl *.blg *.log *.pdf
