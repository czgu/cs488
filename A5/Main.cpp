#include "Game.hpp"

#include <iostream>
using namespace std;

int main( int argc, char **argv ) 
{
	CS488Window::launch(argc, argv, new Game(), 1024, 768, "Legend of Cube");
	return 0;
}
