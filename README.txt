Implementation notes:

    A user's manual was added to aid the player whenever he starts the application or makes a command mistake.

    Whenever the GS terminates (due to manual CTRL+C or error) all the contents within the GAMES/ and SCORES/ folders will be deleted.
    This is implemented to simplify running the game.

    For some reason, when multiple folders exist in the GAMES/ directory, when exitting, the command "rm -rf GAMES/*" is unable to remove them.
    This does not happen in wsl - we assume this has to be out of our capabilities.

    The "hint" functionality in the GS is not operational. However it is still implemented so that it may (eventually) be evaluated (big if :)).
    If you choose to test the "hint" in the GS beware that it will bug the program. To fix it just CTRL+C both player and GS.

    It was not specified as such in the problem statement but an IMAGES/ file was added in the working directory in order to make it cleaner.
    The GS server takes that into account, so please insert the images into that folder.

    If you wish to change the maximum time that the player awaits a response from the server, you can do so in line 25 of the player.c file.
    Change the MAX_TIME to whatever value you want, in seconds.

    If you wish for the GS to fetch a random word from the file, change the RANDOM in line 26 of the GS.c file to value = 1.
    Otherwise, RANDOM = 0 will make the fetching sequencial.