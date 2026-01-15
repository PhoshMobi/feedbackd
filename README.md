# Theme based Haptic, Visual and Audio Feedback

feedbackd provides a DBus daemon (feedbackd) to act on events to provide
haptic, visual and audio feedback. It offers a library (libfeedback) and
GObject introspection bindings to ease using it from applications.

## License

feedbackd is licensed under the GPLv3+ while the libfeedback library is
licensed under LGPL 2.1+.

## Getting the source

```sh
git clone https://gitlab.freedesktop.org/agx/feedbackd
cd feedbackd
```

The `main` branch has the current development version.

## Dependencies

On a Debian based system run

```sh
sudo apt-get -y install build-essential
sudo apt-get -y build-dep .
```

to install the needed build dependencies.

For an explicit list of dependencies check the `Build-Depends` entry in the
[debian/control][] file.

## Building

We use the meson (and thereby Ninja) build system for feedbackd.  The quickest
way to get going is to do the following:

```sh
meson setup _build
meson compile -C _build
meson test -C _build
```

## Installing

To install the files to `/usr/local` you can use

```
meson install -C _build
```

however for testing and development this is usually not necessary as you
can things out of the built source tree.

## Running

### Running from the source tree

To run the daemon use

```sh
_build/run _build/src/feedbackd
```

To run under `gdb` use

``` sh
FBD_GDB=1 _build/run _build/src/feedbackd
```

You can introspect and get the current profile with

```sh
gdbus introspect --session --dest org.sigxcpu.Feedback --object-path /org/sigxcpu/Feedback
```

