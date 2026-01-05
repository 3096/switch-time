# SwitchTime
Change NetworkSystemClock

## Credit
- [@thedax](https://github.com/thedax) for NX-ntpc, from which the parent project is forked.
- [@Cytraen](https://github.com/Cytraen) for changing the servers to cloudfare.
- [@vonhabsbourg](https://github.com/vonhabsbourg) for adding month and year change.
- [@izenn](https://github.com/izenn) for the <a href="https://www.flaticon.com/free-icons/watch" title="watch icons">watch icon created by Freepik - Flaticon</a>.
- [@ZHDreamer](https://github.com/3096/switch-time/commits?author=ZHDreamer) for adding left-right navigable interaction.
- [@gzk47](https://github.com/3096/switch-time/commits?author=gzk47) for starting the GH Actions CI for this project.

## Functionality
- Change time by day/hour
- Contact a time server at `time.cloudflare.com` to set the time back to normal

## Building
Install [devkitA64 (along with libnx)](https://devkitpro.org/wiki/Getting_Started), and run `make` under the project directory.

## Disclaimer
This program changes NetworkSystemClock, which may cause a desync between console and servers. Use at your own risk! It is recommended that you only use the changed clock while offline, and change it back as soon as you are connected (either manually or using the Cloudflare time server.)
