swayng(1)

# NAME

swayng - An i3-compatible Wayland compositor

# SYNOPSIS

*swayng* [options...] [command]

# OPTIONS

*-h, --help*
	Show help message and quit.

*-c, --config* <config>
	Specifies a config file.

*-C, --validate*
	Check the validity of the config file, then exit.

*-d, --debug*
	Enables full logging, including debug information.

*-v, --version*
	Show the version number and quit.

*-V, --verbose*
	Enables more verbose logging.

*--get-socketpath*
	Gets the IPC socket path and prints it, then exits.

# DESCRIPTION

swayng was created to fill the need of an i3-like window manager for Wayland. The
upstream i3 developers have no intention of porting i3 to Wayland, and projects
proposed by others ended up as vaporware. Many thanks to the i3 folks for
providing such a great piece of software, so good that your users would rather
write an entirely new window manager from scratch that behaved _exactly_ like i3
rather than switch to something else.

You can run swayng directly from a tty, or via a Wayland-compatible login manager.

# CONFIGURATION

swayng searches for a config file in the following locations, in this order:

. ~/.swayng/config
. $XDG_CONFIG_HOME/swayng/config (suggested location)
. ~/.i3/config
. $XDG_CONFIG_HOME/i3/config
. /etc/swayng/config
. /etc/i3/config

If unset, $XDG_CONFIG_HOME defaults to *~/.config*.

An error is raised when no config file is found. The recommended default
configuration is usually installed to */etc/swayng/config*; you are encouraged to
copy this to *~/.config/swayng/config* and edit it from there.

For information on the config file format, see *swayng*(5).

# IPC COMMANDS

Though *swayngmsg*(1) is generally preferred, you may run *swayng* _command_ to
send _command_ to the running instance of swayng. You can also issue commands
with *i3-msg*(1) or even with *i3*(1).

# ENVIRONMENT

The following environment variables have an effect on swayng:

_SWAYSOCK_
	Specifies the path to the swayng IPC socket.

_XKB\_DEFAULT\_RULES_, _XKB\_DEFAULT\_MODEL_, _XKB\_DEFAULT\_LAYOUT_,
_XKB\_DEFAULT\_VARIANT_, _XKB\_DEFAULT\_OPTIONS_
	Configures the xkb keyboard settings. See *xkeyboard-config*(7). The
	preferred way to configure the keyboard is via the configuration file, see
	*swayng-input*(5).

The following environment variables are set by swayng:

_DISPLAY_
	If compiled with Xwayland support and Xwayland is not disabled by the
	config, this will be set to the name of the X display used for Xwayland.

_I3SOCK_
	For compatibility with i3, specifies the path to the swayng IPC socket.

_SWAYSOCK_
	Specifies the path to the swayng IPC socket.

_WAYLAND_DISPLAY_
	Specifies the name of the Wayland display that swayng is running on.

_XCURSOR_SIZE_
	Specifies the configured cursor size.

_XCURSOR_THEME_
	Specifies the configured cursor theme.

# AUTHORS

Maintained by Simon Ser <contact@emersion.fr>, who is assisted by other open
source contributors. For more information about swayng development, see
<https://github.com/swaywm/sway>.

# SEE ALSO

*swayng*(5) *swayngmsg*(1) *swayng-input*(5) *swayng-output*(5) *swayng-bar*(5)
*swayng-ipc*(7)
