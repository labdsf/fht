Tecumseh_Building_front.out.jpg: 1 Tecumseh_Building_front.jpg
	./1 Tecumseh_Building_front.jpg Tecumseh_Building_front.out.jpg

1: 1.cc -lopencv_core -lopencv_highgui -lopencv_imgproc

clean:
	rm -f Tecumseh_Building_front.out.jpg 1
