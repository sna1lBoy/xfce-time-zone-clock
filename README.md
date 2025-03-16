# time zone clock
a XFCE4 panel plugin that shows additional time zones on mouseover

# installation

(requires gcc and sudo permissions)

`git clone https://codeberg.org/snailboy/xfce-time-zone-clock`

`cd xfce-time-zone-clock`

`./install`

then add to a panel


# configuration

time zones are parsed using [the time zone database](https://data.iana.org/time-zones/tz-link.html)'s identifiers (a list can be found [here](https://en.wikipedia.org/wiki/List_of_tz_database_time_zones#List))

## alternate main timezone

the time zone to use for the clock on the panel

ex: `America/Chicago`

## alternate calendar command

a command that is ran when the clock is clicked instead of displaying the default XFCE calendar

ex: `protonmail-bridge`

## date and time format

formatting is done using the [GNU calendar time format](https://www.gnu.org/software/libc/manual/html_node/Formatting-Calendar-Time.html), newlines (`\n`) are supported

ex: `%a, %b %d \n%l:%M:%S %P`

## font

the font for the panel clock

## tooltip time zones
the time zones to display in the tooltip

ex: `America/Chicago,Europe/London,UTC`
