# Optimize background image processing for car-count application
-----------------------
### Input  
- default video source: D:\Users\Holger\counter\traffic640x480.avi

### Output
- single image files with binary foreground mask
- default path: D:\Users\Holger\counter\Skip Truck 440 - 455\\<path-depends-on-processing-steps>
- file name: mask_<frame_number>.png

### Processing steps
- apply background subtraction						(default: vibe rgb)
- medianBlur for suppressing salt-and-pepper noise	(default: blur 3)
- save frame sequence of interest					(default: frame 440 - 455)