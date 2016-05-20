Tecumseh_Building_front.out.jpg: fht Tecumseh_Building_front.jpg
	./fht Tecumseh_Building_front.jpg Tecumseh_Building_front.out.jpg

fht: fht.cc -lopencv_core -lopencv_highgui -lopencv_imgproc

clean:
	rm -f Tecumseh_Building_front.out.jpg fht
