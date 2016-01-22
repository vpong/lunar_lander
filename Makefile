default:
	g++ -g -Wno-c++98-compat -Wno-padded *.cpp -std=c++11 -lSDL2 -lSDL2_ttf -o lunar_lander

nodebug:
	clang++ -O3 *.cpp -std=c++11 -lSDL2 -lSDL2_ttf -o lunar_lander

nowarn:
	g++ -g *.cpp -std=c++11 -lSDL2 -lSDL2_ttf -o lunar_lander

windows:
	g++ -target x86_64-intel-win32-gnu *.cpp -std=c++11 -lSDL2 -lSDL2_ttf -o lunar_lander

clean:
	rm lunar_lander
