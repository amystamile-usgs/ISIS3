#!/usr/bin/env python3
"""
Create modern Material Design-style PNG icons directly with PIL
"""
from PIL import Image, ImageDraw, ImageFont
import os

def create_zoom_in_icon(size=24):
    """Create zoom in icon"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Circle (magnifying glass)
    center = size // 2 - 2
    radius = int(size * 0.3)
    draw.ellipse([center-radius, center-radius, center+radius, center+radius],
                 outline=(204, 204, 204, 255), width=2)

    # Handle
    handle_start = int(center + radius * 0.7)
    handle_end = int(size - 2)
    draw.line([handle_start, handle_start, handle_end, handle_end],
              fill=(204, 204, 204, 255), width=2)

    # Plus sign (teal)
    plus_size = int(radius * 0.8)
    draw.line([center, center-plus_size, center, center+plus_size],
              fill=(76, 175, 180, 255), width=2)
    draw.line([center-plus_size, center, center+plus_size, center],
              fill=(76, 175, 180, 255), width=2)

    return img

def create_zoom_out_icon(size=24):
    """Create zoom out icon"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Circle
    center = size // 2 - 2
    radius = int(size * 0.3)
    draw.ellipse([center-radius, center-radius, center+radius, center+radius],
                 outline=(204, 204, 204, 255), width=2)

    # Handle
    handle_start = int(center + radius * 0.7)
    handle_end = int(size - 2)
    draw.line([handle_start, handle_start, handle_end, handle_end],
              fill=(204, 204, 204, 255), width=2)

    # Minus sign (teal)
    minus_size = int(radius * 0.8)
    draw.line([center-minus_size, center, center+minus_size, center],
              fill=(76, 175, 180, 255), width=2)

    return img

def create_rgb_icon(size=24):
    """Create RGB color icon"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    radius = int(size * 0.2)
    y_center = size // 2

    # Red circle
    draw.ellipse([3, y_center-radius, 3+radius*2, y_center+radius],
                 fill=(255, 82, 82, 200))

    # Green circle
    cx = size // 2
    draw.ellipse([cx-radius, 3, cx+radius, 3+radius*2],
                 fill=(76, 175, 80, 200))

    # Blue circle
    draw.ellipse([size-3-radius*2, y_center-radius, size-3, y_center+radius],
                 fill=(33, 150, 243, 200))

    return img

def create_stretch_icon(size=24):
    """Create histogram/stretch icon"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Histogram bars
    bar_width = 3
    spacing = 4
    heights = [10, 14, 18, 13]

    for i, h in enumerate(heights):
        x = 4 + i * spacing
        y = size - 4 - h
        draw.rounded_rectangle([x, y, x+bar_width, size-4],
                              radius=1, fill=(76, 175, 180, 255))

    return img

def create_pan_icon(size=24):
    """Create pan/move icon"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    center = size // 2
    arrow_len = 6
    arrow_head = 3

    # Cross with arrows (teal)
    # Up
    draw.line([center, 3, center, center-1], fill=(76, 175, 180, 255), width=2)
    draw.line([center, 3, center-arrow_head, 3+arrow_head], fill=(76, 175, 180, 255), width=2)
    draw.line([center, 3, center+arrow_head, 3+arrow_head], fill=(76, 175, 180, 255), width=2)

    # Down
    draw.line([center, center+1, center, size-3], fill=(76, 175, 180, 255), width=2)
    draw.line([center, size-3, center-arrow_head, size-3-arrow_head], fill=(76, 175, 180, 255), width=2)
    draw.line([center, size-3, center+arrow_head, size-3-arrow_head], fill=(76, 175, 180, 255), width=2)

    # Left
    draw.line([3, center, center-1, center], fill=(76, 175, 180, 255), width=2)
    draw.line([3, center, 3+arrow_head, center-arrow_head], fill=(76, 175, 180, 255), width=2)
    draw.line([3, center, 3+arrow_head, center+arrow_head], fill=(76, 175, 180, 255), width=2)

    # Right
    draw.line([center+1, center, size-3, center], fill=(76, 175, 180, 255), width=2)
    draw.line([size-3, center, size-3-arrow_head, center-arrow_head], fill=(76, 175, 180, 255), width=2)
    draw.line([size-3, center, size-3-arrow_head, center+arrow_head], fill=(76, 175, 180, 255), width=2)

    return img

def main():
    icons_dir = os.path.dirname(os.path.abspath(__file__))

    icons = {
        'viewmag+.png': create_zoom_in_icon,
        'viewmag-.png': create_zoom_out_icon,
        'rgb.png': create_rgb_icon,
        'stretch.png': create_stretch_icon,
        'move.png': create_pan_icon,
    }

    # Backup old icons
    backup_dir = os.path.join(icons_dir, 'old_icons_backup')
    os.makedirs(backup_dir, exist_ok=True)

    for filename, create_func in icons.items():
        filepath = os.path.join(icons_dir, filename)

        # Backup if exists
        if os.path.exists(filepath):
            import shutil
            shutil.copy2(filepath, os.path.join(backup_dir, filename))
            print(f"Backed up: {filename}")

        # Create new icon
        img = create_func(24)
        img.save(filepath)
        print(f"Created: {filename}")

if __name__ == '__main__':
    main()
