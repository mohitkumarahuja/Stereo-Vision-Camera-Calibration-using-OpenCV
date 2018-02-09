# Stereo-Vision-Camera-Calibration-using-OpenCV

I have made some functions in QT for stereo vision camera calibration, so if you want stereo camera calibration, you just need to upload your images captured from right camera and images captured from left camera. 
1. Load Images: Load images button will load all the images captured by both left and right cameras.
2. Camera Parameters: This button will compute the stereo vision parameters like internal and external parameters of both cameras as well as projection matrix of both cameras. And after computing it will save the parameters in two text files (one for each camera) and locate that files to your desired location.
3. Load Camera parameters: This button will load all your parameters saved in both text files and can be used for further purposes like computing disparity map.
4. 3D Map: This button will use the loaded parameters of the “Load camera parameters” button and will use two photos captured from both cameras and will compute the 3D view of those pictures. Which we can use for extracting 3D location of the Tip.
