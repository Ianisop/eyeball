# eyeba# Eyeball
ll
Eyeball uses inotify to sort your junk and keeps your downloads folder empty.
It's designed to run as a cli tool, which forks it's own daemon to run in the background.
It's super lightweight and works by moving your regular files to their own folders.
Eyeball is configurable through it's own config file called ```paths.config```.
Simply create or move the provided example config to ```/.config/eyeball/``` and your're ready to run.

## Building
Eyeball requires cmake and c++ 17 to work, so make sure you have that ready.
You can easily build eyeball via the provided build script.
>Remember to give the build script the right permissions.
```./build.sh```
And you're ready to go! 
## Running
You can toggle Eyeball via ```eyeball on``` and turn off again via ```eyeball off```.
You can also view its current status via the ```eyeball status``` command any time, in case you forgot.

>It's currently **LINUX** only, sorry :(
