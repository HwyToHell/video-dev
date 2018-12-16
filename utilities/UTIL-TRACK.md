# Save tracking results from video file in single images
-----------------------
### Input  
- standard video source: D:\Users\Holger\counter\traffic640x480.avi

### Output
- default path: D:\Users\Holger\counter\segment-motion\
- file names:
-- post processing results: debug_<frame_number>.png
-- tracking results: track_<frame_number>.png

### Functions
- read video
- apply post processing algos:  
-- median blur (salt and pepper noise)  
-- dilate (connect segments)  
-- find contours and bounding boxes  
-- combine close blobs  
-- discard smaller blobs  
- display diagnostic information in tracking frame:  
-- previous blob (thin line) and actual blob (thick line)  
-- track ID, confidence, lenght of track, velocity  
- write tracking results to file (each frame = one file)