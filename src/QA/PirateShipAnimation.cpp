#include "QA/PirateShipAnimation.h"
#include <Arduino.h>

// Define the frames of the ASCII animation with the new pirate ship art and hashtags
const char* pirateShipFrames[] = {
    R"(
                                                    _  _
                                                   ' \/ '
   _  _                        <|
    \/              __'__     __'__      __'__
                   /    /    /    /     /    /
                  /\____\    \____\     \____\               _  _
                 / ___!___   ___!___    ___!___               \/
               // (      (  (      (   (      (
             / /   \______\  \______\   \______\
           /  /   ____!_____ ___!______ ____!_____
         /   /   /         //         //         /
      /    /   |         ||         ||         |
     /_____/     \         \\         \\         \
           \      \_________\\_________\\_________\
            \         |          |         |
             \________!__________!_________!________/
              \|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_/|
               \    _______________                /
^^^%%%^%^^^%^%%^\_"/_)/_)_/_)__)/_)/)/)_)_"_'_"_//)/)/)/)%%%^^^%^^%%%%^
^!!^^"!%%!^^^!^^^!!^^^%%%%%!!!!^^^%%^^^!!%%%%^^^!!!!!!%%%^^^^%^^%%%^^^!
             #BadgeLife                  #BadgePirates
    )"
};

// Total number of frames
const int numFrames = sizeof(pirateShipFrames) / sizeof(pirateShipFrames[0]);

// Function to play the pirate ship animation
void playPirateShipAnimation() {
    for (int i = 0; i < numFrames; i++) {
        Serial.println(pirateShipFrames[i]);
        delay(500);  // Adjust delay for animation speed
    }
}
