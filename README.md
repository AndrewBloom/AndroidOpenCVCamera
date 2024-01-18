# AndroidOpenCVCamera
A boilerplate camera app for processing camera preview frames in real-time using the OpenCV Android SDK and the native OpenCV library.

This is intended to give developers a simple way to prototype real-time image processing techniques on a smartphone. The application is just the simplest demo for using OpenCV and uses a very slow way to grab images, which is through openGL glReadPixels. It's far from being the best practice and it's quite slow. 

As an example, this application uses OpenCV to get the Laplacian of each preview frame.

![AndroidOpenCVCamera screenshot](https://lh3.googleusercontent.com/B2lvLW6ZU6-V-k8IEN2p0ZmePYehoEFV8VEm2nTfK8M0mDfyHBpocy8JtyJEoPaBgtx2KYWTIT-nmrjHqhjb5BeWU_c9my4HydEk2LuNga2C2vCHD9dDk2Tj_jsp6_XQUwOw2AG_0W20g8eJKBEpMRLtnjDZIOzmaMkUxCRskFaF02BEEpWY1bIUwCFFfvh5u5_lcIQ0UT0LV4HST11nt09Le_yIc4hFWdjsvRpgpJLBTYCl7jPuY-MoRSXSGuAE3Fcvyj6W-FLBRICnxDU-UzDqFCcH506k9q7k8OeYIfG34geylx5MPNbCgRz7x-PUiLRZ_UTdnDMbdgSMouZUrCk3nxujNp367ya1FPzNuIW-uad4zw0AnTMz2FYyRL_Ygsqdhfp94B2qCvrcl2jFmiYGbtaJoIun3gtRwDcXkbDyjPg1sdJDrCZEEzm0zMJDkpZpfjUllQ0CleZ9ITaO_tCrI7ahJFwxTqWyLmcV3W5cMM19azaiOyaXxmnh3i0ups2A5awnMWcDRPdRfh6sJzSbAsOJ4i_-vJOWN8y2ug3r4IQXKCVrwHAwqZmPEMNEa0cJL3brO_Awvj6C4swd1hGFfeeuH-gvYuEp0uDaX7l3m6YPw5dFeb7PmLi1zMCiYdKf5Tf661pMVN9VYjruWjYYSVzxOLIODw=w377-h625-no)

## Features
- Modifies preview frame from Camera2 API with OpenCV Android SDK
- Performs image processing with native C++
- Ability to swap between front and back cameras
- Shows FPS in application

## Setup
#### [Tested with OpenCV 4.8]
This project contains only the Android code necessary for performing native image processing. In order to import and use OpenCV, users must perform a few steps, which are outlined below. You can also reference [this guide](https://proandroiddev.com/android-studio-step-by-step-guide-to-download-and-install-opencv-for-android-9ddcb78a8bc3).

### Step 1 - Download the OpenCV Android SDK
You can obtain the latest version of the OpenCV Android SDK at https://github.com/opencv/opencv/releases, labelled *opencv-{VERSION}-android-sdk.zip* under Assets. After extracting the contents of the zip file, you should have a directory called *OpenCV-android-sdk*.

### Step 2 - Import the OpenCV Android SDK as a module in this project
Open the cloned version of this repo as a project in Android Studio. Then, click File -> New -> Import Module. Navigate to where you just extracted the OpenCV Android SDK files, and use *OpenCV-android-sdk/sdk/* as the source directory for the import. Click Next, then click Finish, using the default import settings.

### Step 3 - Modify the OpenCV module's imported build.gradle
After the import completes, a new folder *sdk* is created in the root of the project. In this directory, there's a file called *build.gradle*, which you must modify in order to meet the SDK requirements of the application. Make the following changes:

for me, this meant:
compileSdkVersion 33
targetSdkVersion 33

compileOptions {
    sourceCompatibility JavaVersion.VERSION_17
    targetCompatibility JavaVersion.VERSION_17
}

buildFeatures {
    aidl = true
    buildConfig = true
}

### Step 4 - Build and run
The app should build and run now. If you want to modify the behavior of the application, `MyGLSurfaceView.onCameraTexture` is the callback used in the Java layer, and it calls *`processFrame`* to do work in the native layer.

## Credits
I updated the sample from https://github.com/J0Nreynolds/AndroidOpenCVCamera to an update version of OpenCV (4.8), the android app (manifest and gradle) and improved the CMakelist file.

## Original Credits
I created this application using OpenCV's Android samples, namely [Tutorial 4 - OpenCL](https://github.com/opencv/opencv/tree/3.4/samples/android/tutorial-4-opencl). The OpenCL sample demonstrated how to use the OpenCV Android SDK's `CameraGLSurfaceView`, which provides a nice interface for intercepting and processing Android camera preview frames.
