# MangoHud

A modification of the Mesa vulkan overlay. mostly just stuff to make my life easier.

## Differences

![](gifs/overlay_example.gif)

- Removed hud decoration [90a2212](https://github.com/flightlessmango/mesa/commit/90a2212055a8047d46d0220d5fdc30a76900aaed)
- Changed frametime graph to Lines instead of Histogram [e40533b](https://github.com/flightlessmango/mesa/commit/e40533b7f46858e5b9f08829e789277b2364d5d1)
- Set static min/max ms on frametime graph to act like Afterburners graph [df5238f](https://github.com/flightlessmango/mesa/commit/df5238f990218f5d6e698d572b05ddd19e52b108)
- Added CPU/GPU usage (Only Nvidia for now)
- Changed font to UbuntuMono-Bold [73f0aa9](https://github.com/flightlessmango/mesa/commit/73f0aa94d382365205a4a4128d82208315b0b190)
- Increased hud font size [b7d238b](https://github.com/flightlessmango/mesa/commit/b7d238b07eb82153f272d34bf7d1353b701f32e0)

## Environment Variables
- `MANGO_OUTPUT`: Define name and location of the output file (Required for logging)

## Keybindings
- `F2` : Toggle Logging
- `F12`: Toggle Hud

## MangoLog file

When you toggle logging on (using the keybind `F2`), a file is created with your chosen name + date/time stamp (`MANGO_OUTPUT`). this file can be uploaded to [Flightlessmango.com](https://flightlessmango.com/logs/new) to create graphs automatically.
you can share the created page with others, just link it.

#### Multiple log files

It's possible to upload multiple files. You can rename them to your preferred names and upload them in a batch.
These names will be used in the graphs.

# Normal usage
A Vulkan layer to display information about the running application
using an overlay.

To turn on the layer run :

VK_INSTANCE_LAYERS=VK_LAYER_MESA_overlay_mango /path/to/my_vulkan_app

Position the layer :

VK_INSTANCE_LAYERS=VK_LAYER_MESA_overlay_mango VK_LAYER_MESA_OVERLAY_CONFIG=position=top-right /path/to/my_vulkan_app
