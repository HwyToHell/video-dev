# Read png file with post processed images,
# check blob assignment to tracks
-----------------------
### Input  
- single image files with bounding box of detected objects (blue color), default path:  
D:\\Users\\Holger\\counter\\151 - 175 car\\track debug\\
- file name: mask_<frame_number>.png

### Output
- manual mode: imshow
- automatic mode: write image to file, default path:  
D:\\Users\\Holger\\counter\\151 - 175 car\\track debug\\improve_1\\

### Function
- read seqence of files (input), apply tracking algo, write tracking result
- adjusted algorithms in (track.h, track.cpp)
- SceneTracker::updateTracksIntersect  
-- call blob assignment in Track
-- combine tracks (same direction, intersection)
-- delete tracks with low confidence
- Track::updateTrackIntersect  
-- assigning blobs based on rectangle intersection (instead of distance and size)
-- change deletion to SceneTracker

