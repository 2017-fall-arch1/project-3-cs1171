The goal of this project was to create a more advanced 'toy' using the MSP430.
I chose to make a clone of pong, as it uses more advanced features like
shapes and collision detection, as well as sound, text, and user input to
control the paddles.

Instructions:
These commands should be entered from the parent directory (./project3):
make all
cd toy/
make load

This will create all necessary library files and then load the program into
the MSP430.

To clean all extra files after loading the program, from the parent directory
(./project3):
make clean

How the game works:
This is basically a clone of Pong, but for those who are unfamiliar...Pong
is a game in which a ball bounces between the top and bottom of the screen.
This is a 2 player game; each player controls either the top or bottom paddle,
and the goal is to move your paddle using the buttons to block the ball
from touching your side of the screen. Player one controls the bottom paddle,
while player 2 controls the top paddle. Button 1 moves paddle 1 left, button
2 moves paddle 1 right. Button 3 moves paddle 2 left, while button 4 moves
paddle 2 right.

When the ball touches the top field, player 1 scores. When the ball touches
the bottom field, player 2 scores. The scores for each player are indicated
at the top of the screen. Each 'score' increments the value by one. The first
player to reach 10 points is the winner, and a game over screen will be shown.
The gameover screen will play a short fanfare, then the game will start over
with each player's score reset to 0. THE FUN NEVER REALLY ENDS!!!!

For the purposes of the demo, paddle 2 collision is disabled so all features
of the game can easily be shown.

All library files were provided as part of the project. I collaborated with
Jonathan Cobos when discussing how detect the collion between the ball and
paddle, so credit goes to him for using ball and paddle axis for detecting
collisions.