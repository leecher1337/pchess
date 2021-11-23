# What is it?

I recently stumbled upon the following thread:
https://sourceforge.net/p/dxwnd/discussion/general/thread/98dd46dfc6/?page=0

Some people were trying to run an old Chess Game by Sierra called
PowerChess. It can be found at:
https://www.old-games.ru/game/download/7740.html

PowerChess uses 2 DOS Chess-Engines that are communicating via STDIN/STDOUT
Pipes with the graphical frontend. On newer Windows Versions, this 
unfortunately fails, due to the use of flag DETACHED_PROCESS (0x8), which
is incompaitble with NTVDM launcher code.
Additionally, the Pipe communication via STDIN/STDOUT doesn't work out of
the box when launching the DOS application directly, so it needs to be
wrapped with CMD /C.
Even worse, the pipes also get stuck, if the DOS Chess engine is thinking 
about its next move. The executables responsible for the respective Chess 
Engines in use are KING.EXE and QUEEN.EXE. They are in Pharlap .EXP format 
(old header) and are launched via RUN386.EXE launcher for PharLap DOS 
Extender.

# How does it work?

So in order to get it running:
1) The CreateProcess flags need to be patched: 
  * Remove DETACHED_PROCESS flag
  * Add CREATE_NO_WINDOW flag
2) RUN386.EXE needs to be wrapped by a process that spawns original 
   RUN386 with CMD /C prepended and reads the stdout pipe bytewise so
   the GUI process doesn't get stuck on "Thinking..."

This patcher tries to accomplish both by 
1) Patch CreateProcess in PCHESS.EXE as mentioned above
2) Rename original RUN386.EXE to RUN386D.EXE
3) Place a RUN386.EXE wrapper, that launches RUN386D as child and does
   correct Pipe forwarding

# Installation

If you already installed PowerChess, just run Patcher.exe from Releases

# Supported Operating Systems

Tested on:

 * Plain Windows XP 32bit.
 * windows 7 64bit with NTVDMx64 (https://github.com/leecher1337/ntvdmx64/)
   CCPU Free Build  (compiled with autobuild-ccpu-fre.cmd)
 * Windows 11 64bit with NTVDMx64
 
So it should theoretically work on all Windows Versions

# Contact

Project site: https://github.com/leecher1337/pchess
If you have questions, feel free to ask, i.e. in Issue tracker
