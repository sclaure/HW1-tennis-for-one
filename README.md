The game that I have decided to implement for hw1 is Tennis For One

The game should be able to build and run well after enterring "nmake -f Makefile.win" followed by "main.exe"

The game I implemented does not use any special command line arguments in either building or running

The design document for the game can be found at the provided link:
  http://graphics.cs.cmu.edu/courses/15-466-f17/game0-design/

The only difference between the design document and the implementation is that when the game ends,
  the game outputs the contents of game over ("Game Over" message, score) to the command prompt and
  not to the game display. This is because there was no easily convenient way to demonstrate a score
  through pure rectangles. Insteaad, the main display restarts the game to be played once more.