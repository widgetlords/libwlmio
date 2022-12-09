# libwlmio

A C and Python library for use with the Widgetlords [WL-MIO](https://wlmio.com/) product line.

## Examples

- https://wlmio.com/applications.html
- https://github.com/widgetlords/wlmio_examples

## Dependencies

- libcanard (MIT)
- libgpiod (LGPL-2.1)

## Package for Raspberry Pi

```
apt install devscripts meson libgpiod-dev python3-dev
debuild -b -us -uc -i
```
