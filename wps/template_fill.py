"""
Rough script to scale up the existing classic_statusbar
It will scale up by scale_pixel_size_by, and add padding specificed by
padding_x/y.

The status bar size will be determined by main_viewport_size_by.
So if you want zero padding just make sure those two numbers are the same.

This is all based on the 8px tall classic_statusbar, but this script can
be adapted for different sbs or wps file. Just need to create a template
which is just jinja template syntax on top of a sbs or wps.

The images were hacked together, you specify their real image path
from the wps folder, but it will hack off the asset folder because
rockbox automatically adds it. Oops.
"""
from jinja2 import Environment, PackageLoader, select_autoescape
from PIL import Image
import os

env = Environment(
    loader=PackageLoader("template_fill"),
    autoescape=select_autoescape()
)

template = env.get_template("classic_statusbar.sbs.j2")

original_pixel_size = 8
padding_x = 4
padding_y = 4
scale_pixel_size_by = 2
main_viewport_size_by = 3
scaled_pixel_size = original_pixel_size * scale_pixel_size_by
main_viewport_size = original_pixel_size * main_viewport_size_by
font_id = 0

asset_folder = 'classic_statusbar'


def upscale_element(element: dict) -> dict:
    if element.get('image'):
        element_img: str = element['image']
        # Split off the last .bmp (and recombine if the filename contains periods)
        out_element_image = (
            f'{'.'.join(element_img.split('.')[0:-1])}.{scaled_pixel_size}.bmp'
        )

        try:
            if os.path.isfile(out_element_image):
                im = Image.open(out_element_image)
            else:
                im = Image.open(element_img)
                size = im.width * scale_pixel_size_by, im.height * scale_pixel_size_by
                im = im.resize(size, Image.Resampling.NEAREST)
                im.save(out_element_image, "BMP")

            # Bit of a hack really
            element['image'] = out_element_image.replace(
                f'{asset_folder}/', '')
        except IOError:
            print(f"cannot resize for <{element_img}>")

    if element.get('source_x') and element['source_x'] != '-':
        element['source_x'] *= scale_pixel_size_by
    if element.get('source_y') and element['source_y'] != '-':
        element['source_x'] *= scale_pixel_size_by

    if element.get('viewport'):
        if element['viewport']['x'] != '-':
            element['viewport']['x'] *= scale_pixel_size_by
            element['viewport']['x'] += padding_x

        if element['viewport']['y'] != '-':
            element['viewport']['y'] *= scale_pixel_size_by
            element['viewport']['y'] += padding_x

        if element['viewport']['width'] != '-':
            element['viewport']['width'] *= scale_pixel_size_by

        if element['viewport']['height'] != '-':
            element['viewport']['height'] *= scale_pixel_size_by

    return element


# These are based on the original resolution, they're upscaled later
battery = upscale_element({
    'image': 'classic_statusbar/battery.bmp',
    'source_x': 0,
    'source_y': 0,
    'sub_image_count': 16,
    'viewport': {
        'x': 0,
        'y': 0,
        'width': 17,
        'height': 7,
        'font_id': font_id
    }
})

battery_cap = upscale_element({
    'image': 'classic_statusbar/batter-y.bmp',
    'source_x': 0,
    'source_y': 0,
    'sub_image_count': 0,
    'viewport': {
        'x': 17,
        'y': 0,
        'width': 3,
        'height': 7,
        'font_id': font_id
    }
})

battery_no_icon = upscale_element({
    'viewport': {
        'x': 0,
        'y': 0,
        'width': 18,
        'height': 8,
        'font_id': font_id
    }
})

volume = upscale_element({
    'image': 'classic_statusbar/volume.bmp',
    'source_x': 1,
    'source_y': 0,
    'sub_image_count': 17,
    'viewport': {
        'x': 28,
        'y': 0,
        'width': 19,
        'height': 8,
        'font_id': font_id
    }
})

status = upscale_element({
    'image': 'classic_statusbar/status.bmp',
    'source_x': 0,
    'source_y': 0,
    'sub_image_count': 15,
    'viewport': {
        'x': 47,
        'y': 0,
        'width': 8,
        'height': 8,
        'font_id': font_id
    }
})

power = upscale_element({
    'viewport': {
        'x': 20,
        'y': 0,
        'width': 8,
        'height': 8,
        'font_id': font_id
    }
})

playback = upscale_element({
    'viewport': {
        'x': 47,
        'y': 0,
        'width': 9,
        'height': 8,
        'font_id': font_id
    }
})

repeat = upscale_element({
    'viewport': {
        'x': 56,
        'y': 0,
        'width': 9,
        'height': 8,
        'font_id': font_id
    }
})

shuffle = upscale_element({
    'viewport': {
        'x': 65,
        'y': 0,
        'width': '-',
        'height': 8,
        'font_id': font_id
    }
})

access_disk = upscale_element({
    'image': 'classic_statusbar/access_disk.bmp',
    'source_x': 0,
    'source_y': 0,
    'sub_image_count': 0,
    'viewport': {
        'x': -12,
        'y': 0,
        'width': '-',
        'height': 8,
        'font_id': font_id
    }
})


rec_bitrate = upscale_element({
    'image': 'classic_statusbar/rec_mpegbitrates.bmp',
    'source_x': 1,
    'source_y': 0,
    'sub_image_count': 18,
    'viewport': {
        'x': 28,
        'y': 0,
        'width': 19,
        'height': 8,
        'font_id': font_id
    }
})

rec_freq = upscale_element({
    'image': 'classic_statusbar/rec_frequencies.bmp',
    'source_x': 0,
    'source_y': 0,
    'sub_image_count': 12,
    'viewport': {
        'x': 55,
        'y': 0,
        'width': '-',
        'height': 8,
        'font_id': font_id
    }
})

rec_encoders = upscale_element({
    'image': 'classic_statusbar/rec_encoders.bmp',
    'source_x': 0,
    'source_y': 0,
    'sub_image_count': 3,
})

rec_channels = upscale_element({
    'image': 'classic_statusbar/rec_channels.bmp',
    'source_x': 13,
    'source_y': 0,
    'sub_image_count': 2,
})

rtc = upscale_element({
    'viewport': {
        'x': -43,
        'y': 0,
        'width': 31,
        'height': 8,
        'font_id': font_id
    }
})


rendered = template.render(
    viewport={
        'width': 0,
        'height': main_viewport_size,
        'font_id': 1
    },
    options={},
    battery=battery,
    battery_cap=battery_cap,
    battery_no_icon=battery_no_icon,
    volume=volume,
    status=status,
    access_disk=access_disk,
    rec_bitrate=rec_bitrate,
    rec_freq=rec_freq,
    rec_encoders=rec_encoders,
    rec_channels=rec_channels,
    power=power,
    playback=playback,
    repeat=repeat,
    shuffle=shuffle,
    rtc=rtc,
)

print(rendered)

with open('./classic_statusbar.sbs', 'w') as fh:
    fh.write(rendered)