To run feedback for an event, use [fbcli](#fbcli)

See `examples/` for a simple python example using GObject introspection.

## How it works

Whenever an event is submitted to the daemon by a client via the DBus API *feedbackd*
looks up the corresponding feedbacks according to the current profile in the currently
active feedback theme (taking per application profile settings into account).

Any feedback triggered by a client via an event will be stopped latest when the
client disconnects from DBus. This makes sure all feedbacks get canceled if the
app that triggered them crashes.

### Feedback theme

As devices have varying capabilities and users different needs, events
are mapped to feedbacks (sound, LED, vibra) via a configurable theme.

There are two types of themes: *custom* themes and *device* themes.
They both use the same format but have different purpose. Custom
themes are meant to tweak feedbackd's output to the users needs while
device themes are meant to cater for hardware differences.

#### Custom themes

Feedbackd is shipped with a default theme `default.json`. You can
replace this by your own, custom theme in multiple ways:

1. By exporting an environment variable `FEEDBACK_THEME` with a path to a
   valid theme file. This is not recommended, use it for testing only.

1. By creating a theme file under `$XDG_CONFIG_HOME/feedbackd/themes/default.json`.
   If `XDG_CONFIG_HOME` environment variable is not set or empty, it will
   default to `$HOME/.config`

1. By creating a theme file under `$XDG_CONFIG_HOME/feedbackd/themes/custom.json` and
   telling feedbackd to use that theme. In this custom theme you only
   specify the events you want to change. Also add a `parent-name` entry
   to chain up to the default theme:

   ```json
   {
      "name: "custom"
      "parent-name": "default"
      "profiles" : [
       ...(events you want to change go here)...
      ]
   }
   ```

   This has the upside that your theme stays minimal and that new
   entries added to the default theme will automatically be used by
   your theme too. See
   [here](./tests/data/user-config/feedbackd/themes/custom.json) for
   an example.

   Once you have the file in place, tell feedbackd to use this theme
   instead of the default one:

   ```sh
   gsettings set org.sigxcpu.feedbackd theme custom
   ```

   When you want to go back to the default theme just do:

   ```sh
   gsettings reset org.sigxcpu.feedbackd theme
   ```

   Note that you can name your theme as you wish but avoid theme names
   starting with `__` or `$` as this namespace is reserved.

   This is the preferred way to specify a custom theme.

For available feedback types see the [feedback-themes][](5) manpage.

Upon reception of `SIGHUP` signal, the daemon process will proceed to retrigger
the above logic to find the themes, and reload the corresponding one. This can
be used to avoid having to restart the daemon in case of configuration changes.

### Device themes

Feedbackd has support to pick up device specific themes
automatically. This allows us to handle device differences like
varying strength of haptic motors or different LED colors in an
automatic way.

Which theme is selected for a device is determined by the device's
device tree `compatible` as supplied by the kernel. To see the list
of compatibles of a device use:

```sh
cat /sys/firmware/devicetree/base/compatible | tr '\0' "\n"
```

Check out the companion [feedbackd-device-themes][1] repository for
the current list of device-specific themes we ship by default.

Device specific themes have the same format as custom themes. They
also use the `parent-name` property to chain up to the default theme
and only override the events that need adjustment. However the rules
for finding the device theme in the file system differ and device
themes always have the `name` property set to `$device`. The simplest
device theme, doing nothing looks like:

```json
{
  "name" : "$device",
  "parent-name": "default"
}
```

If multiple device theme files exist, the selection logic follows
these steps:

1. It picks an identifier from the devicetree, until none are left
1. It searches through the folders in `XDG_DATA_DIRS` in order of appearance,
   until none are left
1. If a theme file is found in the current location with the current name,
   **it will be chosen** and other themes are ignored.

Example for a Pine64 PinePhone:

```sh
$ cat /sys/firmware/devicetree/base/compatible | tr '\0' "\n"
pine64,pinephone-1.2
pine64,pinephone
allwinner,sun50i-a64

$ echo $XDG_DATA_DIRS
/usr/local/share/:/usr/share/
```

The above selection logic would look at these concrete locations:

- `/usr/local/share/feedbackd/themes/pine64,pinephone-1.2.json` takes
  precedence over `/usr/local/share/feedbackd/themes/pine64-pinephone.json`
- `/usr/local/share/feedbackd/themes/pine64-pinephone.json` takes precedence
  over `/usr/share/feedbackd/themes/pine64-pinephone-1.2.json`
- etc...

If you create or adjust a device theme and consider the changes
generally useful, please submit them as merge request in the
[feedbackd-device-themes][1] repository.

#### Stability guarantees

Note that the feedback theme API, including the theme file format, is
not stable but considered internal to the daemon.

### Profiles

The profile determines which parts of the theme are in use:

- `full`: Use configured events from the `full`, `quiet` and `silent` parts of
  the feedback them.
- `quiet`: Use `quiet` and `silent` part from of the feedback theme. This usually
  means no audio feedback.
- `silent`: Only use the `silent` part from the feedback theme. This usually means
  to not use audio or vibra.

It can be set via a GSetting

```sh
  gsettings set org.sigxcpu.feedbackd profile full
```

## fbcli

`fbcli` can be used to trigger feedback for different events. Here are some examples:

### Phone call

Run feedbacks for event `phone-incoming-call` until explicitly stopped:

```sh
_build/cli/fbcli -t 0 -E phone-incoming-call
```

### New instant message

Run feedbacks for event `message-new-instant` just once:

```sh
_build/cli/fbcli -t -1 -E message-new-instant
```

### Alarm clock

Run feedbacks for event `message-new-instant` for 10 seconds:

```sh
_build/cli/fbcli -t 10 -E alarm-clock-elapsed
```

## Examples

Here's some examples that show how to use libfeedback in your application:

### C

The command line tool [`fbcli`](./cli/fbcli.c) can be used as example
on how to use libfeedback from C.

### Python

There's an [`example.py`](./examples/example.py) script demonstrating
how to use the introspection bindings and how to trigger feedback via
an event.

### Rust

The [libfeedback-rs](https://gitlab.gnome.org/guidog/libfeedback-rs) Rust
bindings ship an [example](https://gitlab.gnome.org/guidog/libfeedback-rs/-/blob/main/libfeedback/examples/hello-world.rs?ref_type=heads)
to demo the usage.

## Per app profiles

One can set the feedback profile of an individual application
via `GSettings`. E.g. for an app with app id `sm.puri.Phosh`
to set the profile to `quiet` do:

```sh
# If you don't have feedbackd installed, run this from the built source tree:
export GSETTINGS_SCHEMA_DIR=_build/data/
gsettings set org.sigxcpu.feedbackd.application:/org/sigxcpu/feedbackd/application/sm-puri-phosh/ profile quiet
```

## Haptic API

In order to give applications like browsers and games more control
over the haptic motor `Feebackd` implements an interface that allows
to set vibra patterns. The vibration pattern is given as a sequence of
rumble magnitude and duration pairs like:

```
busctl call --user org.sigxcpu.Feedback /org/sigxcpu/Feedback org.sigxcpu.Feedback.Haptic Vibrate 'sa(du)' org.foo.app 3 1.0 200  0.0 50  0.5 300
```

Vibration can be ended by submitting an empty array:

```sh
busctl call --user org.sigxcpu.Feedback /org/sigxcpu/Feedback org.sigxcpu.Feedback.Haptic Vibrate 'sa(du)' org.foo.app 0
```

The API is exported as a separate interface `org.sigxcpu.Feedback.Haptic` which is only available when
a haptic device is found.

## Getting in Touch

- Issue tracker: <https://gitlab.freedesktop.org/agx/feedbackd/-/issues>
- Matrix: <https://matrix.to/#/#phosh:phosh.mobi>

## Code of Conduct

Note that as a project hosted on freedesktop.org, feedbackd follows its
[Code of Conduct], based on the Contributor Covenant. Please conduct yourself
in a respectful and civilized manner when communicating with community members
on IRC and bug tracker.

## Documentation

- [Libfeedback API](https://feedbackd-b29738.pages.freedesktop.org/)
- [Event naming spec draft](./doc/Event-naming-spec-0.0.0.md)
- [Feedback-theme-spec draft](./doc/Feedback-theme-spec-0.0.0.md)
- [W3's vibration API draft](https://www.w3.org/TR/vibration/)

[debian/control]: ./debian/control#L5
[1]: https://gitlab.freedesktop.org/agx/feedbackd-device-themes
[feedback-themes]: ./doc/feedback-themes.rst
[Code of Conduct]: https://www.freedesktop.org/wiki/CodeOfConduct/
[feedbackd-themes manpage]: ./doc/feedbackd-themes.rst
