## PROJECT SUMMARY
---
Last year, me and another individual on the Formula team inherited the steering wheel code, that code was written back in MF8. The code isn't very readable, and uses some very bad practices.
For MF13, your task is to revamp the codebase. This is a very big project, and has a LOT of moving parts. Don't be intimidated though, we'll break it down into bite-size chunks.

### Step one
---
- Read the Code, no seriously, **read the code** before you type a *single* line of code make sure you know what you're actually messing with.
- I'd recommend setting a timer for an hour, talk with eachother, and dont write any code! Just read it.

### Step two
---
- Tackle can.cpp
- Find documentation for the team's can id and protocal, and then implement the empty methods

### Step three
---
- Reformat neopixel.h and neopixel.cpp
- Right now, all the code is in the header file. this is *bad practice* and the code should be moved to .cpp to match the other files accordingly

### Step four
---
- Testing and Integrate! Grab a steering wheel screen, and test and debug.
