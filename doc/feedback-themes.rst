.. _feedback-themes(5):

===============
feedback-themes
===============

---------------------------------
Theme configuration for feedbackd
---------------------------------

DESCRIPTION
-----------

The feedback provided to the user for an event is determined by the
currently active feedback theme.

Each theme consists of up to three profile sections named `full`,
`quiet`, and `silent` which contain event names and their associated
feedback. A theme can reference another theme via the `parent-name`
element to avoid repetition. If an event received by the daemon is not
present in the active theme or its parent theme it will be ignored.

Each event maps to at most one feedback type in each profile.
Event names in themes follow the
`Event Naming Specifiaction
<https://gitlab.freedesktop.org/feedbackd/feedbackd/-/blob/main/doc/Event-naming-spec-0.0.0.md>`_.

There are two types of themes: *custom* themes and *device* themes.
Both use the same format but serve different purposes. Custom
themes are intended to tailor feedbackd's output to the user's needs while
device themes account for hardware differences. Feedback themes use a
JSON format that can be validated with
``fbd-theme-validate(1)``.

For details on creating or modifying feedback themes, see the
``feedbackd documentation`` at https://gitlab.freedesktop.org/feedbackd/feedbackd#feedback-theme.

Feedback types
--------------

You can use the following feedback types to build a theme:

- `Sound`:  Plays a sound from the installed sound theme.
- `VibraRumble`: Produces a single rumble using the haptic motor.
- `VibraPeriodic`: Produces periodic rumbles using the haptic motor.
- `VibraPattern`: Specifies a pattern of rumbles using the haptic motor.
- `Led`: Blinks a LED in a periodic pattern.

Sound feedback
~~~~~~~~~~~~~~

Sound feedback specifies an event name from an XDG sound theme. Sound
themes are described in the
``Sound Theme Specification`` at https://freedesktop.org/wiki/Specifications/sound-theme-spec/

- `event-name`: The name of the sound event to use.
- `media-role`: The media role to assign to the audio stream being
  played. Defaults to `event`.

`Sound` feedback is usually only used in the `full` profile section of
a theme.

VibraRumble feedback
~~~~~~~~~~~~~~~~~~~~

`The VibraRumble` feedback type has the following properties:

- `count`: The number of rumbles.
- `pause`: The pause between consecutive rumbles, in milliseconds.
- `duration`: The duration of the rumble, in milliseconds.
- `magnitude`: The relative magnitude of the rumble (``[0, 1]``).

`VibraRumble` feedback is usually only used in the `quiet` profile
section of a theme.

VibraPattern feedback
~~~~~~~~~~~~~~~~~~~~~

The `VibraPattern` feedback type has the following properties:

- `magnitudes`: An array of doubles specifying the relative magnitude
  of each rumble in the range ``[0.0, 1.0]``.
- `durations`: An array of unsigned integers specifying the duration
  of each rumble, in milliseconds.

Both arrays must have the same length. `VibraPattern` feedback is
usually used only in the quiet profile section of a theme.

Led feedback
~~~~~~~~~~~~

The `Led` feedback type uses two properties to specify how an LED blinks:

- `color`: The color of the LED. It supports the fixed color names `red`,
  `green`, `blue` and `white` as well as RGB hexadecimal values in the
  ``#RRGGBB`` format.  In this format, ``RR``, ``GG``, and ``BB`` are
  two-digit hexadecimal values between ``00`` and ``FF``, specifying
  the values of the red, green, and blue components, respectively. For
  example, ``#00FFFF`` represents cyan.
- `frequency`: The LED blinking frequency, in millihertz (mHz).

`Led` feedback is usually used only in the `silent` profile section of
a theme.

See also
========

``feedbackd(8)`` ``fbcli(1)`` ``fbd-theme-validate(1)``
