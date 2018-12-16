# Optimize tracking algorithm by reading post-processing blobs from file (frame by frame)
-----------------------
### Input  
- single image files with blob bounding boxes (blue)
- file name: mask_<frame_number>.png

### Output
- single image files with masked frame and bounding box of moving objects
- default path: D:\Users\Holger\counter\Skip Truck 440 - 455\vibe 200x200 post\
- file name: frame_masked_<frame_number>.png
- processing: apply various opencv image processing functions (medianBlur, morphology, findContours)

### Functions
- Segment-motion = providing input for post-processing
- process video in module segment-motion.cpp
- standard video source: D:\Users\Holger\counter\traffic640x480.avi
- join blobs that are close to each other
- standard output destination:  D:\Users\Holger\counter\Skip Truck 440 - 455\
- write foreground segmentation mask to .png file (each frame = one file)