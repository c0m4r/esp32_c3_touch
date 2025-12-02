from PIL import Image
import sys
import os

def convert_image(input_path, output_path):
    # Check if input exists
    if not os.path.exists(input_path):
        print(f"Error: {input_path} not found")
        sys.exit(1)

    img = Image.open(input_path)
    img = img.resize((320, 240)) # Ensure correct size
    img = img.convert('RGB')
    
    pixels = list(img.getdata())
    
    with open(output_path, 'w') as f:
        f.write('#ifndef IMAGE_DATA_H\n')
        f.write('#define IMAGE_DATA_H\n\n')
        f.write('#include <pgmspace.h>\n\n')
        
        # Write RGB565 data
        f.write('const uint16_t splash_image_rgb565[320*240] PROGMEM = {\n')
        
        for i, p in enumerate(pixels):
            r, g, b = p
            # Convert to RGB565
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            f.write(f'0x{rgb565:04X}, ')
            if (i + 1) % 16 == 0:
                f.write('\n')
                
        f.write('};\n\n')
        f.write('#endif\n')

if __name__ == '__main__':
    convert_image('esp32_c3_super_mini.png', 'image_data.h')
    print("Converted esp32_c3_super_mini.png to image_data.h")
