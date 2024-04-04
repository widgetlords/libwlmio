# libwlmio

A C and Python library for use with the Widgetlords [WL-MIO](https://wlmio.com/) product line.

## Examples

- https://wlmio.com/applications.html
- https://github.com/widgetlords/wlmio_examples

## Dependencies

- Python 3.9
- libcanard (MIT)
- libgpiod (LGPL-2.1)

## Package for Raspberry Pi

Download a (Release)[https://github.com/widgetlords/libwlmio/releases]

OR build your own package

```
apt install devscripts meson libgpiod-dev python3-dev debhelper pkg-config
debuild -b -us -uc -i
```
