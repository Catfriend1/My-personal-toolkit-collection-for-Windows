This is my personal toolkit collection for Windows. There are useful things in it like a music player with low memory and cpu footprint. There is a quicker way to see running processes and associated windows plus the ability to terminate them with one click from the systray menu. It contains key macros: "ROLL" prints the date to be used as a  filename prefix. The hotkey "WIN+Y" can lock the current user session and provides a quick alternative to "WIN+L". It has a logging component which can be set up to log process/window/lock activity on the system at three different levels of detail. Some functions may require the service part of this app to make use of the LocalSystem account. It can run as two processes (service mode and user mode) or user mode only. The app uses the TCP/IP stack, but only to communicate with its service locally like this: "127.0.0.1 -> 127.0.0.1". No information is sent outside to the internet. If logging is enabled, the logs are stored locally on your computer. The log hotkey is "WIN+S". On Windows 10, you maybe would like to import the "Disable_Cortana_WIN+S.reg" file from this repo to make the hotkey available for the app and turn it off for Cortana.