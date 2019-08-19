# Trace Functions for Debugging Tracking
----------------------------------------

### Directory tree
- trace (.git, README.md) 
   | 
   |-frm (Qt Widget Forms)
   |
   |-inc
   | 
   |-qtcr-trace (QtCreator .pro) 
   | 
   |-qtcr-trace-test (test cases) 
   | 
   |-src
      |-main.cpp: gui app "trace" showing track results
      |-mainwindow.cpp: window class for "trace" app
      |-sql-main.cpp: console app for testing Sqlite C-functions
      |-sql_trace.cpp: class for tracing SceneTracker::updateTracks with sql db
      |-sql-trace-test.cpp: console test app for video file processing
      | and saving tracking results in sql db
      |-trace-test.cpp: test app for showing track results in highgui windows
      |-trackimages.cpp: wrapper functions for calling SceneTracker
      | from Qt application (trace) 
